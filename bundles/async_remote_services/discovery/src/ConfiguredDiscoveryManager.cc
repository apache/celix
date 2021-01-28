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

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager,
                                                       std::string configurationFilePath) :
        _dependencyManager{std::move(dependencyManager)},
        _endpointEventListeners{},
        _configurationFilePath{std::move(configurationFilePath)},
        _endpoints{},
        _publishedEndpoints{} {

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

            for (rapidjson::Value::ConstValueIterator endpointIter = endpointJsonArray.Begin();
                        endpointIter != endpointJsonArray.End(); endpointIter++) {

                if (endpointIter->IsObject()) {

                    const auto& endpointJson = endpointIter->GetObject();
                    _endpoints.emplace_back(std::make_shared<ConfiguredEndpoint>(endpointJson));

                } else {
                    // TODO invalid endpoint JSON object.
                }
            }
        } else {
            // TODO no available/valid endpoints in array.
        }
    } else {
        // TODO parsed json invalid.
    }

    publishParsedEndpoints();
}

void ConfiguredDiscoveryManager::publishParsedEndpoints() {

    for (const auto& endpoint : _endpoints) {

        const auto endpointProperties = endpoint->getProperties();
        const auto celixProperties = celix::dm::Properties{{"service.imported",
                                                           std::to_string(endpointProperties.isImported()).c_str()},
                                                           {"service.exported.interfaces", endpointProperties.getExports()},
                                                           {"endpoint.id", endpointProperties.getId()}};
        _publishedEndpoints.emplace_back(
                &_dependencyManager->createComponent<IEndpoint>().addInterface<IEndpoint>(
                        "1.0.0", celixProperties).build().getInstance());
    }
}

void ConfiguredDiscoveryManager::updateListeners(const std::shared_ptr<IEndpoint>& endpoint) {

    for(const auto& listener : _endpointEventListeners) {
        listener->incomingEndpointEvent(endpoint);
    }
}

} // end namespace celix::async_rsa::discovery.
