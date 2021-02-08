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

#include <ConfiguredEndpoint.h>

#include <rapidjson/writer.h>
#include <rapidjson/filereadstream.h>

namespace celix::async_rsa::discovery {

constexpr const char* ENDPOINT_ARRAY = "endpoints";

rapidjson::Document parseJSONFile(const std::string& filePath)  {

    rapidjson::Document resultDocument{};
    FILE* filePtr = fopen(filePath.c_str(), "r");
    char readBuffer[65536];
    auto fileStream = rapidjson::FileReadStream(filePtr, readBuffer, sizeof(readBuffer));
    resultDocument.ParseStream(fileStream);
    return resultDocument;
}

celix::dm::Properties convertEndpointPropertiesToCelix(const ConfiguredEndpointProperties& endpointProperties) {

    return celix::dm::Properties{{"service.imported", std::to_string(endpointProperties.isImported()).c_str()},
                                 {"service.exported.interfaces", endpointProperties.getExports()},
                                 {"endpoint.id", endpointProperties.getId()}};
}

ConfiguredEndpointProperties convertCelixPropertiesToEndpoint(const celix::dm::Properties& celixProperties) {

    auto endpointId = celixProperties.at("endpoint.id");
    auto exports = celixProperties.at("service.exported.interfaces");
    auto imported = celixProperties.at("service.imported");

    return ConfiguredEndpointProperties{endpointId,
                                        (std::strcmp(imported.c_str(), "true") == 0),
                                        {}, exports, {}, "", ""};
}

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager,
                                                       std::string configurationFilePath) :
        _dependencyManager{std::move(dependencyManager)},
        _configurationFilePath{std::move(configurationFilePath)},
        _endpoints{},
        _publishedEndpoints{} {
}

void ConfiguredDiscoveryManager::discoverEndpoints() {

    const rapidjson::Document& parsedJson = parseJSONFile(_configurationFilePath);

    if (parsedJson.IsObject()) {
        if (parsedJson.HasMember(ENDPOINT_ARRAY)) {

            const rapidjson::Value& endpointJsonArray = parsedJson[ENDPOINT_ARRAY];
            if (endpointJsonArray.IsArray() && ( endpointJsonArray.Size() > 0 )) {

                for (rapidjson::Value::ConstValueIterator endpointIter = endpointJsonArray.Begin();
                     endpointIter != endpointJsonArray.End(); endpointIter++) {

                    if (endpointIter->IsObject()) {
                        const auto& endpointJson = endpointIter->GetObject();
                        _endpoints.emplace_back(std::make_shared<ConfiguredEndpoint>(endpointJson));
                    }
                }
            }
        }
    }
    publishParsedEndpoints();
}

void ConfiguredDiscoveryManager::publishParsedEndpoints() {

    for (const auto& endpoint : _endpoints) {

        _publishedEndpoints.emplace_back(
                &_dependencyManager->createComponent<IEndpoint>().addInterface<IEndpoint>(
                        "1.0.0",
                        convertEndpointPropertiesToCelix(endpoint->getProperties()))
                        .build().getInstance()
        );

        std::cout << endpoint->ToString() << std::endl;
    }
}

void ConfiguredDiscoveryManager::addExportedEndpoint(IEndpoint* /*endpoint*/, celix::dm::Properties&& properties) {

    auto endpoint = std::make_shared<ConfiguredEndpoint>(convertCelixPropertiesToEndpoint(properties));
    auto parsedJson = parseJSONFile(_configurationFilePath);
    auto endpointJSON = endpoint->exportToJSON(parsedJson);

    if (parsedJson.IsObject()) {
        if (parsedJson.HasMember(ENDPOINT_ARRAY)) {
            rapidjson::Value& endpointJsonArray = parsedJson[ENDPOINT_ARRAY];
            if (endpointJsonArray.IsArray()){
                endpointJsonArray.PushBack(endpointJSON, parsedJson.GetAllocator());
            }
        }
    }

}

void ConfiguredDiscoveryManager::removeExportedEndpoint(IEndpoint* /*endpoint*/, celix::dm::Properties&& /*properties*/) { /* not used */ }

} // end namespace celix::async_rsa::discovery.
