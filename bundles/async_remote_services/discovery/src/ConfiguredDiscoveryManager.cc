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

namespace celix::async_rsa::discovery {

ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<DependencyManager> dependencyManager) :
        _dependencyManager{std::move(dependencyManager)},
        _endpointEventListeners{},
        _json{},
        _jsonPrinter{} {

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

    const char* jsonStr = "{"
                          "\"hello\": \"world\", "
                          "\"t\": true,"
                          "\"f\": false,"
                          "\"n\": null,"
                          "\"i\": 123,"
                          "\"pi\": 3.1416,"
                          "\"a\": [1, 2, 3, 4]"
                          "}";

    _json.Parse(jsonStr);

    // configure a JSON printer for logging purposes.
    auto writer = rapidjson::Writer<rapidjson::StringBuffer>(_jsonPrinter);
    _json.Accept(writer);

    std::cout << "BLABLABLA: " << _jsonPrinter.GetString() << std::endl;
}

void ConfiguredDiscoveryManager::updateListeners(const std::shared_ptr<IEndpoint>& endpoint) {

    for(const auto& listener : _endpointEventListeners) {
        listener->incomingEndpointEvent(endpoint);
    }
}

} // end namespace celix::async_rsa::discovery.
