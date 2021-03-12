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
#include <map>

#include <celix/dm/Properties.h>

constexpr const char* ENDPOINT_IDENTIFIER = "endpoint.id";
constexpr const char* ENDPOINT_IMPORTED = "service.imported";
constexpr const char* ENDPOINT_IMPORT_CONFIGS = "service.imported.configs";
constexpr const char* ENDPOINT_EXPORTS = "service.exported.interfaces";
constexpr const char* ENDPOINT_OBJECTCLASS = "endpoint.objectClass";
constexpr const char* ENDPOINT_SCOPE = "endpoint.scope";
constexpr const char* ENDPOINT_TOPIC = "endpoint.topic";

namespace celix::rsa {

/**
 * Endpoint class for celix::rsa namespace.
 * This class holds data for discovered remote services or published local services.
 */
class Endpoint {
public:

    /**
     * Endpoint constructor.
     * @param properties celix properties with information about this endpoint and what its documenting.
     */
    explicit Endpoint(celix::dm::Properties properties) : _celixProperties{std::move(properties)} {
        // TODO validate mandatory properties are set.
    }

    [[nodiscard]] const celix::dm::Properties& getProperties() const {
        return _celixProperties;
    }

    [[nodiscard]] std::string getEndpointId() const {
        return _celixProperties.get("endpoint.id");
    }

protected:
    celix::dm::Properties _celixProperties;

};
} // end namespace celix::rsa.
