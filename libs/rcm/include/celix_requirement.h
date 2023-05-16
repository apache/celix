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

#ifndef CELIX_CELIX_REQUIREMENT_H
#define CELIX_CELIX_REQUIREMENT_H

#include "celix_rcm_types.h"
#include "celix_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file celix_requirement.h
* @brief The celix_requirement_t is a requirement for a capability.
*
* A requirement is a requirement for a capability. A requirement has a namespace, a filter and a set of directives and attributes.
* The namespace is used to identify the type of requirement. The filter is used to filter the capabilities.
* The directives and attributes are used to configure the requirement.
*
* @thread_safety none
*/

/**
 * @brief Creates a new requirement.
 *
 * In case of a error, an error message is added to celix_err.
 *
 * @param[in] resource The resource which contains the requirement. Can be NULL.
 * @param[in] ns The namespace of the requirement.
 * @param[in] filter The filter of the requirement. Can be NULL.
 * @return The new requirement.
 * @retval NULL If an error occurred.
 */
celix_requirement_t* celix_requirement_create(
        celix_resource_t* resource,
        const char* ns,
        const char* filter);


/**
 * @brief Destroys the requirement.
 * @param[in] requirement The requirement to destroy. Can be NULL.
 */
void celix_requirement_destroy(celix_requirement_t* requirement);

/**
 * @brief Check if 2 requirements are equal.
 */
bool celix_requirement_equals(const celix_requirement_t* req1, const celix_requirement_t* req2);

/**
 * @brief Returns the hash code of the requirement.
 */
unsigned int celix_requirement_hashCode(const celix_requirement_t* req);

/**
 * @brief Returns the resource which contains the requirement.
 * @param[in] req The requirement.
 * @return The resource which contains the requirement or NULL if the requirement is not part of a resource.
 */
const celix_resource_t* celix_requirement_getResource(const celix_requirement_t* req);

/**
 * @brief Returns the namespace of the requirement.
 * @param[in] req The requirement.
 * @return The namespace of the requirement.
 */
const char* celix_requirement_getNamespace(const celix_requirement_t* req);

/**
 * @brief Returns the filter of the requirement.
 * @param[in] req The requirement.
 * @return The filter of the requirement.
 */
const char* celix_requirement_getFilter(const celix_requirement_t* req);

/**
 * @brief Returns the directives of the requirement.
 * @param[in] req The requirement.
 * @return The directives of the requirement. Will be an empty properties if no directives are set.
 */
const celix_properties_t* celix_requirement_getDirectives(const celix_requirement_t* req);

/**
 * @brief Returns the value of the directive with the given key.
 * @param[in] req The requirement.
 * @param[in] key The key of the directive.
 * @return The value of the directive or NULL if the directive is not set.
 */
const char* celix_requirement_getDirective(const celix_requirement_t* req, const char* key);

/**
 * @brief Returns the attributes of the requirement.
 * @param[in] req The requirement.
 * @return The attributes of the requirement. Will be an empty properties if no attributes are set.
 */
const celix_properties_t* celix_requirement_getAttributes(const celix_requirement_t* req);

/**
 *  @brief Returns the value of the attribute with the given key.
 * @param[in] req The requirement.
 * @param[in] key The key of the attribute.
 * @return The value of the attribute or NULL if the attribute is not set.
 */
const char* celix_requirement_getAttribute(const celix_requirement_t* req, const char* key);

/**
 * @brief Add a new directive to the requirement.
 *
 * Note it replaces an existing directive if the key is already set.
 *
 * @param[in] req The requirement.
 * @param[in] key The key of the directive.
 * @param[in] value The value of the directive.
 */
void celix_requirement_addDirective(celix_requirement_t* req, const char* key, const char* value);

/**
 * @brief Add a new directives to the requirement.
 *
 * Note it replaces existing directives if some of the provided keys are already set.
 *
 * @param[in] req The requirement.
 * @param[in] directives The directives to add.
 *                       Note the directives are copied, so the caller is still owner of the properties.
 */
void celix_requirement_addDirectives(celix_requirement_t* req, const celix_properties_t* directives);

/**
 * @brief Add a new attribute to the requirement.
 *
 * Note it replaces an existing attribute if the key is already set.
 *
 * @param[in] req The requirement.
 * @param[in] key The key of the attribute.
 * @param[in] value The value of the attribute.
 */
void celix_requirement_addAttribute(celix_requirement_t* req, const char* key, const char* value);

/**
 * @brief Add a new attributes to the requirement.
 *
 * Note it replaces existing attributes if some of the provided keys are already set.
 *
 * @param[in] req The requirement.
 * @param[in] attributes The attributes to add.
 *                       Note the attributes are copied, so the caller is still owner of the properties.
 */
void celix_requirement_addAttributes(celix_requirement_t* req, const celix_properties_t* attributes);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_REQUIREMENT_H
