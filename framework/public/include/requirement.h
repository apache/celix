/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * requirement.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef REQUIREMENT_H_
#define REQUIREMENT_H_

typedef struct requirement *REQUIREMENT;

#include "capability.h"
#include "hash_map.h"
#include "version_range.h"

celix_status_t requirement_create(apr_pool_t *pool, HASH_MAP directives, HASH_MAP attributes, REQUIREMENT *requirement);
celix_status_t requirement_getVersionRange(REQUIREMENT requirement, VERSION_RANGE *range);
celix_status_t requirement_getTargetName(REQUIREMENT requirement, char **targetName);

celix_status_t requirement_isSatisfied(REQUIREMENT requirement, CAPABILITY capability, bool *inRange);

#endif /* REQUIREMENT_H_ */
