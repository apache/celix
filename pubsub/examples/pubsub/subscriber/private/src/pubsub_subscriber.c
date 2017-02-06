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
 * pubsub_subscriber.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include "poi.h"
#include "pubsub_subscriber_private.h"

pubsub_receiver_pt subscriber_create(char* topics) {
	pubsub_receiver_pt sub = calloc(1,sizeof(*sub));
	sub->name = strdup(topics);
	return sub;
}


void subscriber_start(pubsub_receiver_pt subscriber){
	printf("Subscriber started...\n");
}

void subscriber_stop(pubsub_receiver_pt subscriber){
	printf("Subscriber stopped...\n");
}

void subscriber_destroy(pubsub_receiver_pt subscriber){
	if(subscriber->name!=NULL){
		free(subscriber->name);
	}
	subscriber->name=NULL;
	free(subscriber);
}

int pubsub_subscriber_recv(void* handle, const char* msgType, unsigned int msgTypeId, void* msg,pubsub_multipart_callbacks_t *callbacks, bool* release){

	location_t place = (location_t)msg;
	int nrchars = 25;
	printf("Recv (%s): [%f, %f] (%s, %s) data_len = %ld data =%*.*s\n",msgType, place->position.lat, place->position.lon,place->name,place->description, strlen(place->data) + 1, nrchars, nrchars, place->data);

	return 0;

}
