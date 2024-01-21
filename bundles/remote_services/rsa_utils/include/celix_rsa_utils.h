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

#ifndef CELIX_CELIX_RSA_UTILS_H
#define CELIX_CELIX_RSA_UTILS_H

#include "celix_errno.h"
#include "celix_properties_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a typed service properties from endpoint properties.
 *
 * The conversion ensures that "service.ranking" and "bundle.id" (if present) are set as long values and that the
 * "service.version" (if present) is set as version.
 *
 * Note that the "service.id" long "service.bundleid" properties are set during service registration and
 * therefore not set by this function.
 *
 * @param[in] endpointProperties
 * @param[out] serviceProperties
 * @return CELIX_SUCCESS if successfully, CELIX_ENOMEM if out of memory.
 */
celix_status_t
celix_rsaUtils_createServicePropertiesFromEndpointProperties(const celix_properties_t* endpointProperties,
                                                             celix_properties_t** serviceProperties);

#ifdef __cplusplus
};
#endif

#endif // CELIX_CELIX_RSA_UTILS_H
