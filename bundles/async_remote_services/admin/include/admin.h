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

#include <Endpoint.h>
#include <celix_api.h>
#include <ImportedServiceFactory.h>
#include <ExportedServiceFactory.h>
#include <mutex>
#include <pubsub/api.h>
#include <pubsub_message_serialization_service.h>

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_memory_resource
#include <memory_resource>
#endif

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
        void addEndpoint(celix::rsa::Endpoint* endpoint, Properties &&properties);
        void removeEndpoint(celix::rsa::Endpoint* endpoint, Properties &&properties);

        // Imported endpoint add/remove functions
        void addImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);
        void removeImportedServiceFactory(celix::async_rsa::IImportedServiceFactory *factory, Properties&& properties);

        // Exported endpoint add/remove functions
        void addExportedServiceFactory(celix::async_rsa::IExportedServiceFactory *factory, Properties&& properties);
        void removeExportedServiceFactory(celix::async_rsa::IExportedServiceFactory *factory, Properties&& properties);

        // Add/remove services, used to track all services registered with celix and check if remote = true.
        void addService(void *svc, const celix_properties_t *props);
        void removeService(void *svc, const celix_properties_t *props);


    private:
        std::shared_ptr<celix::dm::DependencyManager> _mng{};
        celix_log_helper_t *_logger;
        long _serviceTrackerId{-1};
        std::mutex _m{}; // protects below

#if __cpp_lib_memory_resource
        std::pmr::unsynchronized_pool_resource _memResource{};
        std::pmr::unordered_map<std::string, celix::async_rsa::IExportedServiceFactory*> _exportedServiceFactories{&_memResource};
        std::pmr::unordered_map<std::string, celix::async_rsa::IImportedServiceFactory*> _importedServiceFactories{&_memResource};
        std::pmr::unordered_map<std::string, celix::dm::BaseComponent&> _importedServiceInstances{&_memResource};
        std::pmr::unordered_map<std::string, celix::dm::BaseComponent&> _exportedServiceInstances{&_memResource};
#else
        std::unordered_map<std::string, celix::async_rsa::IExportedServiceFactory*> _exportedServiceFactories{};
        std::unordered_map<std::string, celix::async_rsa::IImportedServiceFactory*> _importedServiceFactories{};
        std::unordered_map<std::string, celix::dm::BaseComponent&> _importedServiceInstances{};
        std::unordered_map<std::string, celix::dm::BaseComponent&> _exportedServiceInstances{};
#endif
        std::vector<celix::rsa::Endpoint> _toBeCreatedImportedEndpoints{};
        std::vector<std::pair<void*, Properties>> _toBeCreatedExportedEndpoints{};

        // Internal functions for code re-use
        void addEndpointInternal(celix::rsa::Endpoint& endpoint);
    };
}