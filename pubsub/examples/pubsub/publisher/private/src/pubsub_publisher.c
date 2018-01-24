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
 * pubsub_publisher.c
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

#include "poi.h"

#include "pubsub_publisher_private.h"

static bool stop=false;

static double randCoordinate(double min, double max){

	double ret = min + (((double)random()) / (((double)RAND_MAX)/(max-min))) ;

	return ret;

}

static void* send_thread(void* arg){

	send_thread_struct_pt st_struct = (send_thread_struct_pt)arg;

	pubsub_publisher_pt publish_svc = (pubsub_publisher_pt)st_struct->service;
	pubsub_sender_pt publisher = (pubsub_sender_pt)st_struct->publisher;

	char fwUUID[9];
	memset(fwUUID,0,9);
	memcpy(fwUUID,publisher->ident,8);

	//poi_t point = calloc(1,sizeof(*point));
	location_t place = calloc(1,sizeof(*place));

	char* desc = calloc(64,sizeof(char));
	snprintf(desc,64,"fw-%s [TID=%lu]", fwUUID, (unsigned long)pthread_self());

	char* name = calloc(64,sizeof(char));
	snprintf(name,64,"Bundle#%ld",publisher->bundleId);

	place->name = name;
	place->description = desc;
	place->extra = "DONT PANIC";
	printf("TOPIC : %s\n",st_struct->topic);
	unsigned int msgId = 0;
	if( publish_svc->localMsgTypeIdForMsgType(publish_svc->handle,st_struct->topic,&msgId) == 0 ){

		while(stop==false){
			place->position.lat = randCoordinate(MIN_LAT,MAX_LAT);
			place->position.lon = randCoordinate(MIN_LON,MAX_LON);
			int nr_char = (int)randCoordinate(5,100000);
			place->data = calloc(nr_char, 1);
			for(int i = 0; i < (nr_char-1); i++) {
				place->data[i] = i%10 + '0';
			}
			if(publish_svc->send) {
				if(publish_svc->send(publish_svc->handle,msgId,place)==0){
					printf("Sent %s [%f, %f] (%s, %s) data len = %d\n",st_struct->topic, place->position.lat, place->position.lon,place->name,place->description, nr_char);
				}
			} else {
				printf("No send for %s\n", st_struct->topic);
			}

			free(place->data);
			sleep(2);
		}
	}
	else{
		printf("PUBLISHER: Cannot retrieve msgId for message '%s'\n",MSG_POI_NAME);
	}

	free(place->description);
	free(place->name);
	free(place);

	free(st_struct);


	return NULL;

}

pubsub_sender_pt publisher_create(array_list_pt trackers,const char* ident,long bundleId) {
	pubsub_sender_pt publisher = malloc(sizeof(*publisher));

	publisher->trackers = trackers;
	publisher->ident = ident;
	publisher->bundleId = bundleId;
	publisher->tid_map = hashMap_create(NULL, NULL, NULL, NULL);

	return publisher;
}

void publisher_start(pubsub_sender_pt client) {
	printf("PUBLISHER: starting up...\n");
}

void publisher_stop(pubsub_sender_pt client) {
	printf("PUBLISHER: stopping...\n");
}

void publisher_destroy(pubsub_sender_pt client) {
	hashMap_destroy(client->tid_map, false, false);
	client->trackers = NULL;
	client->ident = NULL;
	free(client);
}

celix_status_t publisher_publishSvcAdded(void * handle, service_reference_pt reference, void * service){
	pubsub_publisher_pt publish_svc = (pubsub_publisher_pt)service;
	pubsub_sender_pt manager = (pubsub_sender_pt)handle;

	printf("PUBLISHER: new publish service exported (%s).\n",manager->ident);

	send_thread_struct_pt data = calloc(1,sizeof(struct send_thread_struct));
	const char *value = NULL;
	serviceReference_getProperty(reference, PUBSUB_PUBLISHER_TOPIC, &value);
	data->service = publish_svc;
	data->publisher = manager;
	data->topic = value;
	celix_thread_t *tid = malloc(sizeof(*tid));
	stop=false;
	celixThread_create(tid,NULL,send_thread,(void*)data);
	hashMap_put(manager->tid_map, publish_svc, tid);
	return CELIX_SUCCESS;
}

celix_status_t publisher_publishSvcRemoved(void * handle, service_reference_pt reference, void * service){
	pubsub_sender_pt manager = (pubsub_sender_pt)handle;
	celix_thread_t *tid = hashMap_get(manager->tid_map, service);

#if defined(__APPLE__) && defined(__MACH__)
	uint64_t threadid;
	pthread_threadid_np(tid->thread, &threadid);
	printf("PUBLISHER: publish service unexporting (%s) %llu!\n",manager->ident, threadid);
#else
	printf("PUBLISHER: publish service unexporting (%s) %li!\n",manager->ident, tid->thread);
#endif

	stop=true;
	celixThread_join(*tid,NULL);
	free(tid);
	return CELIX_SUCCESS;
}
