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
 * capability.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "capability.h"
#include "attribute.h"

struct capability {
	char * serviceName;
	module_t module;
	version_t version;
	hash_map_t attributes;
	hash_map_t directives;
};

apr_status_t capability_destroy(void *capabilityP);

celix_status_t capability_create(apr_pool_t *pool, module_t module, hash_map_t directives, hash_map_t attributes, capability_t *capability) {
	celix_status_t status = CELIX_SUCCESS;
	*capability = (capability_t) apr_palloc(pool, sizeof(**capability));
	if (!*capability) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *capability, capability_destroy);

		(*capability)->module = module;
		(*capability)->attributes = attributes;
		(*capability)->directives = directives;

		(*capability)->version = NULL;
		status = version_createEmptyVersion(pool, &(*capability)->version);
		if (status == CELIX_SUCCESS) {
			attribute_t versionAttribute = NULL;
			attribute_t serviceAttribute = (attribute_t) hashMap_get(attributes, "service");
			status = attribute_getValue(serviceAttribute, &(*capability)->serviceName);
			if (status == CELIX_SUCCESS) {
				versionAttribute = (attribute_t) hashMap_get(attributes, "version");
				if (versionAttribute != NULL) {
					char *versionStr = NULL;
					attribute_getValue(versionAttribute, &versionStr);
					(*capability)->version = NULL;
					status = version_createVersionFromString(pool, versionStr, &(*capability)->version);
				}
			}
		}
	}

	return status;
}

apr_status_t capability_destroy(void *capabilityP) {
	capability_t capability = capabilityP;

	hash_map_iterator_t attrIter = hashMapIterator_create(capability->attributes);
	while (hashMapIterator_hasNext(attrIter)) {
		attribute_t attr = hashMapIterator_nextValue(attrIter);
		hashMapIterator_remove(attrIter);
	}
	hashMapIterator_destroy(attrIter);
	hashMap_destroy(capability->attributes, false, false);
	hashMap_destroy(capability->directives, false, false);

	capability->attributes = NULL;
	capability->directives = NULL;
	capability->module = NULL;
	capability->version = NULL;

	return APR_SUCCESS;
}

celix_status_t capability_getServiceName(capability_t capability, char **serviceName) {
	*serviceName = capability->serviceName;
	return CELIX_SUCCESS;
}

celix_status_t capability_getVersion(capability_t capability, version_t *version) {
	*version = capability->version;
	return CELIX_SUCCESS;
}

celix_status_t capability_getModule(capability_t capability, module_t *module) {
	*module = capability->module;
	return CELIX_SUCCESS;
}
