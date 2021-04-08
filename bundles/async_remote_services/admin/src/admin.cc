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

#define L_DEBUG(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(_logger, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

celix::async_rsa::AsyncAdmin::AsyncAdmin(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept : _mng(mng), _logger(celix_logHelper_create(mng->bundleContext(), "celix_async_rsa_admin")) {

    // C++ version of tracking services without type not possible (yet?)
    celix_service_tracking_options_t opts{};
    opts.callbackHandle = this;
    opts.addWithProperties = [](void *handle, void *svc, const celix_properties_t *props){
        auto *admin = static_cast<AsyncAdmin*>(handle);
        admin->addService(svc, props);
    };
    opts.removeWithProperties = [](void *handle, void *svc, const celix_properties_t *props){
        auto *admin = static_cast<AsyncAdmin*>(handle);
        admin->removeService(svc, props);
    };
    _serviceTrackerId = celix_bundleContext_trackServicesWithOptions(_mng->bundleContext(), &opts);

    if(_serviceTrackerId < 0) {
        L_ERROR("couldn't register tracker");
    }
}

celix::async_rsa::AsyncAdmin::~AsyncAdmin() {
    celix_logHelper_destroy(_logger);
    celix_bundleContext_stopTracker(_mng->bundleContext(), _serviceTrackerId);
}

void celix::async_rsa::AsyncAdmin::addEndpoint(celix::rsa::Endpoint* endpoint, [[maybe_unused]] Properties &&properties) {
    std::unique_lock l(_m);
    addEndpointInternal(*endpoint);
}

void celix::async_rsa::AsyncAdmin::removeEndpoint([[maybe_unused]] celix::rsa::Endpoint* endpoint, [[maybe_unused]] Properties &&properties) {
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
        L_DEBUG("Removing endpoint but no exported interfaces");
        return;
    }

    std::unique_lock l(_m);

    _toBeCreatedImportedEndpoints.erase(std::remove_if(_toBeCreatedImportedEndpoints.begin(), _toBeCreatedImportedEndpoints.end(), [&interface](auto const &endpoint){
        auto endpointInterface = endpoint.getProperties().get(ENDPOINT_EXPORTS);
        return !endpointInterface.empty() && endpointInterface == interface;
    }), _toBeCreatedImportedEndpoints.end());

    auto svcId = properties.get(ENDPOINT_IDENTIFIER);

    if(svcId.empty()) {
        L_DEBUG("Removing endpoint but no service instance");
        return;
    }

    auto instanceIt = _importedServiceInstances.find(svcId);

    if(instanceIt == end(_importedServiceInstances)) {
        return;
    }

    _mng->destroyComponent(instanceIt->second);
    _importedServiceInstances.erase(instanceIt);
}

void celix::async_rsa::AsyncAdmin::addImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties &&properties) {
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
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

    for(auto it = _toBeCreatedImportedEndpoints.begin(); it != _toBeCreatedImportedEndpoints.end();) {
        auto tbceInterface = it->getProperties().get(ENDPOINT_EXPORTS);
        if(tbceInterface.empty() || tbceInterface != interface) {
            it++;
        } else {
            addEndpointInternal(*it);
            _toBeCreatedImportedEndpoints.erase(it);
        }
    }
}

void celix::async_rsa::AsyncAdmin::removeImportedServiceFactory([[maybe_unused]] celix::async_rsa::IImportedServiceFactory *factory, Properties &&properties) {
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
        L_WARN("Removing service factory but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);
    _importedServiceFactories.erase(interface);
}

void celix::async_rsa::AsyncAdmin::addExportedServiceFactory(celix::async_rsa::IExportedServiceFactory *factory, Properties&& properties) {
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
        L_WARN("Adding exported factory but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);
    auto factoryIt = _exportedServiceFactories.find(interface);

    if(factoryIt != end(_exportedServiceFactories)) {
        L_WARN("Adding exported factory but factory already exists");
        return;
    }

    _exportedServiceFactories.emplace(interface, factory);
    L_WARN("Looping over %i tbce for interface %s", _toBeCreatedExportedEndpoints.size(), interface.c_str());

    for(auto it = _toBeCreatedExportedEndpoints.begin(); it != _toBeCreatedExportedEndpoints.end(); ) {
        auto interfaceToBeCreated = it->second.get(ENDPOINT_EXPORTS);
        L_WARN("Checking tbce interface %s", interfaceToBeCreated.c_str());

        if(interfaceToBeCreated.empty() || interfaceToBeCreated != interface) {
            it++;
        } else {
            auto endpointId = it->second.get(ENDPOINT_IDENTIFIER);
            _exportedServiceInstances.emplace(endpointId, factory->create(it->first, endpointId));
            it = _toBeCreatedExportedEndpoints.erase(it);
        }
    }
}

void celix::async_rsa::AsyncAdmin::removeExportedServiceFactory(celix::async_rsa::IExportedServiceFactory *, Properties&& properties) {
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
        L_WARN("Removing imported factory but missing exported interfaces");
        return;
    }

    std::unique_lock l(_m);
    _exportedServiceFactories.erase(interface);
}

void celix::async_rsa::AsyncAdmin::addService(void *svc, const celix_properties_t *props) {
    auto *objectClass = celix_properties_get(props, OSGI_FRAMEWORK_OBJECTCLASS, nullptr);
    auto *remote = celix_properties_get(props, "remote", nullptr);
    auto endpointId = celix_properties_get(props, ENDPOINT_IDENTIFIER, nullptr);

    if(objectClass == nullptr) {
        L_WARN("Adding service to be exported but missing objectclass");
        return;
    }

    if(remote == nullptr) {
        // not every service is remote, this is fine but not something the RSA admin is interested in.
        return;
    } else {
        L_WARN("found remote service %s", objectClass);
    }

    if(endpointId == nullptr) {
        L_WARN("Adding service to be exported but missing endpoint.id");
        return;
    }

    std::unique_lock l(_m);
    auto factory = _exportedServiceFactories.find(objectClass);

    if(factory == end(_exportedServiceFactories)) {
        L_WARN("Adding service to be exported but no factory available yet, delaying creation");

        Properties cppProps;

        auto it = celix_propertiesIterator_construct(props);
        const char* key;
        while(key = celix_propertiesIterator_nextKey(&it), key != nullptr) {
            cppProps.set(key, celix_properties_get(props, key, nullptr));
        }

        _toBeCreatedExportedEndpoints.emplace_back(std::make_pair(svc, std::move(cppProps)));
        return;
    }

    _exportedServiceInstances.emplace(endpointId, factory->second->create(svc, endpointId));
}

void celix::async_rsa::AsyncAdmin::removeService(void *, const celix_properties_t *props) {
    auto *objectClass = celix_properties_get(props, OSGI_FRAMEWORK_OBJECTCLASS, nullptr);
    auto *remote = celix_properties_get(props, "remote", nullptr);
    auto svcId = celix_properties_get(props, ENDPOINT_IDENTIFIER, nullptr);

    if(objectClass == nullptr) {
        L_WARN("Removing service to be exported but missing objectclass");
        return;
    }

    if(remote == nullptr) {
        // not every service is remote, this is fine but not something the RSA admin is interested in.
        return;
    }

    if(svcId == nullptr) {
        L_WARN("Removing service to be exported but missing endpoint.id");
        return;
    }

    std::unique_lock l(_m);
    auto instanceIt = _exportedServiceInstances.find(svcId);

    if(instanceIt == end(_exportedServiceInstances)) {
        return;
    }

    _mng->destroyComponent(instanceIt->second);
    _exportedServiceInstances.erase(instanceIt);
}

void celix::async_rsa::AsyncAdmin::addEndpointInternal(celix::rsa::Endpoint& endpoint) {

    const auto& properties = endpoint.getProperties();
    auto interface = properties.get(ENDPOINT_EXPORTS);

    if(interface.empty()) {
        L_WARN("Adding endpoint but missing exported interfaces");
        return;
    }

    auto endpointId = properties.get(ENDPOINT_IDENTIFIER);

    if(endpointId.empty()) {
        L_WARN("Adding endpoint but missing service id");
        return;
    }

    auto existingFactory = _importedServiceFactories.find(interface);

    if(existingFactory == end(_importedServiceFactories)) {
        L_DEBUG("Adding endpoint but no factory available yet, delaying creation");
        _toBeCreatedImportedEndpoints.emplace_back(celix::rsa::Endpoint{properties});
        return;
    }

    L_DEBUG("Adding endpoint, created service");
    _importedServiceInstances.emplace(endpointId, existingFactory->second->create(endpointId));
}

class AdminActivator {
public:
    explicit AdminActivator(std::shared_ptr<celix::dm::DependencyManager> mng) :
            _cmp(mng->createComponent(std::make_unique<celix::async_rsa::AsyncAdmin>(mng))), _mng(std::move(mng)) {

        _cmp.createServiceDependency<celix::rsa::Endpoint>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addEndpoint, &celix::async_rsa::AsyncAdmin::removeEndpoint)
                .build();

        _cmp.createServiceDependency<celix::async_rsa::IImportedServiceFactory>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addImportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeImportedServiceFactory)
                .build();

        _cmp.createServiceDependency<celix::async_rsa::IExportedServiceFactory>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addExportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeExportedServiceFactory)
                .build();

        _cmp.build();
    }

    ~AdminActivator() noexcept {
        _mng->destroyComponent(_cmp);
    }

    AdminActivator(const AdminActivator &) = delete;
    AdminActivator &operator=(const AdminActivator &) = delete;
private:
    celix::dm::Component<celix::async_rsa::AsyncAdmin>& _cmp;
    std::shared_ptr<celix::dm::DependencyManager> _mng;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(AdminActivator)
