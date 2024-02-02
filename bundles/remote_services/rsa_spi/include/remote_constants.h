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
/**
 * remote_constants.h
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_CONSTANTS_H_
#define REMOTE_CONSTANTS_H_

#define CELIX_RSA_SERVICE_EXPORTED_INTERFACES "service.exported.interfaces"
#define CELIX_RSA_ENDPOINT_FRAMEWORK_UUID "endpoint.framework.uuid"
#define CELIX_RSA_ENDPOINT_SERVICE_ID "endpoint.service.id"
#define CELIX_RSA_ENDPOINT_ID "endpoint.id"
#define CELIX_RSA_SERVICE_IMPORTED "service.imported"
#define CELIX_RSA_SERVICE_IMPORTED_CONFIGS "service.imported.configs"
#define CELIX_RSA_SERVICE_EXPORTED_CONFIGS "service.exported.configs"
#define CELIX_RSA_SERVICE_LOCATION "service.location"

/**
 * @brief It identify which types of remote service configurations are supported by a distribution provider.
 * @ref https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.remoteservices.html#i1708968
 */
#define CELIX_RSA_REMOTE_CONFIGS_SUPPORTED "remote.configs.supported"

/**
 * @breif It is a property of remote service admin service. It indicates whether the RSA implementation supports dynamic IP address fill-in for service exports.
 * Its type is boolean.If this property is not specified, it defaults to false.
 * @details If dynamic IP fill-in is supported, the RSA implementation should bind to ANY address (0.0.0.0). And the topology manager will create multiple endpoints for a single export registration based on the available network interfaces and an optional network selection configuration.
 */
#define CELIX_RSA_DYNAMIC_IP_SUPPORT "celix.rsa.dynamic.ip.support"

/**
 * @brief It is a replaceable property of endpoint. It indicates the dynamically determined IP address for the endpoint. Its type is string with comma-separated list of IP addresses.
 * @todo Use string array instead of comma-separated list.(depends on https://github.com/apache/celix/issues/674)
 */
#define CELIX_RSA_IP_ADDRESSES "celix.rsa.ip.addresses"

/**
 * @brief It is a property of endpoint. It indicates the port number of the endpoint for which a dynamic IP address is required. Its type is integer.
 */
#define CELIX_RSA_PORT "celix.rsa.port"

/**
 * @brief It is a replaceable property of endpoint. It indicates which network interface is used for exported endpoint exposure.
 * Its type is string. If this property is not specified, discovery implementation should select a proper exposure policy.
 */
#define CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE "celix.rsa.ifname"

/**
 * @brief It is a property of discovery endpoint listener service. It indicates whether the discovery implementation supports handling of interface-specific endpoints.
 * Its type is boolean. If this property is not specified, it defaults to false.
 * @details If handling interface-specific endpoints are supported, the discovery implementation will fill in the dynamic IP address to the property `CELIX_RSA_IP_ADDRESSES` of endpoint.
 */
#define CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT "celix.rsa.discovery.interface.specific.endpoints.support"

#endif /* REMOTE_CONSTANTS_H_ */
