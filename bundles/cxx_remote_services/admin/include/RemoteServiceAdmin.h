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

#include "celix/rsa/Endpoint.h"
#include <celix_api.h>
#include <mutex>
#include "celix_log_service.h"

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_memory_resource
#include <memory_resource>
#include <celix/rsa/IImportServiceFactory.h>
#include <celix/rsa/IExportServiceFactory.h>

#endif

namespace celix::rsa {

    class RemoteServiceAdmin {
    public:
        // Imported endpoint add/remove functions
        void addEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint);
        void removeEndpoint(const std::shared_ptr<celix::rsa::Endpoint>& endpoint);

        // import service factory. used to create new imported services
        void addImportedServiceFactory(const std::shared_ptr<celix::rsa::IImportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);
        void removeImportedServiceFactory(const std::shared_ptr<celix::rsa::IImportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);

        // export service factory. used to create new exported services
        void addExportedServiceFactory(const std::shared_ptr<celix::rsa::IExportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);
        void removeExportedServiceFactory(const std::shared_ptr<celix::rsa::IExportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);

        // Add/remove services, used to track all services registered with celix and check if remote = true.
        void addService(const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties);
        void removeService(const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties);

        // Set logging service
        void setLogger(const std::shared_ptr<celix_log_service>& logService);
    private:
        void createExportServices();
        void createImportServices();

        std::shared_ptr<celix_log_service> _logService{};
        std::mutex _m{}; // protects below

#if __cpp_lib_memory_resource
        std::pmr::unsynchronized_pool_resource _memResource{};

        std::pmr::unordered_map<std::string, std::shared_ptr<celix::rsa::IExportServiceFactory>> _exportServiceFactories{&_memResource}; //key = service name
        std::pmr::unordered_map<std::string, std::shared_ptr<celix::rsa::IImportServiceFactory>> _importServiceFactories{&_memResource}; //key = service name
        std::pmr::unordered_map<std::string, std::unique_ptr<celix::rsa::IImportServiceGuard>> _importedServices{&_memResource}; //key = endpoint id
        std::pmr::unordered_map<long, std::unique_ptr<celix::rsa::IExportServiceGuard>> _exportedServices{&_memResource}; //key = service id
#else
        std::unordered_map<std::string, std::shared_ptr<celix::rsa::IExportServiceFactory>> _exportServiceFactories{}; //key = service name
        std::unordered_map<std::string, std::shared_ptr<celix::rsa::IImportServiceFactory>> _importServiceFactories{}; //key = service name
        std::unordered_map<std::string, std::unique_ptr<celix::rsa::IImportServiceGuard>> _importedServices{}; //key = endpoint id
        std::unordered_map<long, std::unique_ptr<celix::rsa::IExportServiceGuard>> _exportedServices{}; //key = service id
#endif
        std::vector<std::shared_ptr<celix::rsa::Endpoint>> _toBeImportedServices{};
        std::vector<std::shared_ptr<const celix::Properties>> _toBeExportedServices{};
    };
}