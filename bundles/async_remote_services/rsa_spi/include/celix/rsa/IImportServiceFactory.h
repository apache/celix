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
#pragma once

#include <memory>

namespace celix::rsa {

    /**
     * @brief ImportService class which represent a (opaque) imported service.
     * The lifetime of this object should be coupled with the lifetime of the imported service.
     */
    class ImportedService {
    public:
        virtual ~ImportedService() noexcept = default;

        /**
         * @brief returns the endpoint used for this imported service.
         */
        virtual const celix::rsa::Endpoint& getEndpoint() const = 0;
    };

    /**
     * @brief A import service factory for a specific service type.
     *
     * The service type which this import service factory targets is provided with
     * the mandatory celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME service property.
     *
     */
    class IImportServiceFactory {
    public:
        static constexpr std::string_view TARGET_SERVICE_NAME = "target.service.name";

        virtual ~IImportServiceFactory() noexcept = default;

        /**
         * @brief The service name for which this factory can created exported services.
         */
        std::string& const getTargetServiceName() const = 0;

        /**
         * @brief Imports the service identified with svcId
         * @param svcId The service id of the exported service.
         * @return A ImportService.
         * @throws celix::rsa::Exception if the import failed.
         */
        virtual std::unique_ptr<ImportedService> importService(celix::rsa::Endpoint& endpoint) = 0;
    };
}