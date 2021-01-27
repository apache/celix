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

class ConfiguredEndpoint : public IEndpoint {
public:

    ConfiguredEndpoint() = delete;

    ConfiguredEndpoint(std::string id,
                       bool imported,
                       std::vector<std::string> importConfigs,
                       std::string exports,
                       std::vector<std::string> objectClass,
                       std::string scope,
                       std::string topic) :
                               _id{std::move(id)},
                               _imported{imported},
                               _importConfigs{std::move(importConfigs)},
                               _exports{std::move(exports)},
                               _objectClass{std::move(objectClass)},
                               _scope{std::move(scope)},
                               _topic{std::move(topic)} {
    }

    std::string ToString() {
        return "[ConfiguredEndpoint][ID: " + _id + "][Imported: " + std::to_string(_imported) +
        "][Import (count): " + std::to_string(_importConfigs.size()) + "][Exports: " + _exports +
        "][ObjectClass (count) " + std::to_string(_objectClass.size()) + "][Scope: " + _scope + "][Topic: " + _topic + "]";
    }

private:
    std::string _id;
    bool _imported;
    std::vector<std::string> _importConfigs;
    std::string _exports;
    std::vector<std::string> _objectClass;
    std::string _scope;
    std::string _topic;
};

rapidjson::Document parseJSONFile(const std::string& filePath)  {

    rapidjson::Document resultDocument{};
    FILE* filePtr = fopen(filePath.c_str(), "r");
    char readBuffer[65536];
    auto fileStream = rapidjson::FileReadStream(filePtr, readBuffer, sizeof(readBuffer));
    resultDocument.ParseStream(fileStream);
    return resultDocument;
}

std::vector<std::string> parseJSONStringArray(const rapidjson::Value& jsonArray) {

    std::vector<std::string> resultVec{};
    if (jsonArray.IsArray() && (jsonArray.Size() > 0)) {
        for (rapidjson::Value::ConstValueIterator iter = jsonArray.Begin(); iter != jsonArray.End(); iter++) {
            if (iter->IsString()) {
                resultVec.emplace_back(iter->GetString());
            }
        }
    }
    return resultVec;
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

            for (rapidjson::Value::ConstValueIterator endpointIter = endpointJsonArray.Begin();
                        endpointIter != endpointJsonArray.End(); endpointIter++) {

                if (endpointIter->IsObject()) {

                    const auto& endpointJson = endpointIter->GetObject();
                    const auto endpointId = endpointJson[ENDPOINT_IDENTIFIER].GetString();
                    const auto endpointImported = endpointJson[ENDPOINT_IMPORTED].GetBool();
                    const auto endpointImportConfigs = parseJSONStringArray(endpointJson[ENDPOINT_IMPORT_CONFIGS]);
                    const auto endpointExports = endpointJson[ENDPOINT_EXPORTS].GetString();
                    const auto endpointObjectClass = parseJSONStringArray(endpointJson[ENDPOINT_OBJECTCLASS]);
                    const auto endpointScope = endpointJson[ENDPOINT_SCOPE].GetString();
                    const auto endpointTopic = endpointJson[ENDPOINT_TOPIC].GetString();

                    const auto newEndpointPtr = std::make_shared<ConfiguredEndpoint>(endpointId,
                                                                                     endpointImported,
                                                                                     endpointImportConfigs,
                                                                                     endpointExports,
                                                                                     endpointObjectClass,
                                                                                     endpointScope,
                                                                                     endpointTopic);
                    std::cout << newEndpointPtr->ToString() << std::endl;

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
}

void ConfiguredDiscoveryManager::updateListeners(const std::shared_ptr<IEndpoint>& endpoint) {

    for(const auto& listener : _endpointEventListeners) {
        listener->incomingEndpointEvent(endpoint);
    }
}

} // end namespace celix::async_rsa::discovery.
