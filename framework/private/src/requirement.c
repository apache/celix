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
 * requirement.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "requirement.h"
#include "version_range.h"
#include "attribute.h"

struct requirement {
	char * targetName;
	version_range_t versionRange;
	hash_map_t attributes;
	hash_map_t directives;
};

apr_status_t requirement_destroy(void *requirementP);

celix_status_t requirement_create(apr_pool_t *pool, hash_map_t directives, hash_map_t attributes, requirement_t *requirement) {
	celix_status_t status = CELIX_SUCCESS;

	*requirement = (requirement_t) apr_palloc(pool, sizeof(**requirement));
	if (!*requirement) {
		status = CELIX_ENOMEM;
	} else {
		attribute_t serviceAttribute = NULL;
		attribute_t versionAttribute = NULL;

		apr_pool_pre_cleanup_register(pool, *requirement, requirement_destroy);

		(*requirement)->attributes = attributes;
		(*requirement)->directives = directives;

		serviceAttribute = (attribute_t) hashMap_get(attributes, "service");
		status = attribute_getValue(serviceAttribute, &(*requirement)->targetName);
		if (status == CELIX_SUCCESS) {
			(*requirement)->versionRange = NULL;
			status = versionRange_createInfiniteVersionRange(pool, &(*requirement)->versionRange);
			if (status == CELIX_SUCCESS) {
				versionAttribute = (attribute_t) hashMap_get(attributes, "version");
				if (versionAttribute != NULL) {
					char *versionStr = NULL;
					attribute_getValue(versionAttribute, &versionStr);
					(*requirement)->versionRange = NULL;
					status = versionRange_parse(pool, versionStr, &(*requirement)->versionRange);
				}
			}
		}
	}

	return status;
}

apr_status_t requirement_destroy(void *requirementP) {
	requirement_t requirement = requirementP;
	hash_map_iterator_t attrIter = hashMapIterator_create(requirement->attributes);
	while (hashMapIterator_hasNext(attrIter)) {
		attribute_t attr = hashMapIterator_nextValue(attrIter);
		hashMapIterator_remove(attrIter);
	}
	hashMapIterator_destroy(attrIter);
	hashMap_destroy(requirement->attributes, false, false);
	hashMap_destroy(requirement->directives, false, false);

	requirement->attributes = NULL;
	requirement->directives = NULL;
	requirement->versionRange = NULL;

	return APR_SUCCESS;
}

celix_status_t requirement_getVersionRange(requirement_t requirement, version_range_t *range) {
	*range = requirement->versionRange;
	return CELIX_SUCCESS;
}

celix_status_t requirement_getTargetName(requirement_t requirement, char **targetName) {
	*targetName = requirement->targetName;
	return CELIX_SUCCESS;
}

celix_status_t requirement_isSatisfied(requirement_t requirement, capability_t capability, bool *inRange) {
	celix_status_t status = CELIX_SUCCESS;
	version_t version = NULL;
	version_range_t range = NULL;

	status = capability_getVersion(capability, &version);
	if (status == CELIX_SUCCESS) {
		status = requirement_getVersionRange(requirement, &range);
		if (status == CELIX_SUCCESS) {
			status = versionRange_isInRange(range, version, inRange);
		}
	}

	return status;
}
