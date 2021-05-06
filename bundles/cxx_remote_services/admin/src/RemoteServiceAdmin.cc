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

#include "RemoteServiceAdmin.h"
#include "celix/rsa/RemoteConstants.h"
#include "celix/rsa/IImportServiceFactory.h"
#include "celix/rsa/IExportServiceFactory.h"

#define L_DEBUG(...) \
        _logHelper.debug(__VA_ARGS__);
#define L_INFO(...) \
        _logHelper.info(__VA_ARGS__);
#define L_WARN(...) \
        _logHelper.warning(__VA_ARGS__);
#define L_ERROR(...) \
        _logHelper.error(__VA_ARGS__);

celix::rsa::RemoteServiceAdmin::RemoteServiceAdmin(celix::LogHelper logHelper) : _logHelper{std::move(logHelper)} {}

void celix::rsa::RemoteServiceAdmin::addEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint) {
    assert(endpoint);

    auto interface = endpoint->getInterface();
    if (interface.empty()) {
        L_WARN("Adding endpoint but missing exported interfaces");
        return;
    }

    auto endpointId = endpoint->getId();
    if (endpointId.empty()) {
        L_WARN("Adding endpoint but missing service id");
        return;
    }

    std::lock_guard l(_m);
    _toBeImportedServices.emplace_back(endpoint);
    createImportServices();
}

void celix::rsa::RemoteServiceAdmin::removeEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint) {
    assert(endpoint);

    auto id = endpoint->getId();
    if (id.empty()) {
        L_WARN("Cannot remove endpoint without a valid id");
        return;
    }

    std::shared_ptr<celix::rsa::IImportServiceGuard> tmpStore{}; //to ensure destruction outside of lock
    {
        std::lock_guard l(_m);

        _toBeImportedServices.erase(std::remove_if(_toBeImportedServices.begin(), _toBeImportedServices.end(), [&id](auto const &endpoint){
            return id == endpoint->getId();
        }), _toBeImportedServices.end());

        auto it = _importedServices.find(id);
        if (it != _importedServices.end()) {
            tmpStore = std::move(it->second);
            _importedServices.erase(it);
        }
    }
}

void celix::rsa::RemoteServiceAdmin::addImportedServiceFactory(
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

void celix::rsa::RemoteServiceAdmin::removeImportedServiceFactory(
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

void celix::rsa::RemoteServiceAdmin::addExportedServiceFactory(
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

void celix::rsa::RemoteServiceAdmin::removeExportedServiceFactory(
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

void celix::rsa::RemoteServiceAdmin::addService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
    auto serviceName = props->get(celix::SERVICE_NAME, "");
    if (serviceName.empty()) {
        L_WARN("Adding service to be exported but missing objectclass");
        return;
    }

    std::lock_guard<std::mutex> lock{_m};
    _toBeExportedServices.emplace_back(props);
    createExportServices();
}

void celix::rsa::RemoteServiceAdmin::removeService(const std::shared_ptr<void>& /*svc*/, const std::shared_ptr<const celix::Properties>& props) {
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
        if ((*it)->getAsLong(celix::SERVICE_ID, -1) == svcId) {
            _toBeExportedServices.erase(it);
            break;
        }
    }
}

void celix::rsa::RemoteServiceAdmin::createImportServices() {
    auto it = _toBeImportedServices.begin();
    while (it != _toBeImportedServices.end()) {
        auto interface = (*it)->getInterface();
        auto existingFactory = _importServiceFactories.find(interface);
        if (existingFactory == end(_importServiceFactories)) {
            L_DEBUG("Adding endpoint to be imported but no factory available yet, delaying import");
            ++it;
            continue;
        }
        auto endpointId = (*it)->getId();
        L_DEBUG("Adding endpoint, created service");
        _importedServices.emplace(endpointId, existingFactory->second->importService(**it));
        it = _toBeImportedServices.erase(it);
    }
}

void celix::rsa::RemoteServiceAdmin::createExportServices() {
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

class AdminActivator {
public:
    explicit AdminActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto admin = std::make_shared<celix::rsa::RemoteServiceAdmin>(celix::LogHelper{ctx, celix::typeName<celix::rsa::RemoteServiceAdmin>()});

        auto& cmp = ctx->getDependencyManager()->createComponent(admin);
        cmp.createServiceDependency<celix::rsa::EndpointDescription>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addEndpoint, &celix::rsa::RemoteServiceAdmin::removeEndpoint);
        cmp.createServiceDependency<celix::rsa::IImportServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addImportedServiceFactory, &celix::rsa::RemoteServiceAdmin::removeImportedServiceFactory);
        cmp.createServiceDependency<celix::rsa::IExportServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addExportedServiceFactory, &celix::rsa::RemoteServiceAdmin::removeExportedServiceFactory);
        cmp.build();

        //note adding void service dependencies is not supported for the dependency manager, using a service tracker instead.
        _remoteServiceTracker = ctx->trackAnyServices()
                .setFilter(std::string{"("}.append(celix::rsa::SERVICE_EXPORTED_INTERFACES).append("=*)"))
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