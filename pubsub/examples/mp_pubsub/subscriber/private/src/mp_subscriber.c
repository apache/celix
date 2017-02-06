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
 * mp_subscriber.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include "ew.h"
#include "ide.h"
#include "kinematics.h"
#include "mp_subscriber_private.h"

pubsub_receiver_pt subscriber_create(char* topics) {
	pubsub_receiver_pt sub = calloc(1,sizeof(*sub));
	sub->name = strdup(topics);
	return sub;
}


void subscriber_start(pubsub_receiver_pt subscriber){
	printf("MP_SUBSCRIBER: starting up...\n");
}

void subscriber_stop(pubsub_receiver_pt subscriber){
	printf("MP_SUBSCRIBER: stopping...\n");
}

void subscriber_destroy(pubsub_receiver_pt subscriber){
	if(subscriber->name!=NULL){
		free(subscriber->name);
	}
	subscriber->name=NULL;
	free(subscriber);
}

int pubsub_subscriber_recv(void* handle, const char* msgType, unsigned int msgTypeId, void* msg,pubsub_multipart_callbacks_t *callbacks, bool* release){

	unsigned int kin_msg_id = 0;
	unsigned int ide_msg_id = 0;
	unsigned int ew_msg_id = 0;

	if( callbacks->localMsgTypeIdForMsgType(callbacks->handle,(const char*)MSG_KINEMATICS_NAME,&kin_msg_id) != 0 ){
		printf("MP_SUBSCRIBER: Cannot retrieve msgId for message '%s'\n",MSG_KINEMATICS_NAME);
		return -1;
	}

	if( callbacks->localMsgTypeIdForMsgType(callbacks->handle,(const char*)MSG_IDE_NAME,&ide_msg_id) != 0 ){
		printf("MP_SUBSCRIBER: Cannot retrieve msgId for message '%s'\n",MSG_IDE_NAME);
		return -1;
	}

	if( callbacks->localMsgTypeIdForMsgType(callbacks->handle,(const char*)MSG_EW_NAME,&ew_msg_id) != 0 ){
		printf("MP_SUBSCRIBER: Cannot retrieve msgId for message '%s'\n",MSG_EW_NAME);
		return -1;
	}

	if(msgTypeId!=kin_msg_id){
		printf("MP_SUBSCRIBER: Multipart Message started with wrong message (expected %u, got %u)\n",msgTypeId,kin_msg_id);
		return -1;
	}

	kinematics_pt kin_data = (kinematics_pt)msg;

	void* ide_msg = NULL;
	callbacks->getMultipart(callbacks->handle,ide_msg_id,false,&ide_msg);

	void* ew_msg = NULL;
	callbacks->getMultipart(callbacks->handle,ew_msg_id,false,&ew_msg);

	if(kin_data==NULL){
		printf("MP_SUBSCRIBER: Unexpected NULL data for message '%s'\n",MSG_KINEMATICS_NAME);
	}
	else{
		printf("kin_data: pos=[%f, %f] occurrences=%d\n",kin_data->position.lat,kin_data->position.lon, kin_data->occurrences);
	}

	if(ide_msg==NULL){
		printf("MP_SUBSCRIBER: Unexpected NULL data for message '%s'\n",MSG_IDE_NAME);
	}
	else{
		ide_pt ide_data = (ide_pt)ide_msg;
		printf("ide_data: shape=%s\n",shape_tostring[(int)ide_data->shape]);
	}

	if(ew_msg==NULL){
		printf("MP_SUBSCRIBER: Unexpected NULL data for message '%s'\n",MSG_EW_NAME);
	}
	else{
		ew_data_pt ew_data = (ew_data_pt)ew_msg;
		printf("ew_data: area=%f color=%s\n",ew_data->area,color_tostring[(int)ew_data->color]);
	}

	printf("\n");

	return 0;

}
