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

#include <ConfiguredEndpoint.h>

#include <rapidjson/writer.h>
#include <rapidjson/filereadstream.h>

namespace celix::async_rsa::discovery {

constexpr const char* ENDPOINT_ARRAY = "endpoints";

std::optional<std::string> read_whole_file(const std::string& path) {

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
                                        (imported == "true"),
                                        {}, exports, {}, "", ""};
}

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager,
                                                       std::string configurationFilePath) :
        _dependencyManager{std::move(dependencyManager)},
        _configurationFilePath{std::move(configurationFilePath)},
        _discoveredEndpoints{},
        _publishedDiscoveredEndpoints{} {
}

void ConfiguredDiscoveryManager::discoverEndpoints() {

    auto contents = read_whole_file(_configurationFilePath);
    auto parsedJson = parseJSONFile(contents.value());
    if (parsedJson.IsObject()) {
        if(parsedJson.HasMember(ENDPOINT_ARRAY) && parsedJson[ENDPOINT_ARRAY].IsArray()) {
            for(auto& endpoint : parsedJson[ENDPOINT_ARRAY].GetArray()) {
                _discoveredEndpoints.emplace_back(std::make_shared<ConfiguredEndpoint>(endpoint.GetObject()));
            }
        }
    }
    publishParsedEndpoints();
}

void ConfiguredDiscoveryManager::publishParsedEndpoints() {

    for (const auto& endpoint : _discoveredEndpoints) {
        _publishedDiscoveredEndpoints.emplace_back(
                &_dependencyManager->createComponent<IEndpoint>().addInterface<IEndpoint>(
                        "1.0.0",
                        convertEndpointPropertiesToCelix(endpoint->getProperties()))
                        .build().getInstance()
        );

        std::cout << endpoint->ToString() << std::endl; // TODO remove.
    }
}

void ConfiguredDiscoveryManager::addExportedEndpoint(IEndpoint* /*endpoint*/, celix::dm::Properties&& properties) {

    auto endpoint = std::make_shared<ConfiguredEndpoint>(convertCelixPropertiesToEndpoint(properties));
    auto contents = read_whole_file(_configurationFilePath);
    auto parsedJson = parseJSONFile(contents.value());
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

void ConfiguredDiscoveryManager::removeExportedEndpoint(IEndpoint* /*endpoint*/, celix::dm::Properties&& properties) {

    auto endpoint = std::make_shared<ConfiguredEndpoint>(convertCelixPropertiesToEndpoint(properties));
    auto contents = read_whole_file(_configurationFilePath);
    auto parsedJson = parseJSONFile(contents.value());
    auto endpointJSON = endpoint->exportToJSON(parsedJson);

    if (parsedJson.IsObject()) {
        if (parsedJson.HasMember(ENDPOINT_ARRAY)) {
            rapidjson::Value& endpointJsonArray = parsedJson[ENDPOINT_ARRAY];
            if (endpointJsonArray.IsArray()){
                for (rapidjson::Value::ValueIterator itr = endpointJsonArray.Begin(); itr != endpointJsonArray.End();) {
                    if (std::strcmp(( *itr )["endpoint.id"].GetString(), endpointJSON["endpoint.id"].GetString()) == 0) {
                        itr = endpointJsonArray.Erase(itr);
                    } else {
                        ++itr;
                    }
                }
            }
        }
    }
}

} // end namespace celix::async_rsa::discovery.
