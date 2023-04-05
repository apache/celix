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

celix_resource_t* celix_resource_create();

void celix_resource_destroy(celix_resource_t* resource);

bool celix_resource_equals(const celix_resource_t* res1, const celix_resource_t* res2);

unsigned int celix_resource_hashCode(const celix_resource_t* res);

const celix_array_list_t* celix_resource_getCapabilities(const celix_resource_t* res, const char* ns);

const celix_array_list_t* celix_resource_getRequirements(const celix_resource_t* res, const char* ns);

bool celix_resource_addCapability(celix_resource_t* res, celix_capability_t* cap);

bool celix_resource_addRequirement(celix_resource_t* res, celix_requirement_t* req);


#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_RESOURCE_H
