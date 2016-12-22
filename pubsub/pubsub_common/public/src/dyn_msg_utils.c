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
 * dyn_msg_utils.c
 *
 *  \date       Nov 11, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "utils.h"
#include "dyn_message.h"

#include "dyn_msg_utils.h"

#define SYSTEM_BUNDLE_ARCHIVE_PATH 		"CELIX_FRAMEWORK_EXTENDER_PATH"

static char * getMsgDescriptionDir(bundle_pt bundle);
static void addMsgDescriptorsFromBundle(const char *root, bundle_pt bundle, hash_map_pt msgTypesMap);


unsigned int uintHash(const void * uintNum) {
	return *((unsigned int*)uintNum);
}

int uintEquals(const void * uintNum, const void * toCompare) {
	return ( (*((unsigned int*)uintNum)) == (*((unsigned int*)toCompare)) );
}

void fillMsgTypesMap(hash_map_pt msgTypesMap,bundle_pt bundle){

	char *root = NULL;
	char *metaInfPath = NULL;

	root = getMsgDescriptionDir(bundle);
	asprintf(&metaInfPath, "%s/META-INF/descriptors", root);

	addMsgDescriptorsFromBundle(root, bundle, msgTypesMap);
	addMsgDescriptorsFromBundle(metaInfPath, bundle, msgTypesMap);

	free(metaInfPath);
	if(root!=NULL){
		free(root);
	}
}

void emptyMsgTypesMap(hash_map_pt msgTypesMap)
{
	hash_map_iterator_pt iter = hashMapIterator_create(msgTypesMap);

	while(hashMapIterator_hasNext(iter)){
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		dynMessage_destroy( ((dyn_message_type *) hashMapEntry_getValue(entry)) );
	}
	hashMap_clear(msgTypesMap, true, false);
	hashMapIterator_destroy(iter);
}

static char * getMsgDescriptionDir(bundle_pt bundle)
{
	char *root = NULL;

	bool isSystemBundle = false;
	bundle_isSystemBundle(bundle, &isSystemBundle);

	if(isSystemBundle == true) {
		bundle_context_pt context;
		bundle_getContext(bundle, &context);

		const char *prop = NULL;

		bundleContext_getProperty(context, SYSTEM_BUNDLE_ARCHIVE_PATH, &prop);

		if(prop != NULL) {
			root = strdup(prop);
		} else {
			root = getcwd(NULL, 0);
		}
	} else {
	    char *dir;
		bundle_getEntry(bundle, ".", &dir);
		root = dir;
	}

	return root;
}


static void addMsgDescriptorsFromBundle(const char *root, bundle_pt bundle, hash_map_pt msgTypesMap)
{
	char path[128];
	struct dirent *entry = NULL;
	DIR *dir = opendir(root);

	if(dir) {
		entry = readdir(dir);
	}

	while (entry != NULL) {

		if (strstr(entry->d_name, ".descriptor") != NULL) {

			printf("DMU: Parsing entry '%s'\n", entry->d_name);

			memset(path,0,128);
			snprintf(path, 128, "%s/%s", root, entry->d_name);
			FILE *stream = fopen(path,"r");

			dyn_message_type* msgType = NULL;

			int rc = dynMessage_parse(stream, &msgType);
			if (rc == 0 && msgType!=NULL) {

				char* msgName = NULL;
				dynMessage_getName(msgType,&msgName);

				if(msgName!=NULL){
					unsigned int* msgId = malloc(sizeof(unsigned int));
					*msgId = utils_stringHash(msgName);
					hashMap_put(msgTypesMap,msgId,msgType);
				}

			}
			else{
				printf("DMU: cannot parse message from descriptor %s\n.",path);
			}
			fclose(stream);
		}
		entry = readdir(dir);
	}

	if(dir) {
		closedir(dir);
	}
}
