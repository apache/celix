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
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REQUIREMENT_H_
#define REQUIREMENT_H_

typedef struct requirement *requirement_t;

#include "capability.h"
#include "hash_map.h"
#include "version_range.h"

celix_status_t requirement_create(apr_pool_t *pool, hash_map_t directives, hash_map_t attributes, requirement_t *requirement);
celix_status_t requirement_getVersionRange(requirement_t requirement, version_range_t *range);
celix_status_t requirement_getTargetName(requirement_t requirement, char **targetName);

celix_status_t requirement_isSatisfied(requirement_t requirement, capability_t capability, bool *inRange);

#endif /* REQUIREMENT_H_ */
