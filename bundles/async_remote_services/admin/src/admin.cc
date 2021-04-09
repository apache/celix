/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <admin.h>
#include <celix_api.h>
#include <pubsub_message_serialization_service.h>
#include <ConfiguredEndpoint.h>
#include "celix/rsa/Constants.h"

#define L_DEBUG(...) \
    if (_logService) {                                                  \
        _logService->debug(_logService->handle, __VA_ARGS__);           \
    }
#define L_INFO(...) \
    if (_logService) {                                                  \
        _logService->info(_logService->handle, __VA_ARGS__);            \
    }
#define L_WARN(...) \
    if (_logService) {                                                  \
        _logService->warning(_logService->handle, __VA_ARGS__);         \
    }
#define L_ERROR(...) \
    if (_logService) {                                                  \
        _logService->error(_logService->handle, __VA_ARGS__);           \
    }

celix::async_rsa::AsyncAdmin::AsyncAdmin(std::shared_ptr<celix::BundleContext> ctx) noexcept : _ctx{std::move(ctx)} {

}

void celix::async_rsa::AsyncAdmin::addEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint) {
    assert(endpoint);
    std::unique_lock l(_m);

    auto interface = endpoint->getExportedInterfaces();
    if (interface.empty()) {
        L_WARN("Adding endpoint but missing exported interfaces");
        return;
    }

    auto endpointId = endpoint->getEndpointId();
    if (endpointId.empty()) {
        L_WARN("Adding endpoint but missing service id");
        return;
    }
    _toBeImportedServices.emplace_back(endpoint);

    l.unlock();
    createImportServices();
}

void celix::async_rsa::AsyncAdmin::removeEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint) {
    assert(endpoint);
    std::unique_lock l(_m);

    auto id = endpoint->getEndpointId();

    if (id.empty()) {
        L_WARN("Cannot remove endpoint without a valid id");
        return;
    }

    _toBeImportedServices.erase(std::remove_if(_toBeImportedServices.begin(), _toBeImportedServices.end(), [&id](auto const &endpoint){
        return id == endpoint->getEndpointId();
    }), _toBeImportedServices.end());


    auto instanceIt = _importedServiceInstances.find(id);

    if (instanceIt == end(_importedServiceInstances)) {
        return;
    }

    _ctx->getDependencyManager()->removeComponentAsync(instanceIt->second);
    _importedServiceInstances.erase(instanceIt);
}

void celix::async_rsa::AsyncAdmin::addImportedServiceFactory(const std::shared_ptr<celix::async_rsa::IImportedServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties) {
    auto interface = properties->get(celix::rsa::Endpoint::EXPORTS);

    if (interface.empty()) {
        L_DEBUG("Adding service factory but no exported interfaces");
        return;
    }

    std::unique_lock l(_m);

    auto existingFactory = _importedServiceFactories.find(interface);

    if(existingFactory != end(_importedServiceFactories)) {
        L_WARN("Adding imported factory but factory already exists");
        return;
    }

    _importedServiceFactories.emplace(interface, factory);

    l.unlock();
    createImportServices();
}

void celix::async_rsa::AsyncAdmin::removeImportedServiceFactory(const std::shared_ptr<celix::async_rsa::IImportedServiceFactory>& /*factory*/, const std::shared_ptr<const celix::Properties>& properties) {
    auto interface = properties->get(celix::rsa::Endpoint::EXPORTS);

    if (interface.empty()) {
        L_WARN("Removing service factory but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);
    _importedServiceFactories.erase(interface);

    //TODO TBD are the removal of the imported services also needed when a ImportedServiceFactory is removed (and maybe the bundle is uninstalled?)
}

void celix::async_rsa::AsyncAdmin::createImportServices() {
    std::lock_guard<std::mutex> lock{_m};

    for (auto it = _toBeImportedServices.begin(); it != _toBeImportedServices.end(); ++it) {
        auto interface = (*it)->getExportedInterfaces();
        auto existingFactory = _importedServiceFactories.find(interface);
        if (existingFactory != end(_importedServiceFactories)) {
            auto endpointId = (*it)->getEndpointId();
            L_DEBUG("Adding endpoint, created service");
            _importedServiceInstances.emplace(endpointId, existingFactory->second->create(endpointId).getUUID());
            _toBeImportedServices.erase(it--);
        } else {
            L_DEBUG("Adding endpoint to be imported but no factory available yet, delaying import");
        }
    }
}

void celix::async_rsa::AsyncAdmin::addExportedServiceFactory(const std::shared_ptr<celix::async_rsa::IExportedServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties) {
    auto interface = properties->get(celix::rsa::Endpoint::EXPORTS);

    if (interface.empty()) {
        L_WARN("Adding exported factory but missing exported interfaces");
        return;
    }

    {
        std::lock_guard<std::mutex> lock{_m};
        _exportedServiceFactories.emplace(interface, factory);
    }

    createExportServices();
}

void celix::async_rsa::AsyncAdmin::removeExportedServiceFactory(const std::shared_ptr<celix::async_rsa::IExportedServiceFactory>& /*factory*/, const std::shared_ptr<const celix::Properties>& properties) {
    auto interface = properties->get(celix::rsa::Endpoint::EXPORTS);

    if(interface.empty()) {
        L_WARN("Removing imported factory but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);
    _exportedServiceFactories.erase(interface);

    //TODO TBD are the removal of the exported services also needed when a ExportedServiceFactory is removed (and maybe the bundle is uninstalled?)
}

void celix::async_rsa::AsyncAdmin::addService(const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& props) {
    assert(props->getAsBool(celix::rsa::REMOTE_SERVICE_PROPERTY_NAME, false)); //"only remote services should be tracker by the service tracker");

    auto serviceName = props->get(celix::SERVICE_NAME, "");
    if (serviceName.empty()) {
        L_WARN("Adding service to be exported but missing objectclass");
        return;
    }

    {
        std::lock_guard<std::mutex> lock{_m};
        _toBeExportedServices.emplace_back(std::make_pair(std::move(svc), std::move(props)));
    }

    createExportServices();
}

void celix::async_rsa::AsyncAdmin::removeService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    assert(props->getAsBool(celix::rsa::REMOTE_SERVICE_PROPERTY_NAME, false)); //"only remote services should be tracker by the service tracker");

    long svcId = props->getAsLong(celix::SERVICE_ID, -1);
    if (svcId < 0) {
        L_WARN("Removing service to be exported but missing service.id");
        return;
    }

    std::unique_lock l(_m);

    //remove export interface (if present)
    auto instanceIt = _exportedServiceInstances.find(svcId);
    if (instanceIt != end(_exportedServiceInstances)) {
        //note cannot remove async, because the svc lifetime needs to extend the lifetime of the component.
        //TODO improve by creating export service based on svc id instead of svc pointer, so that export service can track its own dependencies.
        _ctx->getDependencyManager()->removeComponent(instanceIt->second);
        _exportedServiceInstances.erase(instanceIt);
    }

    //remove to be exported endpoint (if present)
    for (auto it = _toBeExportedServices.begin(); it != _toBeExportedServices.end(); ++it) {
        if (it->second->getAsBool(celix::SERVICE_ID, -1) == svcId) {
            _toBeExportedServices.erase(it);
            break;
        }
    }

}

void celix::async_rsa::AsyncAdmin::createExportServices() {
    std::lock_guard<std::mutex> lock{_m};

    for (auto it = _toBeExportedServices.begin(); it != _toBeExportedServices.end(); ++it) {
        auto serviceName = it->second->get(celix::SERVICE_NAME, "");
        auto svcId = it->second->getAsLong(celix::SERVICE_ID, -1);
        if (serviceName.empty()) {
            L_WARN("Adding service to be exported but missing objectclass for svc id %li", svcId);
        } else {
            auto factory = _exportedServiceFactories.find(serviceName);
            if (factory != _exportedServiceFactories.end()) {
                auto endpointId = _ctx->getFramework()->getUUID() + "-" + std::to_string(svcId);
                auto cmpUUID = factory->second->create(it->first.get(), std::move(endpointId)).getUUID();
                _exportedServiceInstances.emplace(svcId, std::move(cmpUUID));
                _toBeExportedServices.erase(it--);
            } else {
                L_DEBUG("Adding service to be exported but no factory available yet, delaying creation");
            }
        }
    }
}

void celix::async_rsa::AsyncAdmin::setLogger(const std::shared_ptr<celix_log_service>& logService) {
    _logService = logService;
}

class AdminActivator {
public:
    explicit AdminActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto admin = std::make_shared<celix::async_rsa::AsyncAdmin>(ctx);

        auto& cmp = ctx->getDependencyManager()->createComponent(admin);
        cmp.createServiceDependency<celix::rsa::Endpoint>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addEndpoint, &celix::async_rsa::AsyncAdmin::removeEndpoint);
        cmp.createServiceDependency<celix::async_rsa::IImportedServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addImportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeImportedServiceFactory);
        cmp.createServiceDependency<celix::async_rsa::IExportedServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addExportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeExportedServiceFactory);
        cmp.createServiceDependency<celix_log_service>(CELIX_LOG_SERVICE_NAME)
                .setRequired(false)
                .setFilter(std::string{"("}.append(CELIX_LOG_SERVICE_PROPERTY_NAME).append("=celix::rsa::RemoteServiceAdmin)"))
                .setStrategy(celix::dm::DependencyUpdateStrategy::suspend)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::setLogger);
        cmp.build();

        //note adding void service dependencies is not supported for the dependency manager, using a service tracker instead.
        _remoteServiceTracker = ctx->trackServices<void>()
                .setFilter(std::string{"("}.append(celix::rsa::REMOTE_SERVICE_PROPERTY_NAME).append("=*)"))
                .addAddWithPropertiesCallback([admin](const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties) {
                    admin->addService(svc, properties);
                })
                .addRemWithPropertiesCallback([admin](const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties) {
                    admin->removeService(svc, properties);
                })
                .build();
    }
private:
    std::shared_ptr<celix::ServiceTracker<void>> _remoteServiceTracker{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(AdminActivator)