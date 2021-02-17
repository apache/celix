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

#include <endpoint.h>
#include <celix_api.h>
#include <ImportedServiceFactory.h>
#include <mutex>
#include <pubsub/api.h>
#include <pubsub_message_serialization_service.h>
#include <memory_resource>

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
        ~AsyncAdmin();

        AsyncAdmin(AsyncAdmin const &) = delete;
        AsyncAdmin(AsyncAdmin&&) = delete;
        AsyncAdmin& operator=(AsyncAdmin const &) = delete;
        AsyncAdmin& operator=(AsyncAdmin&&) = delete;

        // Imported endpoint add/remove functions
        void addEndpoint(celix::rsa::IEndpoint *endpoint, Properties&& properties);
        void removeEndpoint(celix::rsa::IEndpoint *endpoint, Properties&& properties);

        // Imported endpoint add/remove functions
        void addImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);
        void removeImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);


    private:
        std::shared_ptr<celix::dm::DependencyManager> _mng{};
        celix_log_helper_t *_logger;
        std::mutex _m{}; // protects below
        std::pmr::unsynchronized_pool_resource _memResource{};
        std::pmr::unordered_map<std::string, celix::async_rsa::IImportedServiceFactory*> _factories{&_memResource};
        std::pmr::unordered_map<long, celix::dm::BaseComponent&> _serviceInstances{&_memResource};
        std::vector<std::pair<celix::rsa::IEndpoint*, Properties>> _toBeCreatedImportedEndpoints{};

        // Internal functions for code re-use
        void addEndpointInternal(celix::rsa::IEndpoint *endpoint, Properties&& properties);
    };
}