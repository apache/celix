/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * pubsub_publisher.c
 *
 *  \date       Sep 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "service_tracker.h"
#include "celix_threads.h"

#include "poi.h"

#include "pubsub_publisher_private.h"

static double randCoordinate(double min, double max) {
    double ret = min + (((double)random()) / (((double)RAND_MAX)/(max-min)));
    return ret;
}

static void* send_thread(void* arg) {
    send_thread_struct_t *st_struct = (send_thread_struct_t *) arg;

    pubsub_publisher_t *publish_svc = st_struct->service;
    pubsub_sender_t *publisher = st_struct->publisher;

    char fwUUID[9];
    memset(fwUUID, 0, 9);
    memcpy(fwUUID, publisher->ident, 8);

    //poi_t point = calloc(1,sizeof(*point));
    location_t place = calloc(1, sizeof(*place));

    char *desc = calloc(64, sizeof(char));
    snprintf(desc, 64, "fw-%s [TID=%lu]", fwUUID, (unsigned long)pthread_self());

    char *name = calloc(64, sizeof(char));
    snprintf(name, 64, "Bundle#%ld", publisher->bundleId);

    place->name = name;
    place->description = desc;
    place->extra = "extra value";
    printf("TOPIC : %s\n", st_struct->topic);

    unsigned int msgId = 0;

    while (publisher->stop == false) {
        if (msgId == 0) {
            if (publish_svc->localMsgTypeIdForMsgType(publish_svc->handle, st_struct->topic, &msgId) != 0) {
                printf("PUBLISHER: Cannot retrieve msgId for message '%s'\n", MSG_POI_NAME);
            }
        }

        if (msgId > 0) {
            place->position.lat = randCoordinate(MIN_LAT, MAX_LAT);
            place->position.lon = randCoordinate(MIN_LON, MAX_LON);
            int nr_char = (int) randCoordinate(5, 100000);
            place->data = calloc(nr_char, 1);
            for (int i = 0; i < (nr_char - 1); i++) {
                place->data[i] = i % 10 + '0';
            }
            place->data[nr_char - 1] = '\0';
            if (publish_svc->send) {
                celix_properties_t *metadata = celix_properties_create();
                celix_properties_set(metadata, "Key", "Value");
                if (publish_svc->send(publish_svc->handle, msgId, place, metadata) == 0) {
                    printf("Sent %s [%f, %f] (%s, %s) data len = %d\n", st_struct->topic,
                           place->position.lat, place->position.lon, place->name, place->description, nr_char);
                }
            } else {
                printf("No send for %s\n", st_struct->topic);
            }

            free(place->data);
        }
        sleep(2);
    }

    free(place->description);
    free(place->name);
    free(place);

    free(st_struct);

    return NULL;
}

pubsub_sender_t* publisher_create(array_list_pt trackers,const char* ident,long bundleId) {
    pubsub_sender_t *publisher = malloc(sizeof(*publisher));

    publisher->trackers = trackers;
    publisher->ident = ident;
    publisher->bundleId = bundleId;
    publisher->tid_map = hashMap_create(NULL, NULL, NULL, NULL);
    publisher->stop = false;

    return publisher;
}

void publisher_start(pubsub_sender_t *client) {
    printf("PUBLISHER: starting up...\n");
}

void publisher_stop(pubsub_sender_t *client) {
    printf("PUBLISHER: stopping...\n");
}

void publisher_destroy(pubsub_sender_t *client) {
    hashMap_destroy(client->tid_map, false, false);
    client->trackers = NULL;
    client->ident = NULL;
    free(client);
}

void publisher_publishSvcAdded(void * handle, void *svc, const celix_properties_t *props) {
    pubsub_publisher_t *publish_svc = (pubsub_publisher_t *) svc;
    pubsub_sender_t *manager = (pubsub_sender_t *) handle;
    manager->stop = false;

    printf("PUBLISHER: new publish service exported (%s).\n", manager->ident);

    send_thread_struct_t *data = calloc(1, sizeof(*data));
    data->service = publish_svc;
    data->publisher = manager;
    data->topic = celix_properties_get(props, PUBSUB_PUBLISHER_TOPIC, "!ERROR!");
    celix_thread_t *tid = malloc(sizeof(*tid));
    celixThread_create(tid, NULL, send_thread, (void*)data);
    hashMap_put(manager->tid_map, publish_svc, tid);
}

void publisher_publishSvcRemoved(void * handle, void *svc, const celix_properties_t *props) {
    pubsub_sender_t *manager = (pubsub_sender_t *) handle;
    celix_thread_t *tid = hashMap_get(manager->tid_map, svc);
    manager->stop = true;

#if defined(__APPLE__) && defined(__MACH__)
    uint64_t threadid;
    pthread_threadid_np(tid->thread, &threadid);
    printf("PUBLISHER: publish service unexporting (%s) %llu!\n",manager->ident, threadid);
#else
    printf("PUBLISHER: publish service unexporting (%s) %li!\n", manager->ident, tid->thread);
#endif

    celixThread_join(*tid,NULL);
    free(tid);
}
