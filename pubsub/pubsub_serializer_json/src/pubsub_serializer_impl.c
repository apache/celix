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
 * pubsub_serializer_impl.c
 *
 *  \date       Mar 24, 2017
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>

#include "utils.h"
#include "hash_map.h"
#include "bundle_context.h"

#include "log_helper.h"

#include "json_serializer.h"

#include "pubsub_serializer_impl.h"

#define SYSTEM_BUNDLE_ARCHIVE_PATH 		"CELIX_FRAMEWORK_EXTENDER_PATH"
#define MAX_PATH_LEN    1024

static char* pubsubSerializer_getMsgDescriptionDir(bundle_pt bundle);
static void pubsubSerializer_addMsgSerializerFromBundle(const char *root, bundle_pt bundle, hash_map_pt msgTypesMap);
static void pubsubSerializer_fillMsgSerializerMap(hash_map_pt msgTypesMap,bundle_pt bundle);

celix_status_t pubsubSerializer_create(bundle_context_pt context, pubsub_serializer_t** serializer) {
	celix_status_t status = CELIX_SUCCESS;

	*serializer = calloc(1, sizeof(**serializer));

	if (!*serializer) {
		status = CELIX_ENOMEM;
	}
	else{

		(*serializer)->bundle_context= context;

		if (logHelper_create(context, &(*serializer)->loghelper) == CELIX_SUCCESS) {
			logHelper_start((*serializer)->loghelper);
		}

	}

	return status;
}

celix_status_t pubsubSerializer_destroy(pubsub_serializer_t* serializer) {
	celix_status_t status = CELIX_SUCCESS;

	logHelper_stop(serializer->loghelper);
	logHelper_destroy(&serializer->loghelper);

	free(serializer);

	return status;
}

celix_status_t pubsubSerializer_createSerializerMap(pubsub_serializer_t* serializer, bundle_pt bundle, hash_map_pt* serializerMap) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_pt map = hashMap_create(NULL, NULL, NULL, NULL);

	if (map != NULL) {
		pubsubSerializer_fillMsgSerializerMap(map, bundle);
	} else {
		logHelper_log(serializer->loghelper, OSGI_LOGSERVICE_ERROR, "Cannot allocate memory for msg map");
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		*serializerMap = map;
	}
	return status;
}

celix_status_t pubsubSerializer_destroySerializerMap(pubsub_serializer_t* serializer, hash_map_pt serializerMap) {
	celix_status_t status = CELIX_SUCCESS;
	if (serializerMap == NULL) {
		return CELIX_ILLEGAL_ARGUMENT;
	}

	hash_map_iterator_t iter = hashMapIterator_construct(serializerMap);
	while (hashMapIterator_hasNext(&iter)) {
		pubsub_msg_serializer_t* msgSerializer = hashMapIterator_nextValue(&iter);
		dyn_message_type *dynMsg = (dyn_message_type*)msgSerializer->handle;
		dynMessage_destroy(dynMsg); //note msgSer->name and msgSer->version owned by dynType
		free(msgSerializer); //also contains the service struct.
	}

	hashMap_destroy(serializerMap, false, false);

	return status;
}


celix_status_t pubsubMsgSerializer_serialize(pubsub_msg_serializer_t* msgSerializer, const void* msg, void** out, size_t *outLen) {
	celix_status_t status = CELIX_SUCCESS;

	char *jsonOutput = NULL;
	dyn_type* dynType = NULL;
	dyn_message_type *dynMsg = (dyn_message_type*)msgSerializer->handle;
	dynMessage_getMessageType(dynMsg, &dynType);

	if (jsonSerializer_serialize(dynType, msg, &jsonOutput) != 0){
		status = CELIX_BUNDLE_EXCEPTION;
	}

	if (status == CELIX_SUCCESS) {
		*out = jsonOutput;
		*outLen = strlen(jsonOutput) + 1;
	}

	return status;
}

celix_status_t pubsubMsgSerializer_deserialize(pubsub_msg_serializer_t* msgSerializer, const void* input, size_t inputLen, void **out) {

	celix_status_t status = CELIX_SUCCESS;
	void *msg = NULL;
	dyn_type* dynType = NULL;
	dyn_message_type *dynMsg = (dyn_message_type*)msgSerializer->handle;
	dynMessage_getMessageType(dynMsg, &dynType);

	if (jsonSerializer_deserialize(dynType, (const char*)input, &msg) != 0) {
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else{
		*out = msg;
	}

	return status;
}

void pubsubMsgSerializer_freeMsg(pubsub_msg_serializer_t* msgSerializer, void *msg) {
	dyn_type* dynType = NULL;
	dyn_message_type *dynMsg = (dyn_message_type*)msgSerializer->handle;
	dynMessage_getMessageType(dynMsg, &dynType);
	if (dynType != NULL) {
		dynType_free(dynType, msg);
	}
}


static void pubsubSerializer_fillMsgSerializerMap(hash_map_pt msgSerializers, bundle_pt bundle) {
	char* root = NULL;
	char* metaInfPath = NULL;

	root = pubsubSerializer_getMsgDescriptionDir(bundle);

	if(root != NULL){
		asprintf(&metaInfPath, "%s/META-INF/descriptors", root);

		pubsubSerializer_addMsgSerializerFromBundle(root, bundle, msgSerializers);
		pubsubSerializer_addMsgSerializerFromBundle(metaInfPath, bundle, msgSerializers);

		free(metaInfPath);
		free(root);
	}
}

static char* pubsubSerializer_getMsgDescriptionDir(bundle_pt bundle)
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
		bundle_getEntry(bundle, ".", &root);
	}

	return root;
}


static void pubsubSerializer_addMsgSerializerFromBundle(const char *root, bundle_pt bundle, hash_map_pt msgSerializers)
{
	char path[MAX_PATH_LEN];
	struct dirent *entry = NULL;
	DIR *dir = opendir(root);

	if(dir) {
		entry = readdir(dir);
	}

	while (entry != NULL) {

		if (strstr(entry->d_name, ".descriptor") != NULL) {

			printf("DMU: Parsing entry '%s'\n", entry->d_name);

			snprintf(path, MAX_PATH_LEN, "%s/%s", root, entry->d_name);
			FILE *stream = fopen(path,"r");

			if (stream != NULL){
				dyn_message_type* msgType = NULL;

				int rc = dynMessage_parse(stream, &msgType);
				if (rc == 0 && msgType != NULL) {

					char* msgName = NULL;
					rc += dynMessage_getName(msgType,&msgName);

					version_pt msgVersion = NULL;
					rc += dynMessage_getVersion(msgType, &msgVersion);

					if(rc == 0 && msgName != NULL && msgVersion != NULL){

						unsigned int msgId = utils_stringHash(msgName);

						pubsub_msg_serializer_t *msgSerializer = calloc(1,sizeof(pubsub_msg_serializer_t));

						msgSerializer->handle = msgType;
						msgSerializer->msgId = msgId;
						msgSerializer->msgName = msgName;
						msgSerializer->msgVersion = msgVersion;
						msgSerializer->serialize = (void*) pubsubMsgSerializer_serialize;
						msgSerializer->deserialize = (void*) pubsubMsgSerializer_deserialize;
						msgSerializer->freeMsg = (void*) pubsubMsgSerializer_freeMsg;

						bool clash = hashMap_containsKey(msgSerializers, (void*)(uintptr_t)msgId);
						if (clash){
							printf("Cannot add msg %s. clash in msg id %d!!\n", msgName, msgId);
							free(msgSerializer);
							dynMessage_destroy(msgType);
						}
						else if (msgId != 0){
							printf("Adding %u : %s\n", msgId, msgName);
							hashMap_put(msgSerializers, (void*)(uintptr_t)msgId, msgSerializer);
						}
						else{
							printf("Error creating msg serializer\n");
							free(msgSerializer);
							dynMessage_destroy(msgType);
						}

					}
					else{
						printf("Cannot retrieve name and/or version from msg\n");
					}

				} else{
					printf("DMU: cannot parse message from descriptor %s\n.",path);
				}
				fclose(stream);
			}else{
				printf("DMU: cannot open descriptor file %s\n.",path);
			}

		}
		entry = readdir(dir);
	}

	if(dir) {
		closedir(dir);
	}
}
