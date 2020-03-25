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

#ifndef __PUBSUB_INTERCEPTOR_H
#define __PUBSUB_INTERCEPTOR_H

#include <stdlib.h>
#include <stdint.h>

#include "celix_properties.h"

#define PUBSUB_INTERCEPTOR_SERVICE_NAME     "pubsub.interceptor"
#define PUBSUB_INTERCEPTOR_SERVICE_VERSION  "1.0.0"

typedef struct pubsub_interceptor_properties {
    const char *scope;
    const char *topic;
} pubsub_interceptor_properties_t;

struct pubsub_interceptor {
    void *handle;

    bool (*preSend)(void *handle, pubsub_interceptor_properties_t properties, const char *messageType, const uint32_t msgTypeId, const void *message, celix_properties_t *metadata);
    void (*postSend)(void *handle, pubsub_interceptor_properties_t properties, const char *messageType, const uint32_t msgTypeId, const void *message, celix_properties_t *metadata);
    bool (*preReceive)(void *handle, pubsub_interceptor_properties_t properties, const char *messageType, const uint32_t msgTypeId, const void *message, celix_properties_t *metadata);
    void (*postReceive)(void *handle, pubsub_interceptor_properties_t properties, const char *messageType, const uint32_t msgTypeId, const void *message, celix_properties_t *metadata);
};

typedef struct pubsub_interceptor pubsub_interceptor_t;

#endif //__PUBSUB_INTERCEPTOR_H
