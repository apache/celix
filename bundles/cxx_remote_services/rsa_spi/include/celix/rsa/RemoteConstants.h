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

//TODO move header to rsa_api library

namespace celix::rsa {


    /**
     * @brief Endpoint property identifying the universally unique id of the exporting framework.
     *
     * Can be absent if the corresponding endpoint is not for an OSGi service.
     * The value of this property must be of type string.
     */
    constexpr const char * const ENDPOINT_FRAMEWORK_UUID = "endpoint.framework.uuid";

    /**
     * @brief Endpoint property identifying the id for this endpoint.
     *
     * This service property must always be set.
     * The value of this property must be of type string.
     */
    constexpr const char * const ENDPOINT_ID = "endpoint.id";

    /**
     * @brief Endpoint property identifying the service id of the exported service.
     *
     * Can be absent or 0 if the corresponding endpoint is not for an OSGi service.
     * The value of this property must be of type long.
     */
    constexpr const char * const ENDPOINT_SERVICE_ID = "endpoint.service.id";

    /**
     * @brief Service property identifying the configuration types supported by a distribution provider.
     *
     * Registered by the distribution provider on one of its services to indicate the supported configuration types.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const REMOTE_CONFIGS_SUPPORTED = "remote.configs.supported";

    /**
     * @brief Service property identifying the configuration types that should be used to export the service.
     *
     * Each configuration type represents the configuration parameters for an endpoint.
     * A distribution provider should create an endpoint for each configuration type that it supports.
     * This property may be supplied in the properties object passed to a registerService method.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const REMOTE_INTENTS_SUPPORTED = "remote.intents.supported";

    /**
     * @brief  Service property identifying the intents that the distribution provider must implement to distribute the service.
     *
     * Intents listed in this property are reserved for intents that are critical for the code to function correctly,
     * for example, ordering of messages. These intents should not be configurable.
     * This property may be supplied in the properties object passed to the a registerService method.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const SERVICE_EXPORTED_INTENTS = "service.exported.intents";

    /**
     * @brief Service property identifying the extra intents that the distribution provider must implement to distribute the service.
     *
     * This property is merged with the service.exported.intents property before the distribution provider interprets
     * the listed intents; it has therefore the same semantics but the property should be configurable so the
     * administrator can choose the intents based on the topology.
     * Bundles should therefore make this property configurable, for example through the Configuration Admin service.
     * This property may be supplied in the properties object passed to the a registerService method.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const SERVICE_EXPORTED_INTENTS_EXTRA = "service.exported.intents.extra";

    /**
     * @brief Service property marking the service for export. It defines the interfaces under which this service can be exported.
     *
     * Note for Celix only 1 interface can be register per service registration, so only 1 interface can be exported using
     * the service.exported.interfaces property.
     * This value must be the exported service type or the value of an asterisk ('*' \u002A).
     * The value of this property must be of type string.
     */
    constexpr const char * const SERVICE_EXPORTED_INTERFACES = "service.exported.interfaces";

    /**
     * @brief Service property identifying the service as imported.
     *
     * This service property must be set by a distribution provider to any value when it registers the endpoint proxy as an imported service.
     * A bundle can use this property to filter out imported services. The value of this property may be of any type.
     */
    constexpr const char * const SERVICE_IMPORTED = "service.imported";

    /**
     * @brief Service property identifying the configuration types used to import the service.
     *
     * Any associated properties for this configuration types must be properly mapped to the importing system.
     * For example, a URL in these properties must point to a valid resource when used in the importing framework.
     * If multiple configuration types are listed in this property, then they must be synonyms for exactly the same
     * remote endpoint that is used to export this service.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const SERVICE_IMPORTED_CONFIGS = "service.imported.configs";

    /**
     * @brief Service property identifying the intents that this service implement.
     *
     * This property has a dual purpose:
     * A bundle can use this service property to notify the distribution provider that these intents are already
     * implemented by the exported service object.
     * A distribution provider must use this property to convey the combined intents of: The exporting service,
     * and the intents that the exporting distribution provider adds, and the intents that the importing distribution provider adds.
     * To export a service, a distribution provider must expand any qualified intents. Both the exporting and
     * importing distribution providers must recognize all intents before a service can be distributed.
     * The value of this property must be of type string and can be a ',' separated list.
     */
    constexpr const char * const SERVICE_INTENTS = "service.intents";
}