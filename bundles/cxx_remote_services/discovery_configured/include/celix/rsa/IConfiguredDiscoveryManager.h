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

namespace celix::rsa {
    /**
     * @brief Config property for the ',' seperated paths for  configured endpoints files.
     */
    constexpr const char *const CONFIGURED_DISCOVERY_DISCOVERY_FILES = "CELIX_RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILES";

    /**
     * @brief The IConfiguredDiscoveryManager interface.
     *
     * TODO document the expect json format.
     */
    class IConfiguredDiscoveryManager {
    public:
        virtual ~IConfiguredDiscoveryManager() noexcept = default;

        virtual void addConfiguredDiscoveryFile(const std::string& path) = 0;

        virtual void removeConfiguredDiscoveryFile(const std::string& path) = 0;
    };
}
