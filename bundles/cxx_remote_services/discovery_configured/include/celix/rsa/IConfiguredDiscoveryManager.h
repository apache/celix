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

#include <vector>
#include <string>

namespace celix::rsa {
    /**
     * @brief Config property for the configured endpoints files.
     *
     * Configured files will be read by the configured discovery manager during startup and the file will not
     * be monitored for updates.
     * Should be a string and can contain multiple entries ',' separated.
     */
    constexpr const char *const CONFIGURED_DISCOVERY_DISCOVERY_FILES = "CELIX_RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILES";

    /**
     * @brief The IConfiguredDiscoveryManager interface.
     *
     * Expected configured endpoint json format:
     *      {
     *          "endpoints": [
     *              {
     *                  "endpoint.id": "id-01",
     *                  "service.imported": true,
     *                  "service.imported.configs": ["pubsub"],
     *                  "service.exported.interfaces": "IHardcodedService",
     *                  "endpoint.objectClass": "TestComponentName",
     *                  "endpoint.anykey" : "anyval"
     *              }
     *          ]
     *      }
     */
    class IConfiguredDiscoveryManager {
    public:
        virtual ~IConfiguredDiscoveryManager() noexcept = default;

        /**
         * @brief Adds a configured discovery file to the discovery manager.
         * @param path Path to a discovery file
         * @throws celix::rsa::RemoteException if the path is incorrect or if the file format is invalid.
         */
        virtual void addConfiguredDiscoveryFile(const std::string& path) = 0;

        /**
         * @brief Removes a configured discovery file to the discovery manager.
         * @param path Path to a discovery file
         */
        virtual void removeConfiguredDiscoveryFile(const std::string& path) = 0;

        /**
         * @brief List the (valid) configured discovery files known to this configured discovery manager.
         */
        virtual std::vector<std::string> getConfiguredDiscoveryFiles() const = 0;
    };
}
