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

#ifndef CELIX_COMPONENTS_READY_H_
#define CELIX_COMPONENTS_READY_H_

#include "celix_bundle_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_components_ready_check celix_components_ready_check_t;

/**
 * @brief Creates a new components ready check.
 * @param ctx The bundle context.
 */
celix_components_ready_check_t* celix_componentsReadyCheck_create(celix_bundle_context_t * ctx);

/**
 * @brief Destroys the components ready check.
 */
void celix_componentsReadyCheck_destroy(celix_components_ready_check_t* rdy);

/**
 * @brief Sets the framework ready condition.
 * @note Part of the header for testing purposes.
 */
void celix_componentReadyCheck_setFrameworkReadySvc(void* handle, void* svc);

/**
 * @brief The scheduled event callback for the components.ready check.
 * @note Part of the header for testing purposes.
 */
void celix_componentReadyCheck_registerCondition(celix_components_ready_check_t* rdy);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_COMPONENTS_READY_H_ */