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

void celix::async_rsa::AsyncAdmin::addEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint) {
    assert(endpoint);

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

    std::lock_guard l(_m);
    _toBeImportedServices.emplace_back(endpoint);
    createImportServices();
}

void celix::async_rsa::AsyncAdmin::removeEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint) {
    assert(endpoint);

    auto id = endpoint->getEndpointId();
    if (id.empty()) {
        L_WARN("Cannot remove endpoint without a valid id");
        return;
    }

    std::shared_ptr<celix::rsa::IImportServiceGuard> tmpStore{}; //to ensure destruction outside of lock
    {
        std::lock_guard l(_m);

        _toBeImportedServices.erase(std::remove_if(_toBeImportedServices.begin(), _toBeImportedServices.end(), [&id](auto const &endpoint){
            return id == endpoint->getEndpointId();
        }), _toBeImportedServices.end());

        auto it = _importedServices.find(id);
        if (it != _importedServices.end()) {
            tmpStore = std::move(it->second);
            _importedServices.erase(it);
        }
    }
}

void celix::async_rsa::AsyncAdmin::addImportedServiceFactory(
        const std::shared_ptr<celix::rsa::IImportServiceFactory> &factory,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME);
    if (targetServiceName.empty()) {
        L_WARN("Adding service factory but missing %s property", celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME);
        return;
    }


    std::lock_guard l(_m);
    auto existingFactory = _importServiceFactories.find(targetServiceName);
    if (existingFactory != end(_importServiceFactories)) {
        L_WARN("Adding imported factory but factory already exists");
        return;
    }

    _importServiceFactories.emplace(targetServiceName, factory);

    createImportServices();
}

void celix::async_rsa::AsyncAdmin::removeImportedServiceFactory(
        const std::shared_ptr<celix::rsa::IImportServiceFactory> &/*factory*/,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME);
    if (targetServiceName.empty()) {
        L_WARN("Removing service factory but missing %s property", celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME);
        return;
    }

    std::lock_guard l(_m);
    _importServiceFactories.erase(targetServiceName);

    //TODO remove imported services from this factory ??needed
}

void celix::async_rsa::AsyncAdmin::addExportedServiceFactory(
        const std::shared_ptr<celix::rsa::IExportServiceFactory> &factory,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME);
    if (targetServiceName.empty()) {
        L_WARN("Adding service factory but missing %s property", celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME);
        return;
    }

    std::lock_guard<std::mutex> lock{_m};
    _exportServiceFactories.emplace(targetServiceName, factory);
    createExportServices();
}

void celix::async_rsa::AsyncAdmin::removeExportedServiceFactory(
        const std::shared_ptr<celix::rsa::IExportServiceFactory> &/*factory*/,
        const std::shared_ptr<const celix::Properties> &properties) {
    auto targetServiceName = properties->get(celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME);
    if (targetServiceName.empty()) {
        L_WARN("Removing service factory but missing %s property", celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME);
        return;
    }

    std::lock_guard l(_m);
    _exportServiceFactories.erase(targetServiceName);

    //TODO remove exported services from this factory ??needed
}

void celix::async_rsa::AsyncAdmin::addService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    auto serviceName = props->get(celix::SERVICE_NAME, "");
    if (serviceName.empty()) {
        L_WARN("Adding service to be exported but missing objectclass");
        return;
    }

    std::lock_guard<std::mutex> lock{_m};
    _toBeExportedServices.emplace_back(props);
    createExportServices();
}

void celix::async_rsa::AsyncAdmin::removeService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    long svcId = props->getAsLong(celix::SERVICE_ID, -1);
    if (svcId < 0) {
        L_WARN("Removing service to be exported but missing service.id");
        return;
    }

    std::lock_guard l(_m);

    auto instanceIt = _exportedServices.find(svcId);
    if (instanceIt != end(_exportedServices)) {
        _exportedServices.erase(instanceIt);
    }

    //remove to be exported endpoint (if present)
    for (auto it = _toBeExportedServices.begin(); it != _toBeExportedServices.end(); ++it) {
        if ((*it)->getAsBool(celix::SERVICE_ID, -1) == svcId) {
            _toBeExportedServices.erase(it);
            break;
        }
    }
}

void celix::async_rsa::AsyncAdmin::createImportServices() {
    auto it = _toBeImportedServices.begin();
    while (it != _toBeImportedServices.end()) {
        auto interface = (*it)->getExportedInterfaces();
        auto existingFactory = _importServiceFactories.find(interface);
        if (existingFactory == end(_importServiceFactories)) {
            L_DEBUG("Adding endpoint to be imported but no factory available yet, delaying import");
            ++it;
            continue;
        }
        auto endpointId = (*it)->getEndpointId();
        L_DEBUG("Adding endpoint, created service");
        _importedServices.emplace(endpointId, existingFactory->second->importService(**it));
        it = _toBeImportedServices.erase(it);
    }
}

void celix::async_rsa::AsyncAdmin::createExportServices() {
    auto it = _toBeExportedServices.begin();
    while (it != _toBeExportedServices.end()) {
        const auto& svcProperties = **it;
        auto serviceName = svcProperties.get(celix::SERVICE_NAME, "");
        auto svcId = svcProperties.getAsLong(celix::SERVICE_ID, -1);
        if (serviceName.empty()) {
            L_WARN("Adding service to be exported but missing objectclass for svc id %li", svcId);
            continue;
        }
        auto factory = _exportServiceFactories.find(serviceName);
        if (factory == end(_exportServiceFactories)) {
            L_DEBUG("Adding service to be exported but no factory available yet, delaying creation");
            ++it;
            continue;
        }
        _exportedServices.emplace(svcId, factory->second->exportService(svcProperties));
        it = _toBeExportedServices.erase(it);
    }
}

void celix::async_rsa::AsyncAdmin::setLogger(const std::shared_ptr<celix_log_service>& logService) {
    _logService = logService;
}

class AdminActivator {
public:
    explicit AdminActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto admin = std::make_shared<celix::async_rsa::AsyncAdmin>();

        auto& cmp = ctx->getDependencyManager()->createComponent(admin);
        cmp.createServiceDependency<celix::rsa::Endpoint>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addEndpoint, &celix::async_rsa::AsyncAdmin::removeEndpoint);
        cmp.createServiceDependency<celix::rsa::IImportServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addImportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeImportedServiceFactory);
        cmp.createServiceDependency<celix::rsa::IExportServiceFactory>()
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
        _remoteServiceTracker = ctx->trackAnyServices()
                .setFilter(std::string{"("}.append(celix::rsa::REMOTE_SERVICE_EXPORTED_PROPERTY_NAME).append("=*)"))
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