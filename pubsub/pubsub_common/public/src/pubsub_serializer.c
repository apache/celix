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
 * pubsub_serializer_json.c
 *
 *  \date       Dec 7, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "pubsub_serializer.h"

#include "utils.h"
#include "json_serializer.h"
#include "dyn_msg_utils.h"
#include "dyn_type.h"
#include "string.h"
#include "dyn_message.h"
#include "dyn_common.h"

struct _pubsub_message_type {	/* _dyn_message_type */
	struct namvals_head header;
	struct namvals_head annotations;
	struct types_head types;
	dyn_type *msgType;
	version_pt msgVersion;
};

celix_status_t pubsubSerializer_serialize(pubsub_message_type *msgType, const void *input, void **output, int *outputLen){
	celix_status_t status = CELIX_SUCCESS;

	dyn_type *type = NULL;
	dynMessage_getMessageType((dyn_message_type *) msgType, &type);

	char *jsonOutput = NULL;
	int rc = jsonSerializer_serialize(type, (void *) input, &jsonOutput);
	if (rc != 0){
		status = CELIX_BUNDLE_EXCEPTION;
	}

	*output = (void *) jsonOutput;
	*outputLen = strlen(jsonOutput) + 1;

	return status;
}

celix_status_t pubsubSerializer_deserialize(pubsub_message_type *msgType, const void *input, void **output){
	celix_status_t status = CELIX_SUCCESS;

	dyn_type *type = NULL;
	dynMessage_getMessageType((dyn_message_type *) msgType, &type);

	void *textOutput = NULL;
	int rc = jsonSerializer_deserialize(type, (const char *) input, &textOutput);
	if (rc != 0){
		status = CELIX_BUNDLE_EXCEPTION;
	}

	*output = textOutput;

	return status;
}

unsigned int pubsubSerializer_hashCode(const char *string){
	return utils_stringHash(string);
}

version_pt pubsubSerializer_getVersion(pubsub_message_type *msgType){
	version_pt msgVersion = NULL;
	dynMessage_getVersion((dyn_message_type *) msgType, &msgVersion);
	return msgVersion;
}

char* pubsubSerializer_getName(pubsub_message_type *msgType){
	char *name = NULL;
	dynMessage_getName((dyn_message_type *) msgType, &name);
	return name;
}

void pubsubSerializer_fillMsgTypesMap(hash_map_pt msgTypesMap,bundle_pt bundle){
	fillMsgTypesMap(msgTypesMap, bundle);
}

void pubsubSerializer_emptyMsgTypesMap(hash_map_pt msgTypesMap){
	emptyMsgTypesMap(msgTypesMap);
}

void pubsubSerializer_freeMsg(pubsub_message_type *msgType, void *msg){
	dyn_type *type = NULL;
	dynMessage_getMessageType((dyn_message_type *) msgType, &type);
	dynType_free(type, msg);
}

