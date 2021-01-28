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
#pragma once

#include <IEndpoint.h>

#include <memory>
#include <string>
#include <vector>

#include <rapidjson/document.h>

namespace celix::async_rsa::discovery {

class ConfiguredEndpointProperties {
public:

    ConfiguredEndpointProperties() = delete;

    ConfiguredEndpointProperties(std::string id, bool imported, std::vector<std::string> importConfigs,
                                 std::string exports, std::vector<std::string> objectClass,
                                 std::string scope, std::string topic) :
                            _id{std::move(id)}, _imported{imported}, _importConfigs{std::move(importConfigs)},
                            _exports{std::move(exports)}, _objectClass{std::move(objectClass)},
                            _scope{std::move(scope)}, _topic{std::move(topic)} {
    }

    [[nodiscard]] const std::string& getId() const {

        return _id;
    }

    [[nodiscard]] bool isImported() const {

        return _imported;
    }

    [[nodiscard]] const std::vector<std::string>& getImportConfigs() const {

        return _importConfigs;
    }

    [[nodiscard]] const std::string& getExports() const {

        return _exports;
    }

    [[nodiscard]] const std::vector<std::string>& getObjectClass() const {

        return _objectClass;
    }

    [[nodiscard]] const std::string& getScope() const {

        return _scope;
    }

    [[nodiscard]] const std::string& getTopic() const {

        return _topic;
    }

    [[nodiscard]] std::string ToString() const {
        return "[ID: " + _id + ", Imported: " + std::to_string(_imported) +
               ", Import (count): " + std::to_string(_importConfigs.size()) + ", Exports: " + _exports +
               ", ObjectClass (count) " + std::to_string(_objectClass.size()) + ", Scope: "
               + _scope + ", Topic: " + _topic + "]";
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

class ConfiguredEndpoint : public IEndpoint {
public:

    ConfiguredEndpoint() = delete;

    explicit ConfiguredEndpoint(const rapidjson::Value& endpointJson);

    const ConfiguredEndpointProperties& getProperties() const;

    [[nodiscard]] std::string ToString() const {
        return "[ConfiguredEndpoint] Properties: " + _properties.ToString();
    }

private:

    const ConfiguredEndpointProperties _properties;
};

} // end namespace celix::async_rsa::discovery.
