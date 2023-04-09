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

#ifndef CELIX_CELIX_RESOURCE_H
#define CELIX_CELIX_RESOURCE_H

#include <stdbool.h>

#include "celix_rcm_types.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file celix_resource.h
* @brief The celix_resource_t is a resource in the RCM.
*
* A resource is a collection of capabilities and requirements.
*
* @thread_safety none
*/

/**
 * @brief Creates a new resource.
 * @return A new resource. Returns NULL if the resource could not be created.
 */
celix_resource_t* celix_resource_create();

/**
 * @brief Destroys the resource.
 * @param[in] resource The resource to destroy. Can be NULL.
 */
void celix_resource_destroy(celix_resource_t* resource);

/**
 * @brief Returns the capabilities of the resource.
 *
 * Will return all resource capabilities if the provided namespace is NULL.
 *
 * @param[in] res The resource.
 * @param[in] ns The namespace of the capabilities. Can be NULL.
 * @return The capabilities of the resource. Will be an empty list if the resource has no capabilities or
 *         has no capabilities with the provided namespace.
 */
const celix_array_list_t* celix_resource_getCapabilities(const celix_resource_t* res, const char* ns);

/**
 * @brief Returns the requirements of the resource for the provided namespace.
 *
 * Will return all resource requirements if the provided namespace is NULL.
 *
 * @param[in] res The resource.
 * @param[in] ns The namespace of the requirements. Can be NULL.
 * @return The requirements of the resource. Will be an empty list if the resource has no requirements or
 *         has no requirements with the provided namespace.
 */
const celix_array_list_t* celix_resource_getRequirements(const celix_resource_t* res, const char* ns);

/**
 * @brief Adds a capability to the resource.
 *
 * The capability resource must be the same as this resource or a CELIX_ILLEGAL_ARGUMENT error is returned.
 *
 * @param[in] res The resource.
 * @param[in] cap The capability to add.
 * @return CELIX_SUCCESS if the capability was added successfully.
 */
celix_status_t celix_resource_addCapability(celix_resource_t* res, celix_capability_t* cap);

/**
 * @brief Adds a requirement to the resource.
 *
 * The requirement resource must be the same as this resource or a CELIX_ILLEGAL_ARGUMENT error is returned.
 *
 * @param[in] res The resource.
 * @param[in] req The requirement to add.
 * @return CELIX_SUCCESS if the requirement was added successfully.
 */
celix_status_t celix_resource_addRequirement(celix_resource_t* res, celix_requirement_t* req);


#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_RESOURCE_H
