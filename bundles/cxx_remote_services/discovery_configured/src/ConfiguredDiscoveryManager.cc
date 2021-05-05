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

#include <celix_bundle_context.h>

#include <rapidjson/writer.h>

static constexpr const char* ENDPOINT_ARRAY = "endpoints";

static std::optional<std::string> readFile(const std::string& path) {

    std::string contents;
    std::ifstream file(path);
    if(!file) {
        return {};
    }
    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], (std::streamsize)contents.size());
    file.close();
    return contents;
}

static rapidjson::Document parseJSONFile(std::string& contents)  {

    rapidjson::Document resultDocument{};
    resultDocument.ParseInsitu(contents.data());
    return resultDocument;
}

celix::rsa::ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<celix::BundleContext> _ctx) :
        ctx{std::move(_ctx)},
        configuredDiscoveryFiles{ctx->getConfigProperty(celix::rsa::CONFIGURED_DISCOVERY_DISCOVERY_FILES, "")} {
    readConfiguredDiscoveryFiles();
}

void celix::rsa::ConfiguredDiscoveryManager::readConfiguredDiscoveryFiles() {
    //TODO split configuredDiscoveryFiles
    if (!configuredDiscoveryFiles.empty()) {
        for (auto path : celix::split(configuredDiscoveryFiles)) {
            addConfiguredDiscoveryFile(path);
        }
    }
}

//static std::vector<std::string> parseJSONStringArray(const rapidjson::Value& jsonArray) {
//    std::vector<std::string> resultVec{};
//    if (jsonArray.IsArray()) {
//        resultVec.reserve(jsonArray.Size());
//        for (auto& element : jsonArray.GetArray()) {
//            if (element.IsString()) {
//                resultVec.emplace_back(element.GetString());
//            }
//        }
//    }
//    return resultVec;
//}

celix::Properties celix::rsa::ConfiguredDiscoveryManager::convertToEndpointProperties(const rapidjson::Value &endpointJSON) {
    celix::Properties result{};
    result.set(celix::rsa::ENDPOINT_FRAMEWORK_UUID, ctx->getFramework()->getUUID());
    for (auto it = endpointJSON.MemberBegin(); it != endpointJSON.MemberEnd(); ++it) {
        if (it->value.GetType() == rapidjson::Type::kStringType) {
            if (celix_utils_stringEquals(it->name.GetString(), "endpoint.objectClass")) { //TODO improve
                result.set(celix::SERVICE_NAME, it->value.GetString());
            } else {
                result.set(it->name.GetString(), it->value.GetString());
            }
        } else if (it->value.GetType() == rapidjson::Type::kTrueType || it->value.GetType() == rapidjson::Type::kFalseType) {
            result.set(it->name.GetString(), it->value.GetBool());
        } else {
            std::cout << "TODO member " << it->name.GetString() << std::endl;
        }
    }
    return result;
}

void celix::rsa::ConfiguredDiscoveryManager::addConfiguredDiscoveryFile(const std::string& path) {
    std::vector<std::shared_ptr<celix::ServiceRegistration>> newEndpoints{};
    auto contents = readFile(path);
    if (contents) {
        auto parsedJson = parseJSONFile(contents.value());
        if (parsedJson.IsObject()) {
            if (parsedJson.HasMember(ENDPOINT_ARRAY) && parsedJson[ENDPOINT_ARRAY].IsArray()) {

                for (auto& jsonEndpoint : parsedJson[ENDPOINT_ARRAY].GetArray()) {
                    try {
                        auto endpointProperties = convertToEndpointProperties(jsonEndpoint);
                        auto endpointDescription = std::make_shared<EndpointDescription>(std::move(endpointProperties));

                        //TODO log service
                        std::cout << endpointDescription->toString() << std::endl;

                        auto reg = ctx->registerService<celix::rsa::EndpointDescription>(std::move(endpointDescription))
                                .build();
                        newEndpoints.emplace_back(std::move(reg));
                    } catch (celix::rsa::RemoteServicesException& exp) {
                        //TODO log service
                        std::cerr << exp.what() << std::endl;
                    }
                }
            }
        }
        std::lock_guard<std::mutex> lock{mutex};
        endpointRegistrations.emplace(path, std::move(newEndpoints));
    }
}

void celix::rsa::ConfiguredDiscoveryManager::removeConfiguredDiscoveryFile(const std::string& path) {
    std::lock_guard<std::mutex> lock{mutex};
    endpointRegistrations.erase(path);
}
