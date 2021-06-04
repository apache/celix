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

#include "celix/rsa/EndpointDescription.h"
#include "celix/rsa/IImportRegistration.h"

namespace celix::rsa {

    /**
     * @brief A import service factory for a specific service type.
     *
     * The service type which this export service factory targets is provided with `REMOTE_SERVICE_TYPE` and
     * the supported configs with `REMOTE_CONFIGS_SUPPORTED`.
     *
     */
    class IImportServiceFactory {
    public:
        /**
         * @brief The service name for which this factory can import remote services.
         */
        static constexpr const char * const REMOTE_SERVICE_TYPE = "remote.service.type";

        virtual ~IImportServiceFactory() noexcept = default;

        /**
         * @brief The service name for which this factory can import remote services.
         *
         * The value should be based on the "remote.service.type" service property of
         * this import service factory.
         */
        [[nodiscard]] virtual const std::string& getRemoteServiceType() const = 0;

        /**
         * @Brief The supported configs for this import service factory.
         */
        [[nodiscard]] virtual const std::vector<std::string>& getSupportedConfigs() const = 0;

        /**
         * @brief Imports a service for the provided remote service endpoint description.
         * @param endpoint The endpoint description describing the remote service.
         * @return A new import registration.
         * @throws celix::rsa::RemoteServicesException if the import failed.
         */
        [[nodiscard]] virtual std::unique_ptr<celix::rsa::IImportRegistration> importService(const celix::rsa::EndpointDescription& endpoint) = 0;
    };
}