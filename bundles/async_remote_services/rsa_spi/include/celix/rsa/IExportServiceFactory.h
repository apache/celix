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
     * @brief IExportServiceGuard class which represent a (opaque) exported service.
     * If lifetime of this object expires it should remove the underlining exported service.
     * */
    class IExportServiceGuard {
    public:
        virtual ~IExportServiceGuard() noexcept = default;
    };

    /**
     * @brief A export service factory for a specific service type.
     *
     * The service type which this export service factory targets is provided with
     * the mandatory celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME service property.
     *
     */
    class IExportServiceFactory {
    public:
        /**
         * @brief The service name for which this factory can created exported services.
         */
        static constexpr const char * const TARGET_SERVICE_NAME = "target.service.name";

        virtual ~IExportServiceFactory() noexcept = default;

        /**
         * @brief Exports the service identified with svcId
         * @param svcId The service id of the exported service.
         * @return A ExportService.
         * @throws celix::rsa::RemoteServicesException if the export failed.
         */
        virtual std::unique_ptr<celix::rsa::IExportServiceGuard> exportService(const celix::Properties& serviceProperties) = 0;
    };
}