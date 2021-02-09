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

#ifndef REQUIREMENT_H_
#define REQUIREMENT_H_

typedef struct requirement *requirement_pt;

#include "capability.h"
#include "hash_map.h"
#include "celix_version_range.h"
#ifdef __cplusplus
extern "C" {
#endif

celix_status_t requirement_create(hash_map_pt directives, hash_map_pt attributes, requirement_pt *requirement);

celix_status_t requirement_destroy(requirement_pt requirement);

celix_status_t requirement_getVersionRange(requirement_pt requirement, celix_version_range_t **range);

celix_status_t requirement_getTargetName(requirement_pt requirement, const char **targetName);

celix_status_t requirement_isSatisfied(requirement_pt requirement, capability_pt capability, bool *inRange);

#ifdef __cplusplus
}
#endif

#endif /* REQUIREMENT_H_ */
