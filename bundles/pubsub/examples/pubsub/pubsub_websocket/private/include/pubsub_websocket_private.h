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
 * pubsub_websocket_private.h
 *
 *  \date       Jan 16, 2020
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PUBSUB_WEBSOCKET_PRIVATE_H_
#define PUBSUB_WEBSOCKET_PRIVATE_H_


#include <string.h>

#include "celix_api.h"

#include "pubsub/subscriber.h"
#include "pubsub/publisher.h"

struct pubsub_sender{
    array_list_t *trackers;
    const char *ident;
    hash_map_t *tid_map; //service -> tid
    long bundleId;
    bool stop;
};
typedef struct pubsub_sender pubsub_sender_t;

struct pubsub_receiver {
    char *name;
};

typedef struct pubsub_receiver pubsub_receiver_t;


/**
 * Shared Publisher and Subscriber types
 *
 */
struct pubsub_info {
    pubsub_sender_t *sender;
    bool sending;
    pubsub_receiver_t *receiver;
};

typedef struct pubsub_info pubsub_info_t;

/**
 * Publisher
 */
struct send_thread_struct {
    pubsub_publisher_t *service;
    pubsub_info_t *pubsub;
    const char *topic;
};
typedef struct send_thread_struct send_thread_struct_t;

pubsub_sender_t* publisher_create(array_list_pt trackers, const char* ident,long bundleId);

void publisher_start(pubsub_sender_t *client);
void publisher_stop(pubsub_sender_t *client);

void publisher_destroy(pubsub_sender_t *client);

void publisher_publishSvcAdded(void * handle, void *svc, const celix_properties_t *props);
void publisher_publishSvcRemoved(void * handle, void *svc, const celix_properties_t *props);

/**
 * Subscriber
 */
pubsub_receiver_t* subscriber_create(char* topics);
void subscriber_start(pubsub_receiver_t* client);
void subscriber_stop(pubsub_receiver_t* client);
void subscriber_destroy(pubsub_receiver_t* client);

int pubsub_subscriber_recv(void* handle, const char* msgType, unsigned int msgTypeId, void* msg, const celix_properties_t *metadata, bool* release);


#endif /* PUBSUB_WEBSOCKET_PRIVATE_H_ */
