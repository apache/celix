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
 * pubsub_subscriber.c
 *
 *  \date       Sep 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <hash_map.h>
#include <celix_properties.h>

#include "poi.h"
#include "pubsub_subscriber_private.h"

pubsub_receiver_t* subscriber_create(char* topics) {
    pubsub_receiver_t *sub = calloc(1,sizeof(*sub));
    sub->name = strdup(topics);
    return sub;
}


void subscriber_start(pubsub_receiver_t *subscriber) {
    printf("Subscriber started...\n");
}

void subscriber_stop(pubsub_receiver_t *subscriber) {
    printf("Subscriber stopped...\n");
}

void subscriber_destroy(pubsub_receiver_t *subscriber) {
    if (subscriber->name != NULL) {
        free(subscriber->name);
    }
    subscriber->name=NULL;
    free(subscriber);
}

int pubsub_subscriber_recv(void* handle, const char* msgType, unsigned int msgTypeId, void* msg, const celix_properties_t *metadata, bool* release) {
    location_t place = (location_t)msg;
    printf("Recv (%s): [%f, %f] (%s, %s, %s, len data %li)\n", msgType, place->position.lat, place->position.lon, place->name, place->description, place->extra, (long)(strlen(place->data) + 1));

    if (metadata == NULL || celix_properties_size(metadata) == 0) {
        printf("No metadata\n");
    } else {
        const char *key;
        CELIX_PROPERTIES_FOR_EACH(metadata, key) {
            const char *val = celix_properties_get(metadata, key, "!Error!");
            printf("%s=%s\n", key, val);
        }
    }

    return 0;
}
