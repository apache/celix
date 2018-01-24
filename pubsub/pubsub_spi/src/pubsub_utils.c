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

char* pubsub_getScopeFromFilter(char* bundle_filter){

	char* scope = NULL;

	char* filter = strdup(bundle_filter);

	char* oc = strstr(filter,OSGI_FRAMEWORK_OBJECTCLASS);
	if(oc!=NULL){
		oc+=strlen(OSGI_FRAMEWORK_OBJECTCLASS)+1;
		if(strncmp(oc,PUBSUB_PUBLISHER_SERVICE_NAME,strlen(PUBSUB_PUBLISHER_SERVICE_NAME))==0){

			char* scopes = strstr(filter,PUBSUB_PUBLISHER_SCOPE);
			if(scopes!=NULL){

				scopes+=strlen(PUBSUB_PUBLISHER_SCOPE)+1;
				char* bottom=strchr(scopes,')');
				*bottom='\0';

				scope=strdup(scopes);
			} else {
			    scope=strdup(PUBSUB_PUBLISHER_SCOPE_DEFAULT);
			}
		}
	}

	free(filter);

	return scope;
}

char* pubsub_getTopicFromFilter(char* bundle_filter){

	char* topic = NULL;

	char* filter = strdup(bundle_filter);

	char* oc = strstr(filter,OSGI_FRAMEWORK_OBJECTCLASS);
	if(oc!=NULL){
		oc+=strlen(OSGI_FRAMEWORK_OBJECTCLASS)+1;
		if(strncmp(oc,PUBSUB_PUBLISHER_SERVICE_NAME,strlen(PUBSUB_PUBLISHER_SERVICE_NAME))==0){

			char* topics = strstr(filter,PUBSUB_PUBLISHER_TOPIC);
			if(topics!=NULL){

				topics+=strlen(PUBSUB_PUBLISHER_TOPIC)+1;
				char* bottom=strchr(topics,')');
				*bottom='\0';

				topic=strdup(topics);

			}
		}
	}

	free(filter);

	return topic;

}

array_list_pt pubsub_getTopicsFromString(char* string){

	array_list_pt topic_list = NULL;
	arrayList_create(&topic_list);

	char* topics = strdup(string);

	char* topic = strtok(topics,",;|# ");
	arrayList_add(topic_list,strdup(topic));

	while( (topic = strtok(NULL,",;|# ")) !=NULL){
		arrayList_add(topic_list,strdup(topic));
	}

	free(topics);

	return topic_list;

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

