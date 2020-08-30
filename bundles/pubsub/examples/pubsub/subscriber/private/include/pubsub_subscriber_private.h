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
 * pubsub_subscriber_private.h
 *
 *  \date       Sep 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PUBSUB_SUBSCRIBER_PRIVATE_H_
#define PUBSUB_SUBSCRIBER_PRIVATE_H_


#include <string.h>

#include "celixbool.h"

#include "pubsub/subscriber.h"

struct pubsub_receiver {
    char *name;
};

typedef struct pubsub_receiver pubsub_receiver_t;

pubsub_receiver_t* subscriber_create(char* topics);
void subscriber_start(pubsub_receiver_t* client);
void subscriber_stop(pubsub_receiver_t* client);
void subscriber_destroy(pubsub_receiver_t* client);

int pubsub_subscriber_recv(void* handle, const char* msgType, unsigned int msgTypeId, void* msg, const celix_properties_t *metadata, bool* release);


#endif /* PUBSUB_SUBSCRIBER_PRIVATE_H_ */
