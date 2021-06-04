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

#include "celix/rsa/IExportRegistration.h"

namespace celix::rsa {

    /**
     * @brief A export service factory for a specific service type.
     *
     * the mandatory service properties:
     *  - celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE
     *
     */
    class IExportServiceFactory {
    public:
        /**
         * @brief The service name for which this factory can created exported services.
         */
        static constexpr const char * const REMOTE_SERVICE_TYPE = "remote.service.type";

        virtual ~IExportServiceFactory() noexcept = default;

        /**
         * @brief The service name for which this factory can export services as remote services.
         *
         * The value should be based on the "remote.service.type" service property of
         * this import service factory.
         */
        [[nodiscard]] virtual const std::string& getRemoteServiceType() const = 0;

        /**
         * @Brief The supported intents for this export service factory.
         */
        [[nodiscard]] virtual const std::vector<std::string>& getSupportedIntents() const = 0;

        /**
         * @Brief The supported configs for this export service factory.
         */
        [[nodiscard]] virtual const std::vector<std::string>& getSupportedConfigs() const = 0;

        /**
         * @brief Exports the service associated with the provided serviceProperties.
         * @param serviceProperties The service properties of the to be exported service.
         * @return A new export registration.
         * @throws celix::rsa::RemoteServicesException if the export failed.
         */
        [[nodiscard]] virtual std::unique_ptr<celix::rsa::IExportRegistration> exportService(const celix::Properties& serviceProperties) = 0;
    };
}