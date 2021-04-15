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

#include <string>
#include <vector>

namespace celix::rsa {

/**
 * Data object for the ConfiguredEndpoint class, containing all data parsed from input JSON objects.
 */
class ConfiguredEndpointProperties {
public:

    ConfiguredEndpointProperties(
            std::string id, bool imported, std::vector <std::string> importConfigs,
            std::string exports, std::vector <std::string> objectClass,
            std::string scope, std::string topic) :
            _id{std::move(id)},
            _imported{imported},
            _importConfigs{std::move(importConfigs)},
            _exports{std::move(exports)},
            _objectClass{std::move(objectClass)},
            _scope{std::move(scope)},
            _topic{std::move(topic)} {
    }

    std::string getId() const {

        return _id;
    }

    bool isImported() const {

        return _imported;
    }

    const std::vector <std::string>& getImportConfigs() const {

        return _importConfigs;
    }

    const std::string& getExports() const {

        return _exports;
    }

    const std::vector <std::string>& getObjectClass() const {

        return _objectClass;
    }

    const std::string& getScope() const {

        return _scope;
    }

    const std::string& getTopic() const {

        return _topic;
    }

    std::string ToString() const {

        return "id: " + _id + ", imported: " + std::to_string(_imported) +
               ", imports (size): " + std::to_string(_importConfigs.size()) + ", exports: " + _exports +
               ", objclass (size) " + std::to_string(_objectClass.size()) + ", scope: "
               + _scope + ", topic: " + _topic;
    }

private:

    std::string _id;
    bool _imported;
    std::vector <std::string> _importConfigs;
    std::string _exports;
    std::vector <std::string> _objectClass;
    std::string _scope;
    std::string _topic;
};

} // end namespace celix::rsa.
