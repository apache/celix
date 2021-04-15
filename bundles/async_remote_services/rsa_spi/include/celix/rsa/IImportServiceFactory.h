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
#include "celix/rsa/Endpoint.h"

namespace celix::rsa {

    /**
     * @brief IImportServiceGuard class which represent a (opaque) imported service.
     * If lifetime of this object expires it should remove the underlining imported service.
     */
    class IImportServiceGuard {
    public:
        virtual ~IImportServiceGuard() noexcept = default;
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
        /**
         * @brief The service name for which this factory can created exported services.
         */
        static constexpr const char * const TARGET_SERVICE_NAME = "target.service.name";

        virtual ~IImportServiceFactory() noexcept = default;

        /**
         * @brief Imports the service identified with svcId
         * @param svcId The service id of the exported service.
         * @return A ImportService.
         * @throws celix::rsa::RemoteServicesException if the import failed.
         */
        virtual std::unique_ptr<celix::rsa::IImportServiceGuard> importService(const celix::rsa::Endpoint& endpoint) = 0;
    };
}