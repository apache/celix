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
#include <vector>

#include <IEndpoint.h>
#include <celix_api.h>

#include <IEndpointEventListener.h>

namespace celix::async_rsa::discovery {

/**
 * The ConfiguredDiscoveryManager class is responsible for finding and announcing endpoints from
 * a local configuration JSON file. This discovery manager announces local exported endpoints and imported endpoints from the JSON file.
 */
class ConfiguredDiscoveryManager {
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
     * Defaulted destructor.
     */
    ~ConfiguredDiscoveryManager() = default;

    /**
     * Defaulted copy-constructor.
     */
    ConfiguredDiscoveryManager(const ConfiguredDiscoveryManager&) = default;

    /**
     * Defaulted move-constructor.
     */
    ConfiguredDiscoveryManager(ConfiguredDiscoveryManager&&) = default;

    /**
     * Defaulted assignment-operator.
     */
    ConfiguredDiscoveryManager& operator=(const ConfiguredDiscoveryManager&) = default;

    /**
     * Add a endpoint event listener that will be updated by this discovery-manager whenever a new endpoint event is happening.
     * @param endpointEventListener The new endpoint event listener objectref.
     */
    void addEndpointEventListener(const std::shared_ptr<IEndpointEventListener>& endpointEventListener);

private:

    void updateListeners(const std::shared_ptr<IEndpoint>& endpoint);

    std::shared_ptr<DependencyManager> _dependencyManager;
    std::vector<std::shared_ptr<IEndpointEventListener>> _endpointEventListeners;
};

} // end namespace celix::async_rsa::discovery.
