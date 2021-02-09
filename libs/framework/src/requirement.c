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

celix_status_t requirement_create(hash_map_pt directives, hash_map_pt attributes, requirement_pt *requirement) {
	celix_status_t status;

	*requirement = (requirement_pt) malloc(sizeof(**requirement));
	if (!*requirement) {
		status = CELIX_ENOMEM;
	} else {
		attribute_pt serviceAttribute = NULL;
		attribute_pt versionAttribute = NULL;

		(*requirement)->attributes = attributes;
		(*requirement)->directives = directives;
		(*requirement)->versionRange = NULL;

		serviceAttribute = (attribute_pt) hashMap_get(attributes, "service");
		status = attribute_getValue(serviceAttribute, &(*requirement)->targetName);
		if (status == CELIX_SUCCESS) {
			versionAttribute = (attribute_pt) hashMap_get(attributes, "version");
			if (versionAttribute != NULL) {
				char *versionStr = NULL;
				attribute_getValue(versionAttribute, &versionStr);
				status = versionRange_parse(versionStr, &(*requirement)->versionRange);
			} else {
				status = versionRange_createInfiniteVersionRange(&(*requirement)->versionRange);
			}
		}
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create requirement");

	return status;
}

celix_status_t requirement_destroy(requirement_pt requirement) {
	hash_map_iterator_pt attrIter = hashMapIterator_create(requirement->attributes);
	while (hashMapIterator_hasNext(attrIter)) {
		attribute_pt attr = hashMapIterator_nextValue(attrIter);
		hashMapIterator_remove(attrIter);
		attribute_destroy(attr);
	}
	hashMapIterator_destroy(attrIter);
	hashMap_destroy(requirement->attributes, false, false);
	hashMap_destroy(requirement->directives, false, false);

	requirement->attributes = NULL;
	requirement->directives = NULL;

	versionRange_destroy(requirement->versionRange);
	requirement->versionRange = NULL;

	free(requirement);

	return CELIX_SUCCESS;
}

celix_status_t requirement_getVersionRange(requirement_pt requirement, celix_version_range_t **range) {
	*range = requirement->versionRange;
	return CELIX_SUCCESS;
}

celix_status_t requirement_getTargetName(requirement_pt requirement, const char **targetName) {
	*targetName = requirement->targetName;
	return CELIX_SUCCESS;
}

celix_status_t requirement_isSatisfied(requirement_pt requirement, capability_pt capability, bool *inRange) {
	celix_status_t status;
	version_pt version = NULL;
	version_range_pt range = NULL;

	status = capability_getVersion(capability, &version);
	if (status == CELIX_SUCCESS) {
		status = requirement_getVersionRange(requirement, &range);
		if (status == CELIX_SUCCESS) {
			status = versionRange_isInRange(range, version, inRange);
		}
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot check if requirement is satisfied");

	return status;
}
