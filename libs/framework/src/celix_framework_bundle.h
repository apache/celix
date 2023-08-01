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

#ifndef CELIX_FRAMEWORK_BUNDLE_H_
#define CELIX_FRAMEWORK_BUNDLE_H_

#include "celix_errno.h"
#include "celix_bundle_context.h"
#include "framework_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The framework bundle bundle activator create.
 */
celix_status_t celix_frameworkBundle_create(celix_bundle_context_t* ctx, void** userData);

/**
 * @brief The framework bundle bundle activator start.
 */
celix_status_t celix_frameworkBundle_start(void* userData, celix_bundle_context_t* ctx);

/**
 * @brief The framework bundle bundle activator stop.
 */
celix_status_t celix_frameworkBundle_stop(void* userData, celix_bundle_context_t* ctx);

/**
 * @brief The framework bundle bundle activator destroy.
 */
celix_status_t celix_frameworkBundle_destroy(void* userData, celix_bundle_context_t* ctx);

/**
 * @brief The framework bundle framework event handler.
 * @note Part of the header for testing purposes.
 */
celix_status_t celix_frameworkBundle_handleFrameworkEvent(void* handle, framework_event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_BUNDLE_H_ */
