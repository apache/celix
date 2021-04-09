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

#include <ConfiguredEndpoint.h>

namespace celix::rsa {

celix::dm::Properties convertEndpointPropertiesToCelix(const ConfiguredEndpointProperties& endpointProperties) {

    return celix::dm::Properties{{celix::rsa::Endpoint::IMPORTED, std::to_string(endpointProperties.isImported()).c_str()},
                                 {celix::rsa::Endpoint::EXPORTS, endpointProperties.getExports()},
                                 {celix::rsa::Endpoint::IDENTIFIER, endpointProperties.getId()}};
}

ConfiguredEndpointProperties convertCelixPropertiesToEndpoint(const celix::dm::Properties& celixProperties) {

    auto endpointId = celixProperties.get(celix::rsa::Endpoint::IDENTIFIER);
    auto exports = celixProperties.get(celix::rsa::Endpoint::EXPORTS);
    auto imported = celixProperties.get(celix::rsa::Endpoint::IMPORTED);
    return ConfiguredEndpointProperties{endpointId,
                                        (imported == "true"),
                                        {}, exports, {}, "", ""};
}

bool isValidEndpointJson(const rapidjson::Value& endpointJson) {

    return (endpointJson.HasMember(celix::rsa::Endpoint::IDENTIFIER)
         && endpointJson.HasMember(celix::rsa::Endpoint::IMPORTED)
         && endpointJson.HasMember(celix::rsa::Endpoint::IMPORT_CONFIGS)
         && endpointJson.HasMember(celix::rsa::Endpoint::EXPORTS)
         && endpointJson.HasMember(celix::rsa::Endpoint::OBJECTCLASS)
         && endpointJson.HasMember(celix::rsa::Endpoint::SCOPE)
         && endpointJson.HasMember(celix::rsa::Endpoint::TOPIC));
}

std::vector<std::string> parseJSONStringArray(const rapidjson::Value& jsonArray) {

    std::vector<std::string> resultVec{};
    if(jsonArray.IsArray()) {
        resultVec.reserve(jsonArray.Size());
        for(auto& element : jsonArray.GetArray()) {
            if(element.IsString()) {
                resultVec.emplace_back(element.GetString());
            }
        }
    }
    return resultVec;
}

ConfiguredEndpoint::ConfiguredEndpoint(const rapidjson::Value& endpointJson) :
        Endpoint(celix::dm::Properties{}),
        _configuredProperties{} {

    if (isValidEndpointJson(endpointJson)) {

        _configuredProperties = {endpointJson[celix::rsa::Endpoint::IDENTIFIER].GetString(),
                                 endpointJson[celix::rsa::Endpoint::IMPORTED].GetBool(),
                                 parseJSONStringArray(endpointJson[celix::rsa::Endpoint::IMPORT_CONFIGS]),
                                 endpointJson[celix::rsa::Endpoint::EXPORTS].GetString(),
                                 parseJSONStringArray(endpointJson[celix::rsa::Endpoint::OBJECTCLASS]),
                                 endpointJson[celix::rsa::Endpoint::SCOPE].GetString(),
                                 endpointJson[celix::rsa::Endpoint::TOPIC].GetString()};

        _celixProperties = convertEndpointPropertiesToCelix(*_configuredProperties);
    }
}

const ConfiguredEndpointProperties& ConfiguredEndpoint::getConfiguredProperties() const {

    return *_configuredProperties;
}

} // end namespace celix::rsa.
