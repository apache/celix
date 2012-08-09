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
	MODULE module;
	VERSION version;
	HASH_MAP attributes;
	HASH_MAP directives;
};

apr_status_t capability_destroy(void *capabilityP);

celix_status_t capability_create(apr_pool_t *pool, MODULE module, HASH_MAP directives, HASH_MAP attributes, CAPABILITY *capability) {
	celix_status_t status = CELIX_SUCCESS;
	*capability = (CAPABILITY) apr_palloc(pool, sizeof(**capability));
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
			ATTRIBUTE versionAttribute = NULL;
			ATTRIBUTE serviceAttribute = (ATTRIBUTE) hashMap_get(attributes, "service");
			status = attribute_getValue(serviceAttribute, &(*capability)->serviceName);
			if (status == CELIX_SUCCESS) {
				versionAttribute = (ATTRIBUTE) hashMap_get(attributes, "version");
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
	CAPABILITY capability = capabilityP;

	HASH_MAP_ITERATOR attrIter = hashMapIterator_create(capability->attributes);
	while (hashMapIterator_hasNext(attrIter)) {
		ATTRIBUTE attr = hashMapIterator_nextValue(attrIter);
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

celix_status_t capability_getServiceName(CAPABILITY capability, char **serviceName) {
	*serviceName = capability->serviceName;
	return CELIX_SUCCESS;
}

celix_status_t capability_getVersion(CAPABILITY capability, VERSION *version) {
	*version = capability->version;
	return CELIX_SUCCESS;
}

celix_status_t capability_getModule(CAPABILITY capability, MODULE *module) {
	*module = capability->module;
	return CELIX_SUCCESS;
}
