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

#include <ConfiguredDiscoveryManager.h>

#include <fstream>
#include <filesystem>

#include <ConfiguredEndpoint.h>
#include <celix_bundle_context.h>

#include <rapidjson/writer.h>

namespace celix::rsa {

constexpr const char* ENDPOINT_ARRAY = "endpoints";

std::optional<std::string> readFile(const std::string& path) {

    std::string contents;
    std::ifstream file(path);
    if(!file) {
        return {};
    }
    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], contents.size());
    file.close();
    return contents;
}

rapidjson::Document parseJSONFile(std::string& contents)  {

    rapidjson::Document resultDocument{};
    resultDocument.ParseInsitu(contents.data());
    return resultDocument;
}

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager) :
        _dependencyManager{std::move(dependencyManager)},
        _configurationFilePath{} {

    _configurationFilePath = celix_bundleContext_getProperty(
            _dependencyManager->bundleContext(),
            CELIX_ASYNC_RSA_CONFIGURED_DISCOVERY_FILE,
            "");

    discoverEndpoints();
}

void ConfiguredDiscoveryManager::discoverEndpoints() {

    auto contents = readFile(_configurationFilePath);
    if (contents) {
        auto parsedJson = parseJSONFile(contents.value());
        if (parsedJson.IsObject()) {
            if (parsedJson.HasMember(ENDPOINT_ARRAY) && parsedJson[ENDPOINT_ARRAY].IsArray()) {

                for (auto& endpoint : parsedJson[ENDPOINT_ARRAY].GetArray()) {
                    const auto& configuredEndpointPtr = std::make_shared<ConfiguredEndpoint>(endpoint.GetObject());
                    const auto endpointProperties = configuredEndpointPtr->getProperties();

                    std::cout << configuredEndpointPtr->ToString() << std::endl;

                    _dependencyManager->createComponent<Endpoint>(configuredEndpointPtr)
                                       .addInterface<Endpoint>("1.0.0", endpointProperties)
                                       .build();
                }
            }
        }
    }
}

} // end namespace celix::rsa.
