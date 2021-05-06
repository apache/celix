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
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "celix/rsa/IEndpointAnnouncer.h"
#include "celix/BundleContext.h"
#include "celix/LogHelper.h"
#include "celix/rsa/EndpointDescription.h"
#include "celix/rsa/IConfiguredDiscoveryManager.h"

/** Path for configured endpoints file. */

namespace celix::rsa {

/**
 * The ConfiguredDiscoveryManager class is responsible for finding and announcing endpoints from
 * a local configuration JSON file.
 * This configured discovery manager announces local exported endpoints and imported endpoints from the JSON file.
 */
class ConfiguredDiscoveryManager final : public IConfiguredDiscoveryManager, public IEndpointAnnouncer {
public:
    /**
     *  Constructor for the ConfiguredDiscoveryManager.
     * @param dependencyManager shared_ptr to the context/container dependency manager.
     */
    explicit ConfiguredDiscoveryManager(std::shared_ptr<celix::BundleContext> ctx);

    ~ConfiguredDiscoveryManager() noexcept override = default;

    void announceEndpoint(std::unique_ptr<EndpointDescription> /*endpoint*/) override {/*nop*/}

    void revokeEndpoint(std::unique_ptr<EndpointDescription> /*endpoint*/) override {/*nop*/}

    void addConfiguredDiscoveryFile(const std::string& path) override;

    void removeConfiguredDiscoveryFile(const std::string& path) override;

    std::vector<std::string> getConfiguredDiscoveryFiles() const override;
private:
    celix::Properties convertToEndpointProperties(const rapidjson::Value &endpointJSON);
    void readConfiguredDiscoveryFiles();

    const std::shared_ptr<celix::BundleContext> ctx;
    const std::string configuredDiscoveryFiles;
    celix::LogHelper logHelper;

    mutable std::mutex mutex{}; //protects below
    std::unordered_map<std::string, std::vector<std::shared_ptr<celix::ServiceRegistration>>> endpointRegistrations{}; //key = configured discovery file path
};

} // end namespace celix::rsa.
