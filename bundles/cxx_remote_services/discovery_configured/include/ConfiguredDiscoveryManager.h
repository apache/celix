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

#include "celix/rsa/IEndpointAnnouncer.h"

#include <memory>
#include <vector>
#include <string>

#include "celix/rsa/Endpoint.h"
#include <celix_api.h>

#include <ConfiguredEndpoint.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

/** Path for configured endpoint file. */
#define CELIX_ASYNC_RSA_CONFIGURED_DISCOVERY_FILE "CELIX_ASYNC_RSA_CONFIGURED_DISCOVERY_FILE"

namespace celix::rsa {

/**
 * The ConfiguredDiscoveryManager class is responsible for finding and announcing endpoints from
 * a local configuration JSON file.
 * This configured discovery manager announces local exported endpoints and imported endpoints from the JSON file.
 */
class ConfiguredDiscoveryManager final : public IEndpointAnnouncer {
public:

    /**
     * Deleted default constructor, dependencyManager parameter is required.
     */
    ConfiguredDiscoveryManager() = delete;

    /**
     *  Constructor for the ConfiguredDiscoveryManager.
     * @param dependencyManager shared_ptr to the context/container dependency manager.
     */
    explicit ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager);

    /**
     * Deleted copy-constructor, since rapidjson members have no copy-constructor.
     */
    ConfiguredDiscoveryManager(const ConfiguredDiscoveryManager&) = delete;

    /**
     * Defaulted move-constructor.
     */
    ConfiguredDiscoveryManager(ConfiguredDiscoveryManager&&) = default;

    /**
     * Deleted assignment-operator, since rapidjson members have no copy-constructor.
     */
    ConfiguredDiscoveryManager& operator=(const ConfiguredDiscoveryManager&) = delete;

    void announceEndpoint(std::unique_ptr<Endpoint> /*endpoint*/) override {/*nop*/}

    void revokeEndpoint(std::unique_ptr<Endpoint> /*endpoint*/) override {/*nop*/}

private:

    void discoverEndpoints();

    std::shared_ptr<DependencyManager> _dependencyManager;
    std::string _configurationFilePath;
};

} // end namespace celix::rsa.
