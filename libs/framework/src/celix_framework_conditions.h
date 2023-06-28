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

#ifndef CELIX_CELIX_FRAMEWORK_CONDITIONS_H_
#define CELIX_CELIX_FRAMEWORK_CONDITIONS_H_

#include "celix_errno.h"
#include "celix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the initial celix_condition services for the framework.
 *
 * The initial celix_condition services are:
 * - The true celix_condition service
 *
 * Will log an error if the initial celix_condition services administration objects or services registrations
 * fail.
 *
 * @param[in] framework The framework.
 */
void celix_frameworkConditions_registerInitialConditions(celix_framework_t* framework);

/**
 * @brief Register the framework ready celix_condition services for the framework.
 *
 * The framework ready celix_condition services are:
 * - The framework.ready celix_condition service
 *
 * Will log an error if the framework read celix_condition services administration objects or services registrations
 * fail.
 *
 * @param framework The framework.
 */
void celix_frameworkConditions_registerFrameworkReadyConditions(celix_framework_t* framework);

/**
 * @brief Unregister the intial and framework ready celix_condition services for the framework.
 * @param framework The framework.
 */
void celix_frameworkConditions_unregisterConditions(celix_framework_t* framework);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_FRAMEWORK_CONDITIONS_H_
