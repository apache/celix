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

#ifndef CELIX_CELIX_CAPABILITY_H
#define CELIX_CELIX_CAPABILITY_H

#include <stdbool.h>

#include "celix_rcm_types.h"
#include "celix_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file celix_capability.h
* @brief The celix_capability_t is a struct which represents a capability.
*
* @thread_safety none
*/

/**
 * @brief Creates a new capability.
 * @param[in] resource The resource which contains the capability. Can be NULL.
 * @param[in] ns The namespace of the capability.
 * @return The new capability or NULL if an error occurred.
 */
celix_capability_t* celix_capability_create(
        const celix_resource_t* resource,
        const char* ns);

/**
 * @brief Destroys the capability.
 * @param[in] capability The capability to destroy. Can be NULL.
 */
void celix_capability_destroy(celix_capability_t* capability);

/**
 * @brief Check if 2 capabilities are equal.
 */
bool celix_capability_equals(const celix_capability_t* cap1, const celix_capability_t* cap2);

/**
 * @brief Returns the hash code of the capability.
 */
unsigned int celix_capability_hashCode(const celix_capability_t* cap);

/**
 * @brief Returns the resource which contains the capability.
 * @param[in] cap The capability.
 * @return The resource or NULL if no resource is set.
 */
const celix_resource_t* celix_capability_getResource(const celix_capability_t* cap);

/**
 * @brief Returns the namespace of the capability.
 * @param[in] cap The capability.
 * @return The namespace of the capability.
 */
const char* celix_capability_getNamespace(const celix_capability_t* cap);

/**
 * @brief Returns the attributes of the capability.
 * @param[in] cap The capability.
 * @return The attributes of the capability. Will be an empty properties if no attributes are set.
 */
const celix_properties_t* celix_capability_getAttributes(const celix_capability_t* cap);

/**
 * @brief Returns the directives of the capability.
 * @param[in] cap The capability.
 * @return The directives of the capability. Will be an empty properties if no directives are set.
 */
const celix_properties_t* celix_capability_getDirectives(const celix_capability_t* cap);

/**
 * @brief Returns the value of the attribute with the given key.
 * @param[in] cap The capability.
 * @param[in] key The key of the attribute.
 * @return The value of the attribute or NULL if the attribute is not set.
 */
const char* celix_capability_getAttribute(const celix_capability_t* cap, const char* key);

/**
 * @brief Returns the value of the directive with the given key.
 * @param[in] cap The capability.
 * @param[in] key The key of the directive.
 * @return The value of the directive or NULL if the directive is not set.
 */
const char* celix_capability_getDirective(const celix_capability_t* cap, const char* key);

/**
 * @brief Add a new attribute to the capability.
 *
 * Note replaces an existing attribute is the key is already set.
 *
 * @param[in] cap The capability.
 * @param[in] key The key of the attribute.
 * @param[in] value The value of the attribute.
 */
void celix_capability_addAttribute(celix_capability_t* cap, const char* key, const char* value);

/**
 * @brief Add a new attributes to the capability.
 *
 * Note replaces existing attributes is some of the provided keys are already set.
 *
 * @param[in] cap The capability.
 * @param[in] attributes The attributes to add.
 *                       Note the attributes are copied, so the caller is still owner of the properties.
 */
void celix_capability_addAttributes(celix_capability_t* cap, const celix_properties_t* attributes);

/**
 * @brief Add a new directive to the capability.
 *
 * Note replaces an existing directive is the key is already set.
 *
 * @param[in] cap The capability.
 * @param[in] key The key of the directive.
 * @param[in] value The value of the directive.
 */
void celix_capability_addDirective(celix_capability_t* cap, const char* key, const char* value);

/**
 * @brief Add a new directives to the capability.
 *
 * Note replaces existing directives is some of the provided keys are already set.
 *
 * @param[in] cap The capability.
 * @param[in] directives The directives to add.
 *                       Note the directives are copied, so the caller is still owner of the properties.
 */
void celix_capability_addDirectives(celix_capability_t* cap, const celix_properties_t* directives);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_CAPABILITY_H
