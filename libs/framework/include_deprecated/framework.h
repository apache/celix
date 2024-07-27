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

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

#include "celix_errno.h"
#include "celix_bundle.h"
#include "celix_properties.h"
#include "bundle_context.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_create(celix_framework_t **framework, celix_properties_t *config);

/**
 * @brief Start the framework.
 * @note Not thread safe.
 * @param[in] framework The framework to start.
 * @return CELIX_SUCCESS if the framework is started.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_start(celix_framework_t *framework);

/**
 * @brief Stop the framework.
 * @note Not thread safe.
 * @param[in] framework The framework to stop.
 * @return CELIX_SUCCESS if the framework is stopped.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_stop(celix_framework_t *framework);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_destroy(celix_framework_t *framework);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_waitForStop(celix_framework_t *framework);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t framework_getFrameworkBundle(const celix_framework_t *framework, celix_bundle_t **bundle);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_bundle_context_t* framework_getContext(const celix_framework_t *framework);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_H_ */
