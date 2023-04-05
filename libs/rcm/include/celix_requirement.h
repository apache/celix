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

#include <stdbool.h>

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

celix_requirement_t* celix_requirement_create(
        celix_resource_t* resource,
        const char* ns,
        const char* filter);


void celix_requirement_destroy(celix_requirement_t* requirement);

bool celix_requirement_equals(const celix_requirement_t* req1, const celix_requirement_t* req2);

unsigned int celix_requirement_hashCode(const celix_requirement_t* req);

const celix_resource_t* celix_requirement_getResource(const celix_requirement_t* req);

const char* celix_requirement_getNamespace(const celix_requirement_t* req);

const char* celix_requirement_getFilter(const celix_requirement_t* req);

const celix_properties_t* celix_requirement_getDirectives(const celix_requirement_t* req);

const char* celix_requirement_getDirective(const celix_requirement_t* req, const char* key);

const celix_properties_t* celix_requirement_getAttributes(const celix_requirement_t* req);

const char* celix_requirement_getAttribute(const celix_requirement_t* req, const char* key);

void celix_requirement_addDirective(celix_requirement_t* req, const char* key, const char* value);

void celix_requirement_addDirectives(celix_requirement_t* req, const celix_properties_t* directives);

void celix_requirement_addAttribute(celix_requirement_t* req, const char* key, const char* value);

void celix_requirement_addAttributes(celix_requirement_t* req, const celix_properties_t* attributes);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_REQUIREMENT_H
