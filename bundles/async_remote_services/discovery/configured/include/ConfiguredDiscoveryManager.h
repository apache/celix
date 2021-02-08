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

#include <IDiscoveryManager.h>

#include <memory>
#include <vector>
#include <string>

#include <IEndpoint.h>
#include <celix_api.h>

#include <ConfiguredEndpoint.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

namespace celix::async_rsa::discovery {

/**
 * The ConfiguredDiscoveryManager class is responsible for finding and announcing endpoints from
 * a local configuration JSON file. This discovery manager announces local exported endpoints and imported endpoints from the JSON file.
 */
class ConfiguredDiscoveryManager final : public IDiscoveryManager {
public:

    /**
     * Deleted default constructor, dependencyManager parameter is required.
     */
    ConfiguredDiscoveryManager() = delete;

    /**
     *  Constructor for the ConfiguredDiscoveryManager.
     * @param dependencyManager shared_ptr to the context/container dependency manager.
     * @param configurationFilePath string path indicating location of the configuration file.
     */
    ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager,
                               std::string configurationFilePath);

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

    /**
     * @see IDiscoveryManager::discoverEndpoints.
     */
    void discoverEndpoints() override;

    void addExportedEndpoint(IEndpoint *endpoint, celix::dm::Properties&& properties) override;
    void removeExportedEndpoint(IEndpoint *endpoint, celix::dm::Properties&& properties) override;

private:

    void publishParsedEndpoints();

    std::shared_ptr<DependencyManager> _dependencyManager;

    const std::string _configurationFilePath;

    std::vector<std::shared_ptr<ConfiguredEndpoint>> _endpoints;
    std::vector<IEndpoint*> _publishedEndpoints;
};

} // end namespace celix::async_rsa::discovery.
