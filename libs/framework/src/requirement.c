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
/**
 * requirement.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "requirement_private.h"
#include "version_range.h"
#include "attribute.h"
#include "celix_log.h"

//LCOV_EXCL_START
celix_status_t requirement_create(hash_map_pt directives, hash_map_pt attributes, requirement_pt *requirement) {
    celix_status_t status = CELIX_FRAMEWORK_EXCEPTION;
    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create requirement");
    return status;
}

celix_status_t requirement_destroy(requirement_pt requirement) {
    return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t requirement_getVersionRange(requirement_pt requirement, celix_version_range_t **range) {
    return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t requirement_getTargetName(requirement_pt requirement, const char **targetName) {
    return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t requirement_isSatisfied(requirement_pt requirement, capability_pt capability, bool *inRange) {
    celix_status_t status = CELIX_FRAMEWORK_EXCEPTION;
    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot check if requirement is satisfied");
    return status;
}
//LCOV_EXCL_STOP
