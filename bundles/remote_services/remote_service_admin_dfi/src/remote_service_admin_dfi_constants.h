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

#ifndef CELIX_REMOTE_SERVICE_ADMIN_DFI_CONSTANTS_H
#define CELIX_REMOTE_SERVICE_ADMIN_DFI_CONSTANTS_H

#define RSA_PORT_KEY                    "RSA_PORT"
#define RSA_PORT_DEFAULT                8888

#define RSA_IP_KEY                      "RSA_IP"
#define RSA_IP_DEFAULT                  "127.0.0.1"

#define RSA_INTERFACE_KEY               "RSA_INTERFACE"

#define RSA_LOG_CALLS_KEY               "RSA_LOG_CALLS"
#define RSA_LOG_CALLS_DEFAULT           false
#define RSA_LOG_CALLS_FILE_KEY          "RSA_LOG_CALLS_FILE"
#define RSA_LOG_CALLS_FILE_DEFAULT      "stdout"

#define RSA_DFI_CONFIGURATION_TYPE      "org.amdatu.remote.admin.http"
#define RSA_DFI_ENDPOINT_URL            "org.amdatu.remote.admin.http.url"

/**
 * @brief Remote Service Admin DFI environment property (named "RSA_DFI_USE_CURL_SHARE_HANDLE") which specified
 * whether the RSA DFI should use curl's share handle.
 *
 * The curl share handle has a significant performance boost by sharing DNS, COOKIE en CONNECTIONS over multiple calls,
 * but can also introduce some issues (based on experience)
 *
 * The property is of the type boolean and the default is false
 */
#define RSA_DFI_USE_CURL_SHARE_HANDLE           "RSA_DFI_USE_CURL_SHARE_HANDLE"

/**
 * @brief Default value for the enviroment property RSA_DFI_USE_CURL_SHARE_HANDLE
 */
#define RSA_DFI_USE_CURL_SHARE_HANDLE_DEFAULT   false

/**
 * @brief Remote Service Admin DFI environment property (named "CELIX_RSA_BIND_ON_ALL_INTERFACES") which specifies
 * whether the RSA server is reachable from all network interfaces.
 * @details If set false, RSA server bind to the IP address configured by the user.
 * Otherwise, RSA server bind to 0.0.0.0.
 *
 * The property is of the type boolean and the default is true
 */
#define CELIX_RSA_BIND_ON_ALL_INTERFACES "CELIX_RSA_BIND_ON_ALL_INTERFACES"

/**
 * @brief Default value for the property CELIX_RSA_BIND_ON_ALL_INTERFACES
 */
#define CELIX_RSA_BIND_ON_ALL_INTERFACES_DEFAULT true

/**
 * @brief Remote Service Admin DFI environment property, it indicates whether the RSA_DFI implementation supports dynamic IP address fill-in for service exports.
 * Its type is boolean.If this property is not specified, it defaults to false.
 *
 */
#define CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT "CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT"

/**
 * @brief Default value for the property CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT
 */
#define CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT_DEFAULT false


#endif //CELIX_REMOTE_SERVICE_ADMIN_DFI_CONSTANTS_H
