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

#include <filesystem>
#include <fstream>
#include <optional>
#include <rapidjson/writer.h>

#define L_TRACE(...) logHelper.trace(__VA_ARGS__);
#define L_DEBUG(...) logHelper.debug(__VA_ARGS__);
#define L_INFO(...) logHelper.info(__VA_ARGS__);
#define L_WARN(...) logHelper.warning(__VA_ARGS__);
#define L_ERROR(...) logHelper.error(__VA_ARGS__);

#define ENDPOINT_ARRAY "endpoints"

namespace /*anon*/
{
    
static std::optional<std::string> readFile(const std::string& path) {
    std::string contents;
    std::ifstream file(path);
    if (!file) {
        throw celix::rsa::RemoteServicesException{"Cannot open file"};
    }
    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], (std::streamsize)contents.size());
    file.close();
    return contents;
}

static rapidjson::Document parseJSONFile(std::string& contents) {
    rapidjson::Document resultDocument{};
    resultDocument.ParseInsitu(contents.data());
    return resultDocument;
}

} // namespace

celix::rsa::ConfiguredDiscoveryManager::ConfiguredDiscoveryManager(std::shared_ptr<celix::BundleContext> _ctx)
    : ctx{std::move(_ctx)},
      configuredDiscoveryFiles{ctx->getConfigProperty(celix::rsa::CONFIGURED_DISCOVERY_DISCOVERY_FILES, "")},
      logHelper{ctx, celix::typeName<ConfiguredDiscoveryManager>()} {
    readConfiguredDiscoveryFiles();
}

void celix::rsa::ConfiguredDiscoveryManager::readConfiguredDiscoveryFiles() {
    if (!configuredDiscoveryFiles.empty()) {
        for (const auto& path : celix::split(configuredDiscoveryFiles)) {
            try {
                addConfiguredDiscoveryFile(path);
            } catch (celix::rsa::RemoteServicesException& e) {
                L_ERROR("Error adding disc file %s: %s", path.c_str(), e.what());
            }
        }
    }
}

celix::Properties
celix::rsa::ConfiguredDiscoveryManager::convertToEndpointProperties(const rapidjson::Value& endpointJSON) {
    celix::Properties result{};
    result.set(celix::rsa::ENDPOINT_FRAMEWORK_UUID, ctx->getFramework()->getUUID());
    for (auto it = endpointJSON.MemberBegin(); it != endpointJSON.MemberEnd(); ++it) {
        if (it->value.IsString()) {
            if (celix_utils_stringEquals(it->name.GetString(), "endpoint.objectClass")) { // TODO improve
                result.set(celix::SERVICE_NAME, it->value.GetString());
            } else {
                result.set(it->name.GetString(), it->value.GetString());
            }
        } else if (it->value.IsBool()) {
            result.set(it->name.GetString(), it->value.GetBool());
        } else if (it->value.IsArray()) {
            std::string strArray{};
            for (const auto& entry : it->value.GetArray()) {
                if (entry.IsString()) {
                    strArray.append(entry.GetString());
                    strArray.append(",");
                } else {
                    L_WARN("Cannot parse endpoint member %s. Cannot parse array where the elements are not strings",
                           it->name.GetString());
                    continue;
                }
            }
            result.set(it->name.GetString(), strArray.substr(0, strArray.size() - 1 /*remote last ","*/));
        } else {
            L_WARN("Cannot parse endpoint member %s. Type is %i", it->name.GetString(), (int)it->value.GetType());
        }
    }
    return result;
}

void celix::rsa::ConfiguredDiscoveryManager::addConfiguredDiscoveryFile(const std::string& path) {
    try {
        std::vector<std::shared_ptr<celix::ServiceRegistration>> newEndpoints{};
        auto contents = readFile(path);
        if (contents) {
            auto parsedJson = parseJSONFile(contents.value());
            if (parsedJson.IsObject()) {
                if (parsedJson.HasMember(ENDPOINT_ARRAY) && parsedJson[ENDPOINT_ARRAY].IsArray()) {
                    for (auto& jsonEndpoint : parsedJson[ENDPOINT_ARRAY].GetArray()) {
                        try {
                            auto endpointProperties = convertToEndpointProperties(jsonEndpoint);
                            auto endpointDescription =
                                std::make_shared<EndpointDescription>(std::move(endpointProperties));
                            L_TRACE("Created endpoint description from %s: %s",
                                    path.c_str(),
                                    endpointDescription->toString().c_str())

                            auto reg =
                                ctx->registerService<celix::rsa::EndpointDescription>(std::move(endpointDescription))
                                    .build();
                            newEndpoints.emplace_back(std::move(reg));
                        } catch (celix::rsa::RemoteServicesException& e) {
                            L_ERROR("Error creating EndpointDescription from endpoints entry in JSON from path %s: %s",
                                    path.c_str(),
                                    e.what());
                        }
                    }
                }
            }
            std::lock_guard lock{mutex};
            endpointRegistrations.emplace(path, std::move(newEndpoints));
        }
    } catch (std::exception& e) {
        throw celix::rsa::RemoteServicesException{std::string{"Error adding configured discovery file: "} + e.what()};
    } catch (...) {
        throw celix::rsa::RemoteServicesException{"Error adding configured discovery file."};
    }
}

void celix::rsa::ConfiguredDiscoveryManager::removeConfiguredDiscoveryFile(const std::string& path) {
    std::lock_guard lock{mutex};
    endpointRegistrations.erase(path);
}

std::vector<std::string> celix::rsa::ConfiguredDiscoveryManager::getConfiguredDiscoveryFiles() const {
    std::vector<std::string> result{};
    std::lock_guard lock{mutex};
    for (const auto& pair : endpointRegistrations) {
        result.emplace_back(pair.first);
    }
    return result;
}
