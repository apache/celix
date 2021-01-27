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

#include <IEndpointEventListener.h>
#include <rapidjson/writer.h>

#include <rapidjson/filereadstream.h>

namespace celix::async_rsa::discovery {

constexpr const char* ENDPOINT_ARRAY = "endpoints";
constexpr const char* ENDPOINT_IDENTIFIER = "endpoint.id";
constexpr const char* ENDPOINT_IMPORTED = "service.imported";
constexpr const char* ENDPOINT_IMPORT_CONFIGS = "service.imported.configs";
constexpr const char* ENDPOINT_EXPORTS = "service.exported.interfaces";
constexpr const char* ENDPOINT_OBJECTCLASS = "endpoint.objectClass";
constexpr const char* ENDPOINT_SCOPE = "endpoint.scope";
constexpr const char* ENDPOINT_TOPIC = "endpoint.topic";

rapidjson::Document parseJSONFile(const std::string& filePath)  {

    rapidjson::Document resultDocument{};
    FILE* filePtr = fopen(filePath.c_str(), "r");
    char readBuffer[65536];
    auto fileStream = rapidjson::FileReadStream(filePtr, readBuffer, sizeof(readBuffer));
    resultDocument.ParseStream(fileStream);
    return resultDocument;
}

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager,
                                                       std::string configurationFilePath) :
        _dependencyManager{std::move(dependencyManager)},
        _endpointEventListeners{},
        _configurationFilePath{std::move(configurationFilePath)} {

    discoverEndpoints(); // TODO this call should probably come from the topology manager?
}

void ConfiguredDiscoveryManager::addEndpointEventListener(const std::shared_ptr<IEndpointEventListener>& endpointEventListener) {

    // search for duplicates, only add new listeners.
    const auto eventListenerIter = std::find(_endpointEventListeners.begin(), _endpointEventListeners.end(), endpointEventListener);
    if(eventListenerIter == _endpointEventListeners.end()) {
        _endpointEventListeners.push_back(endpointEventListener);
    }
}

void ConfiguredDiscoveryManager::discoverEndpoints() {

    // TODO search JSON files/directory for configured endpoints within.

    /**
     * ENDPOINT FROM 'ROAD TO INTEROPERABILITY' JSON-EXAMPLE:
     *
     * {
     *    "org.apache.aries.rsa.discovery.config~ComponentName": {
     *          "endpoint.id": "id-01",
     *          "service.imported": true,
     *          "service.imported.configs:String[]": [
     *              "pubsub"
     *          ],
     *          "objectClass:String[]": [
     *              "com.temp.componentName"
     *          ],
     *          "scope:String": "ComponentName",
     *          "topic:String": "com.temp.ComponentName"
     *    }
     * }
     */

    /**
     *     _endpoints.emplace_back(
     *       &_mng->createComponent<StaticEndpoint>().addInterface<IEndpoint>(
     *               "1.0.0", celix::dm::Properties{
     *                   {"service.imported", "*"},
     *                   {"service.exported.interfaces", "IHardcodedService"},
     *                   {"endpoint.id", "1"},
     *               })
     *       .build()
     *       .getInstance());
     */

    const rapidjson::Document& parsedJson = parseJSONFile(_configurationFilePath);

    if (parsedJson.IsObject()) {
        const rapidjson::Value& endpointJsonArray = parsedJson[ENDPOINT_ARRAY];
        if (endpointJsonArray.IsArray() && ( endpointJsonArray.Size() > 0)) {

            for (rapidjson::Value::ConstValueIterator endpointIter = endpointJsonArray.Begin(); endpointIter != endpointJsonArray.End(); endpointIter++) {
                if (endpointIter->IsObject()) {

                    const auto& endpointJson = endpointIter->GetObject();
                    std::cout << std::boolalpha << std::endl;
                    std::cout << "Identifier: " << endpointJson[ENDPOINT_IDENTIFIER].GetString() << std::endl;
                    std::cout << "Imported: " << endpointJson[ENDPOINT_IMPORTED].GetBool() << std::endl;

                    const rapidjson::Value& importsJsonArray = endpointJson[ENDPOINT_IMPORT_CONFIGS];
                    if (importsJsonArray.IsArray() && ( importsJsonArray.Size() > 0)) {
                        std::cout << " -> [";
                        for (rapidjson::Value::ConstValueIterator importIter = importsJsonArray.Begin(); importIter != importsJsonArray.End(); importIter++) {
                            if (importIter->IsString()) {
                                const auto& importConfigJsonStr = importIter->GetString();
                                std::cout << importConfigJsonStr;
                            }
                        }
                        std::cout << "]" << std::endl;
                    }

                    std::cout << "ExportedServices: " << endpointJson[ENDPOINT_EXPORTS].GetString() << std::endl;
                    std::cout << "ObjectClass: " << endpointJson[ENDPOINT_OBJECTCLASS].GetString() << std::endl;
                    std::cout << "Scope: " << endpointJson[ENDPOINT_SCOPE].GetString() << std::endl;
                    std::cout << "Topic: " << endpointJson[ENDPOINT_TOPIC].GetString() << std::endl;
                    std::cout << std::endl;
                }
            }

        } else {
            // TODO no available/valid endpoints in array.
        }
    } else {
        // TODO parsed json invalid.
    }
}

void ConfiguredDiscoveryManager::updateListeners(const std::shared_ptr<IEndpoint>& endpoint) {

    for(const auto& listener : _endpointEventListeners) {
        listener->incomingEndpointEvent(endpoint);
    }
}

} // end namespace celix::async_rsa::discovery.
