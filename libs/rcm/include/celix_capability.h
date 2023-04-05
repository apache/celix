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

celix_capability_t* celix_capability_create(
        const celix_resource_t* resource,
        const char* ns);

void celix_capability_destroy(celix_capability_t* capability);

bool celix_capability_equals(const celix_capability_t* cap1, const celix_capability_t* cap2);

unsigned int celix_capability_hashCode(const celix_capability_t* cap);

const celix_resource_t* celix_capability_getResource(const celix_capability_t* cap);

const char* celix_capability_getNamespace(const celix_capability_t* cap);

const celix_properties_t* celix_capability_getAttributes(const celix_capability_t* cap);

const celix_properties_t* celix_capability_getDirectives(const celix_capability_t* cap);

const char* celix_capability_getAttribute(const celix_capability_t* cap, const char* key);

const char* celix_capability_getDirective(const celix_capability_t* cap, const char* key);

void celix_capability_addAttribute(celix_capability_t* cap, const char* key, const char* value);

void celix_capability_addAttributes(celix_capability_t* cap, const celix_properties_t* attributes);

void celix_capability_addDirective(celix_capability_t* cap, const char* key, const char* value);

void celix_capability_addDirectives(celix_capability_t* cap, const celix_properties_t* directives);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_CAPABILITY_H
