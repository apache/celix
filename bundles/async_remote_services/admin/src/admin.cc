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

celix::async_rsa::AsyncAdmin::AsyncAdmin(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept : _mng(mng) {
    std::cout << "[AsyncAdmin] AsyncAdmin" << std::endl;

}

void celix::async_rsa::AsyncAdmin::addEndpoint(celix::async_rsa::IEndpoint *endpoint, Properties &&properties) {
    std::cout << "[AsyncAdmin] addEndpoint" << std::endl;
    for(auto const &[key, value] : properties) {
        std::cout << "[AsyncAdmin] addEndpoint " << key << ", " << value << std::endl;
    }
    std::unique_lock l(_m);
    addEndpointInternal(endpoint, properties);
}

void celix::async_rsa::AsyncAdmin::removeEndpoint([[maybe_unused]] celix::async_rsa::IEndpoint *endpoint, [[maybe_unused]] Properties &&properties) {
    std::cout << "[AsyncAdmin] removeEndpoint" << std::endl;
    auto interfaceIt = properties.find("service.exported.interfaces");

    if(interfaceIt == end(properties)) {
        return;
    }

    std::unique_lock l(_m);

    _toBeCreatedImportedEndpoints.erase(std::remove_if(_toBeCreatedImportedEndpoints.begin(), _toBeCreatedImportedEndpoints.end(), [&interfaceIt](auto const &endpoint){
        auto endpointInterfaceIt = endpoint.second.find("service.exported.interfaces");
        return endpointInterfaceIt != end(endpoint.second) && endpointInterfaceIt->second == interfaceIt->second;
    }), _toBeCreatedImportedEndpoints.end());

    auto svcId = properties.find(OSGI_FRAMEWORK_SERVICE_ID);

    if(svcId == end(properties)) {
        return;
    }

    auto instanceIt = _serviceInstances.find(std::stol(svcId->second));

    if(instanceIt == end(_serviceInstances)) {
        return;
    }

    _mng->destroyComponent(instanceIt->second);
}

void celix::async_rsa::AsyncAdmin::addImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties &&properties) {
    std::cout << "[AsyncAdmin] addImportedServiceFactory" << std::endl;
    auto interfaceIt = properties.find("service.exported.interfaces");

    for(auto const &[key, value] : properties) {
        std::cout << "[AsyncAdmin] addImportedServiceFactory " << key << ", " << value << std::endl;
    }

    if(interfaceIt == end(properties)) {
        std::cout << "[AsyncAdmin] addImportedServiceFactory no interface" << std::endl;
        return;
    }

    std::unique_lock l(_m);

    auto existingFactory = _factories.find(interfaceIt->second);

    if(existingFactory != end(_factories)) {
        std::cout << "[AsyncAdmin] addImportedServiceFactory factory already exists" << std::endl;
        return;
    }

    _factories.emplace(interfaceIt->second, factory);

    for(auto it = _toBeCreatedImportedEndpoints.begin(); it != _toBeCreatedImportedEndpoints.end(); ) {
        auto interfaceToBeCreatedIt = it->second.find("service.exported.interfaces");

        for(auto const &[key, value] : it->second) {
            std::cout << "[AsyncAdmin] addImportedServiceFactory to be created endpoint " << key << ", " << value << std::endl;
        }

        if(interfaceToBeCreatedIt == end(it->second) || interfaceToBeCreatedIt->second != interfaceIt->second) {
            it++;
        } else {
            addEndpointInternal(it->first, it->second);
            it = _toBeCreatedImportedEndpoints.erase(it);
        }
    }
}

void celix::async_rsa::AsyncAdmin::removeImportedServiceFactory([[maybe_unused]] celix::async_rsa::IImportedServiceFactory *factory, Properties &&properties) {
    std::cout << "[AsyncAdmin] removeImportedServiceFactory" << std::endl;
    std::unique_lock l(_m);
    auto interfaceIt = properties.find("service.exported.interfaces");

    if(interfaceIt == end(properties)) {
        return;
    }

    _factories.erase(interfaceIt->second);
}

void celix::async_rsa::AsyncAdmin::addEndpointInternal(celix::async_rsa::IEndpoint *endpoint, Properties &properties) {
    std::cout << "[AsyncAdmin] addEndpointInternal" << std::endl;
    auto interfaceIt = properties.find("service.exported.interfaces");

    if(interfaceIt == end(properties)) {
        std::cout << "[AsyncAdmin] addEndpointInternal no interface" << std::endl;
        return;
    }

    auto svcId = properties.find(OSGI_FRAMEWORK_SERVICE_ID);

    if(svcId == end(properties)) {
        std::cout << "[AsyncAdmin] addEndpointInternal no svcid" << std::endl;
        return;
    }

    if(_subscribers.find(interfaceIt->second) == end(_subscribers)) {
        auto newSub = _subscribers.emplace(interfaceIt->second, std::make_pair(pubsub_subscriber_t{}, std::make_unique<subscriber_handle>(0, this, interfaceIt->second)));
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, std::string("async_rsa." + interfaceIt->second).c_str());
        newSub.first->second.first.handle = newSub.first->second.second.get();
        newSub.first->second.first.receive = [](void *handle, const char *msgType, unsigned int msgId, void *msg, const celix_properties_t *metadata, bool *) -> int {
            auto subHandle = static_cast<subscriber_handle*>(handle);
            return subHandle->admin->receiveMessage(subHandle->interface, msgType, msgId, msg, metadata);
        };
        static_cast<subscriber_handle*>(newSub.first->second.first.handle)->svcId = celix_bundleContext_registerService(_mng->bundleContext(), &newSub.first->second, PUBSUB_SUBSCRIBER_SERVICE_NAME, props);
    }

    auto existingFactory = _factories.find(interfaceIt->second);

    if(existingFactory == end(_factories)) {
        std::cout << "[AsyncAdmin] addEndpointInternal delaying creation" << std::endl;
        _toBeCreatedImportedEndpoints.emplace_back(std::make_pair(endpoint, properties));
        return;
    }

    // TODO remove copy of Properties
    std::cout << "[AsyncAdmin] addEndpointInternal created service" << std::endl;
    _serviceInstances.emplace(std::stol(svcId->second), existingFactory->second->create(_mng, Properties{properties}));
}

/// We exported a service and now receive a message (call that we should service)
/// \param msgId
/// \return
int celix::async_rsa::AsyncAdmin::receiveMessage(std::string_view , const char *, unsigned int msgId, void *, const celix_properties_t *) {
    std::cout << "[AsyncAdmin] receiveMessage" << std::endl;
    std::unique_lock l(_m);
    auto des = _serializers.find(msgId);

    if(des == end(_serializers)) {
        return -1;
    }

//    des->second->deserialize(des->second->handle, )

    return 0;
}

void celix::async_rsa::AsyncAdmin::addSerializer(pubsub_message_serialization_service_t *svc, const celix_properties_t *props) {
    std::cout << "[AsyncAdmin] addSerializer" << std::endl;
    std::unique_lock l(_m);
//    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    const char *msgFqn = celix_properties_get(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, NULL);
//    const char *version = celix_properties_get(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "0.0.0");
    long msgId = celix_properties_getAsLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 0L);

    if (msgId == 0) {
        msgId = celix_utils_stringHash(msgFqn);
    }

    _serializers.try_emplace(msgId, svc);
}

void celix::async_rsa::AsyncAdmin::removeSerializer(pubsub_message_serialization_service_t *, const celix_properties_t *props) {
    std::cout << "[AsyncAdmin] removeSerializer" << std::endl;
    std::unique_lock l(_m);
//    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    const char *msgFqn = celix_properties_get(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, NULL);
//    const char *version = celix_properties_get(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "0.0.0");
    long msgId = celix_properties_getAsLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 0L);

    if (msgId == 0) {
        msgId = celix_utils_stringHash(msgFqn);
    }

    _serializers.erase(msgId);
}

class AdminActivator {
public:
    explicit AdminActivator([[maybe_unused]] std::shared_ptr<celix::dm::DependencyManager> mng) : _cmp(mng->createComponent(std::make_unique<celix::async_rsa::AsyncAdmin>(mng))), _mng(mng) {
        _cmp.createServiceDependency<celix::async_rsa::IEndpoint>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addEndpoint, &celix::async_rsa::AsyncAdmin::removeEndpoint)
                .build();

        _cmp.createServiceDependency<celix::async_rsa::IImportedServiceFactory>()
                .setRequired(false)
                .setCallbacks(&celix::async_rsa::AsyncAdmin::addImportedServiceFactory, &celix::async_rsa::AsyncAdmin::removeImportedServiceFactory)
                .build();

        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        opts.callbackHandle = &_cmp.getInstance();
        opts.addWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            static_cast<celix::async_rsa::AsyncAdmin*>(handle)->addSerializer(static_cast<pubsub_message_serialization_service_t*>(svc), props);
        };
        opts.removeWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            static_cast<celix::async_rsa::AsyncAdmin*>(handle)->removeSerializer(static_cast<pubsub_message_serialization_service_t*>(svc), props);
        };
        _serializersTrackerId = celix_bundleContext_trackServicesWithOptions(mng->bundleContext(), &opts);

        _cmp.build();
    }

    ~AdminActivator() {
        celix_bundleContext_stopTracker(_mng->bundleContext(), _serializersTrackerId);
        _serializersTrackerId = -1;
    }

    AdminActivator(const AdminActivator &) = delete;
    AdminActivator &operator=(const AdminActivator &) = delete;
private:
    celix::dm::Component<celix::async_rsa::AsyncAdmin>& _cmp;
    std::shared_ptr<celix::dm::DependencyManager> _mng;
    long _serializersTrackerId{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(AdminActivator)
