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

#ifndef PUBSUB_PUBLISHER_PRIVATE_H_
#define PUBSUB_PUBLISHER_PRIVATE_H_

#include "celix_api.h"
#include <pthread.h>
#include "pubsub/publisher.h"

struct pubsub_sender {
    array_list_pt trackers;
    const char *ident;
    hash_map_pt tid_map; //service -> tid
    long bundleId;
    bool stop;
};
typedef struct pubsub_sender pubsub_sender_t;

struct send_thread_struct {
    pubsub_publisher_t *service;
    pubsub_sender_t *publisher;
    const char *topic;
};
typedef struct send_thread_struct send_thread_struct_t;

pubsub_sender_t* publisher_create(array_list_pt trackers, const char* ident,long bundleId);

void publisher_start(pubsub_sender_t *client);
void publisher_stop(pubsub_sender_t *client);

void publisher_destroy(pubsub_sender_t *client);

void publisher_publishSvcAdded(void * handle, void *svc, const celix_properties_t *props);
void publisher_publishSvcRemoved(void * handle, void *svc, const celix_properties_t *props);


#endif /* PUBSUB_PUBLISHER_PRIVATE_H_ */
