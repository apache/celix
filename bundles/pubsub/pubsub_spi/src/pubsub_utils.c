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
 * pubsub_utils.c
 *
 *  \date       Sep 24, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <string.h>
#include <stdlib.h>

#include "constants.h"

#include "pubsub_common.h"
#include "pubsub/publisher.h"
#include "pubsub_utils.h"

#include "array_list.h"
#include "bundle.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_KEYBUNDLE_LENGTH 256


celix_status_t pubsub_getPubSubInfoFromFilter(const char* filterstr, char **topicOut, char **scopeOut) {
	celix_status_t status = CELIX_SUCCESS;
	const char *topic = NULL;
	const char *scope = NULL;
	const char *objectClass = NULL;
	celix_filter_t *filter = filter_create(filterstr);
	if (filter != NULL) {
		if (filter->operand == CELIX_FILTER_OPERAND_AND) { //only and pubsub filter valid (e.g. (&(objectClass=pubsub_publisher)(topic=exmpl))
			array_list_t *attributes = filter->children;
			unsigned int i;
			unsigned int size = arrayList_size(attributes);
			for (i = 0; i < size; ++i) {
				filter_t *attr = arrayList_get(attributes, i);
				if (attr->operand == CELIX_FILTER_OPERAND_EQUAL) {
					if (strncmp(OSGI_FRAMEWORK_OBJECTCLASS, attr->attribute, 128) == 0) {
						objectClass = attr->value;
					} else if (strncmp(PUBSUB_PUBLISHER_TOPIC, attr->attribute, 128) == 0) {
						topic = attr->value;
					} else if (strncmp(PUBSUB_PUBLISHER_SCOPE, attr->attribute, 128) == 0) {
						scope = attr->value;
					}
				}
			}
		}
	}

	if (topic != NULL && objectClass != NULL && strncmp(objectClass, PUBSUB_PUBLISHER_SERVICE_NAME, 128) == 0) {
		//NOTE topic must be present, scope can be present in the filter.
		*topicOut = strdup(topic);
                if (scope != NULL) {
		    *scopeOut = strdup(scope);
		} else {
		    *scopeOut = NULL;
		}
	} else {
		*topicOut = NULL;
		*scopeOut = NULL;
	}

	if (filter != NULL) {
             filter_destroy(filter);
        }
	return status;
}


/**
 * Loop through all bundles and look for the bundle with the keys inside.
 * If no key bundle found, return NULL
 *
 * Caller is responsible for freeing the object
 */
char* pubsub_getKeysBundleDir(bundle_context_pt ctx)
{
	array_list_pt bundles = NULL;
	bundleContext_getBundles(ctx, &bundles);
	int nrOfBundles = arrayList_size(bundles);
	long bundle_id = -1;
	char* result = NULL;

	for (int i = 0; i < nrOfBundles; i++){
		bundle_pt b = arrayList_get(bundles, i);

		/* Skip bundle 0 (framework bundle) since it has no path nor revisions */
		bundle_getBundleId(b, &bundle_id);
		if(bundle_id==0){
			continue;
		}

		char* dir = NULL;
		bundle_getEntry(b, ".", &dir);

		char cert_dir[MAX_KEYBUNDLE_LENGTH];
		snprintf(cert_dir, MAX_KEYBUNDLE_LENGTH, "%s/META-INF/keys", dir);

		struct stat s;
		int err = stat(cert_dir, &s);
		if (err != -1){
			if (S_ISDIR(s.st_mode)){
				result = dir;
				break;
			}
		}

		free(dir);
	}

	arrayList_destroy(bundles);

	return result;
}

