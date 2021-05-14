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

#include "celix/rsa/IEndpointAnnouncer.h"
#include <IExportedService.h>
#include <celix_api.h>
#include <mutex>
#include <celix_log_helper.h>

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_memory_resource
#include <memory_resource>
#endif

namespace celix::async_rsa {
        class AsyncTopologyManager {
        public:
            explicit AsyncTopologyManager(celix_log_helper_t *logger) noexcept;
            ~AsyncTopologyManager() ;

            AsyncTopologyManager(AsyncTopologyManager const &) = delete;
            AsyncTopologyManager(AsyncTopologyManager&&) = delete;
            AsyncTopologyManager& operator=(AsyncTopologyManager const &) = delete;
            AsyncTopologyManager& operator=(AsyncTopologyManager&&) = delete;

            // Imported endpoint add/remove functions
            void addExportedService(celix::async_rsa::IExportedService *endpoint, Properties&& properties);
            void removeExportedService(celix::async_rsa::IExportedService *endpoint, Properties&& properties);

            void setDiscovery(celix::rsa::IEndpointAnnouncer *discovery);

        private:
            celix_log_helper_t *_logger;
            std::mutex _m{}; // protects below

#if __cpp_lib_memory_resource
            std::pmr::unsynchronized_pool_resource _memResource{};
            std::pmr::unordered_map<std::string, celix::async_rsa::IExportedService*> _exportedServices{&_memResource};
#else
            std::unordered_map<std::string, celix::async_rsa::IExportedService*> _exportedServices{};
#endif
            celix::rsa::IEndpointAnnouncer *_discovery{};
        };
}