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

//#include <discovery.h>
#include <IEndpoint.h>
#include <celix_api.h>
#include <ImportedServiceFactory.h>
#include <mutex>
#include <pubsub/api.h>
#include <pubsub_message_serialization_service.h>

namespace celix::async_rsa {
    class AsyncAdmin;

    struct subscriber_handle {
        subscriber_handle(long _svcId, AsyncAdmin *_admin, std::string_view _interface) : svcId(_svcId), admin(_admin), interface(_interface) {}
        subscriber_handle(subscriber_handle const&) = default;
        subscriber_handle(subscriber_handle&&) = default;
        subscriber_handle& operator=(subscriber_handle const&) = default;
        subscriber_handle& operator=(subscriber_handle&&) = default;
        ~subscriber_handle() = default;

        long svcId;
        AsyncAdmin *admin;
        std::string interface;
    };

    class AsyncAdmin {
    public:
        explicit AsyncAdmin(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept;
        ~AsyncAdmin() = default;

        // Imported endpoint add/remove functions
        void addEndpoint(celix::async_rsa::IEndpoint *endpoint, Properties&& properties);
        void removeEndpoint(celix::async_rsa::IEndpoint *endpoint, Properties&& properties);

        // Imported endpoint add/remove functions
        void addImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);
        void removeImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);

        void addSerializer(pubsub_message_serialization_service_t *svc, const celix_properties_t *props);
        void removeSerializer(pubsub_message_serialization_service_t *svc, const celix_properties_t *props);
//
        int receiveMessage(std::string_view interface, const char *msgType, unsigned int msgId, void *msg, const celix_properties_t *metadata);

    private:
        std::shared_ptr<celix::dm::DependencyManager> _mng{};
        std::unordered_map<std::string, celix::async_rsa::IImportedServiceFactory*> _factories{};
        std::unordered_map<std::string, std::pair<pubsub_subscriber_t, std::unique_ptr<subscriber_handle>>> _subscribers{};
        std::unordered_map<long, celix::dm::BaseComponent&> _serviceInstances{};
        std::unordered_map<long, pubsub_message_serialization_service*> _serializers{};
        std::vector<std::pair<celix::async_rsa::IEndpoint*, Properties>> _toBeCreatedImportedEndpoints{};
        std::mutex _m{};

        // Internal functions for code re-use
        void addEndpointInternal(celix::async_rsa::IEndpoint *endpoint, Properties& properties);
    };
}