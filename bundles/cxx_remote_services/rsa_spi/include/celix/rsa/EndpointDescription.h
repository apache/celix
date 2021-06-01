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

#include "celix/Utils.h"
#include "celix/Constants.h"
#include "celix/Properties.h"
#include "celix/rsa/RemoteConstants.h"
#include "celix/rsa/RemoteServicesException.h"

namespace celix::rsa {

    /**
     * @brief A description of an endpoint that provides sufficient information for a compatible distribution provider to create a connection to this endpoint.
     *
     * An Endpoint Description is easy to transfer between different systems because it is property based where
     * the property keys are strings and the values are simple types.
     * This allows it to be used as a communications device to convey available endpoint information
     * to nodes in a network. An Endpoint Description reflects the perspective of an importer.
     * That is, the property keys have been chosen to match filters that are created by client bundles that need a service.
     *
     * Therefore the map must not contain any service.exported.* property and must contain the corresponding service.imported.* ones.
     * The service.intents property must contain the intents provided by the service itself combined with the intents
     * added by the exporting distribution provider. Qualified intents appear fully expanded on this property.
     */
    class EndpointDescription {
    public:

        /**
         * @brief Create an Endpoint Description from properties
         *
         * The endpoint.id, service.imported.configs and objectClass properties must be set. .
         *
         * @param The properties from which to create the Endpoint Description.
         */
        explicit EndpointDescription(celix::Properties properties) :
                endpointProperties{std::move(properties)},
                endpointId{endpointProperties.get(celix::rsa::ENDPOINT_ID)},
                configurationTypes{celix::split(endpointProperties.get(celix::rsa::SERVICE_IMPORTED_CONFIGS))},
                frameworkUUID{endpointProperties.get(celix::rsa::ENDPOINT_FRAMEWORK_UUID)},
                intents{celix::split(endpointProperties.get(celix::rsa::SERVICE_INTENTS))},
                serviceName{endpointProperties.get(celix::SERVICE_NAME)} {
            checkValidEndpoint();
        }

        /**
         * @brief Create an Endpoint Description based on service properties and optional provided rsa properties.
         *
         * The properties in the rsa properties take precedence over the service properties.
         * the the service.imported property will be set.
         * The endpoint.id, service.imported.configs and objectClass properties must be set after construction.
         *
         * @param frameworkUUID the framework uuid used for endpoint ids.
         * @param serviceProperties The service properties of a service that can be exported.
         * @param rsaProperties The optional properties provided by the Remote Service Admin.
         */
        EndpointDescription(std::string_view frameworkUUID, const celix::Properties& serviceProperties, const celix::Properties& rsaProperties = {}) :
                endpointProperties{importedProperties(frameworkUUID, serviceProperties, rsaProperties)},
                endpointId{endpointProperties.get(celix::rsa::ENDPOINT_ID)},
                configurationTypes{celix::split(endpointProperties.get(celix::rsa::SERVICE_IMPORTED_CONFIGS))},
                frameworkUUID{endpointProperties.get(celix::rsa::ENDPOINT_FRAMEWORK_UUID)},
                intents{celix::split(endpointProperties.get(celix::rsa::SERVICE_INTENTS))},
                serviceName{endpointProperties.get(celix::SERVICE_NAME)} {
            checkValidEndpoint();
        }

        /**
         * @brief Returns the endpoint's id.
         *
         * The id is an opaque id for an endpoint.
         * No two different endpoints must have the same id.
         * Two Endpoint Descriptions with the same id must represent the same endpoint.
         * The value of the id is stored in the RemoteConstants.ENDPOINT_ID property.
         */
        [[nodiscard]] const std::string& getId() const {
            return endpointId;
        }

        /**
         * @brief Returns the configuration types.
         *
         * A distribution provider exports a service with an endpoint.
         * This endpoint uses some kind of communications protocol with a set of configuration parameters.
         * There are many different types but each endpoint is configured by only one configuration type.
         * However, a distribution provider can be aware of different configuration types and provide synonyms to
         * increase the change a receiving distribution provider can create a connection to this endpoint.
         * This value of the configuration types is stored in the celix::rsa::SERVICE_IMPORTED_CONFIGS service property.
         */
        [[nodiscard]] const std::vector<std::string>& getConfigurationTypes() const {
            return configurationTypes;
        }

        /**
         * @brief Return the framework UUID for the remote service, if present.
         *
         * The value of the remote framework UUID is stored in the celix::rsa::ENDPOINT_FRAMEWORK_UUID endpoint property.
         */
        [[nodiscard]] const std::string& getFrameworkUUID() const {
            return frameworkUUID;
        }

        /**
         * @brief Return the list of intents implemented by this endpoint.
         *
         * The intents are based on the service.intents on an imported service, except for any intents that are
         * additionally provided by the importing distribution provider.
         * All qualified intents must have been expanded.
         * This value of the intents is stored in the RemoteConstants.SERVICE_INTENTS service property.
         */
        [[nodiscard]] const std::vector<std::string>& getIntents() const {
            return intents;
        }

        /**
         * @brief  Provide the interface implemented by the exported service.
         * The value of the interface is derived from the objectClass property.
         */
        [[nodiscard]] const std::string& getInterface() const {
            return serviceName;
        }

        /**
         * @brief Returns all endpoint properties.
         */
        [[nodiscard]] const celix::Properties& getProperties() const {
            return endpointProperties;
        }

        /**
         * @brief converts the EndpointDescription to a printable string.
         */
        [[nodiscard]] std::string toString() const {
            std::string result{"Endpoint["};
            for (auto& prop : endpointProperties) {
                result.append(prop.first);
                result.append("=");
                result.append(prop.second);
                result.append(", ");
            }
            result.append("]");
            return result;
        }
    protected:
        celix::Properties importedProperties(std::string_view frameworkUUID, const celix::Properties& serviceProperties, const celix::Properties& rsaProperties) {
            celix::Properties result{};

            for (const auto& entry: serviceProperties) {
                result.set(entry.first, entry.second); //TODO filter and remove service.export.* entries
            }
            for (const auto& entry: rsaProperties) {
                result.set(entry.first, entry.second);
            }

            result.set(celix::rsa::SERVICE_IMPORTED, true);

            if (result.get(celix::rsa::ENDPOINT_ID).empty()) {
                result.set(celix::rsa::ENDPOINT_ID, std::string{frameworkUUID} + "-" + serviceProperties.get(celix::SERVICE_ID));
            }

            return result;
        }

        void checkValidEndpoint() const {
            std::string baseMsg = "Invalid properties for EndpointDescription, missing mandatory property ";
            if (endpointId.empty()) {
                throw celix::rsa::RemoteServicesException{baseMsg.append(celix::rsa::ENDPOINT_ID)};
            }
            if (configurationTypes.empty()) {
                throw celix::rsa::RemoteServicesException{baseMsg.append(celix::rsa::SERVICE_IMPORTED_CONFIGS)};
            }
            if (serviceName.empty()) {
                throw celix::rsa::RemoteServicesException{baseMsg.append(celix::SERVICE_NAME)};
            }
        }

        const celix::Properties endpointProperties;
        const std::string endpointId;
        const std::vector<std::string> configurationTypes;
        const std::string frameworkUUID;
        const std::vector<std::string> intents;
        const std::string serviceName;
    };
} // end namespace celix::rsa.
