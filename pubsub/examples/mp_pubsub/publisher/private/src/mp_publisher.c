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
 * mp_publisher.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "service_tracker.h"
#include "celix_threads.h"

#include "ew.h"
#include "ide.h"
#include "kinematics.h"
#include "mp_publisher_private.h"


static bool stop=false;
static celix_thread_t tid;

static double randDouble(double min, double max){
	double ret = min + (((double)random()) / (((double)RAND_MAX)/(max-min))) ;
	return ret;
}

static unsigned int randInt(unsigned int min, unsigned int max){
	double scaled = ((double)random())/((double)RAND_MAX);
	return (max - min +1)*scaled + min;
}

static void* send_thread(void* arg){

	send_thread_struct_pt st_struct = (send_thread_struct_pt)arg;

	pubsub_publisher_pt publish_svc = (pubsub_publisher_pt)st_struct->service;

	unsigned int kin_msg_id = 0;
	unsigned int ide_msg_id = 0;
	unsigned int ew_msg_id = 0;

	if( publish_svc->localMsgTypeIdForMsgType(publish_svc->handle,(const char*)MSG_KINEMATICS_NAME,&kin_msg_id) != 0 ){
		printf("MP_PUBLISHER: Cannot retrieve msgId for message '%s'\n",MSG_KINEMATICS_NAME);
		return NULL;
	}

	if( publish_svc->localMsgTypeIdForMsgType(publish_svc->handle,(const char*)MSG_IDE_NAME,&ide_msg_id) != 0 ){
		printf("MP_PUBLISHER: Cannot retrieve msgId for message '%s'\n",MSG_IDE_NAME);
		return NULL;
	}

	if( publish_svc->localMsgTypeIdForMsgType(publish_svc->handle,(const char*)MSG_EW_NAME,&ew_msg_id) != 0 ){
		printf("MP_PUBLISHER: Cannot retrieve msgId for message '%s'\n",MSG_EW_NAME);
		return NULL;
	}

	kinematics_pt kin_data = calloc(1,sizeof(*kin_data));
	ide_pt ide_data = calloc(1,sizeof(*ide_data));
	ew_data_pt ew_data = calloc(1,sizeof(*ew_data));

	unsigned int counter = 1;

	while(stop==false){
		kin_data->position.lat = randDouble(MIN_LAT,MAX_LAT);
		kin_data->position.lon = randDouble(MIN_LON,MAX_LON);
		kin_data->occurrences = randInt(MIN_OCCUR,MAX_OCCUR);
		publish_svc->sendMultipart(publish_svc->handle,kin_msg_id,kin_data, PUBSUB_PUBLISHER_FIRST_MSG);
		printf("Track#%u kin_data: pos=[%f, %f] occurrences=%d\n",counter,kin_data->position.lat,kin_data->position.lon, kin_data->occurrences);

		ide_data->shape = (shape_e)randInt(0,LAST_SHAPE-1);
		publish_svc->sendMultipart(publish_svc->handle,ide_msg_id,ide_data, PUBSUB_PUBLISHER_PART_MSG);
		printf("Track#%u ide_data: shape=%s\n",counter,shape_tostring[(int)ide_data->shape]);

		ew_data->area = randDouble(MIN_AREA,MAX_AREA);
		ew_data->color = (color_e)randInt(0,LAST_COLOR-1);
		publish_svc->sendMultipart(publish_svc->handle,ew_msg_id,ew_data, PUBSUB_PUBLISHER_LAST_MSG);
		printf("Track#%u ew_data: area=%f color=%s\n",counter,ew_data->area,color_tostring[(int)ew_data->color]);

		printf("\n");
		sleep(2);
		counter++;
	}

	free(ew_data);
	free(ide_data);
	free(kin_data);
	free(st_struct);

	return NULL;

}

pubsub_sender_pt publisher_create(array_list_pt trackers,const char* ident,long bundleId) {
	pubsub_sender_pt publisher = malloc(sizeof(*publisher));

	publisher->trackers = trackers;
	publisher->ident = ident;
	publisher->bundleId = bundleId;

	return publisher;
}

void publisher_start(pubsub_sender_pt client) {
	printf("MP_PUBLISHER: starting up...\n");
}

void publisher_stop(pubsub_sender_pt client) {
	printf("MP_PUBLISHER: stopping...\n");
}

void publisher_destroy(pubsub_sender_pt client) {
	client->trackers = NULL;
	client->ident = NULL;
	free(client);
}

celix_status_t publisher_publishSvcAdded(void * handle, service_reference_pt reference, void * service){
	pubsub_publisher_pt publish_svc = (pubsub_publisher_pt)service;
	pubsub_sender_pt manager = (pubsub_sender_pt)handle;

	printf("MP_PUBLISHER: new publish service exported (%s).\n",manager->ident);

	send_thread_struct_pt data = calloc(1,sizeof(struct send_thread_struct));
	data->service = publish_svc;
	data->publisher = manager;

	celixThread_create(&tid,NULL,send_thread,(void*)data);
	return CELIX_SUCCESS;
}

celix_status_t publisher_publishSvcRemoved(void * handle, service_reference_pt reference, void * service){
	//publish_service_pt publish_svc = (publish_service_pt)service;
	pubsub_sender_pt manager = (pubsub_sender_pt)handle;
	printf("MP_PUBLISHER: publish service unexported (%s)!\n",manager->ident);
	stop=true;
	celixThread_join(tid,NULL);
	return CELIX_SUCCESS;
}
