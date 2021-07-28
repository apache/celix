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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#include "celix_properties.h"

#define PUBSUB_INTERCEPTOR_SERVICE_NAME     "pubsub.interceptor"
#define PUBSUB_INTERCEPTOR_SERVICE_VERSION  "2.0.0"

typedef struct pubsub_interceptor_properties {
    const char* psaType; //i.e. zmq, tcp, etc
    const char* serializationType; //i.e. json, avrobin
    const char* scope;
    const char* topic;
} pubsub_interceptor_properties_t;

/**
 * @brief PubSub Interceptor which can be used to intercept pubsub publish/receive callbacks
 *
 */
struct pubsub_interceptor {
    /**
     * Service handle.
     */
    void *handle;

    /**
     * @brief preSend will be called when a user called send on a pubsub/publisher, but before the message is "handed over" to the actual pubsub technology (i.e. TCP stack,  shared memory, etc)
     *
     * This function can be NULL.
     *
     * @param handle The service handle
     * @param properties The scope and topic of the sending publisher
     * @param messageType The fqn of the message
     * @param msgTypeId The (local) type id of the message
     * @param message The actual message pointer
     * @param metadata The metadata of the message
     * @return True if the send should continue.
     */
    bool (*preSend)(void *handle, const pubsub_interceptor_properties_t *properties, const char *messageType, uint32_t msgTypeId, const void *message, celix_properties_t *metadata);

    /**
     * @brief postSend will be called when a user called send on a pubsub/publisher, but after the message is "handed over" to the actual pubsub technology (i.e. TCP stack,  shared memory, etc)
     *
     * This function can be NULL.
     *
     * @param handle The service handle
     * @param properties The scope and topic of the sending publisher
     * @param messageType The fqn of the message
     * @param msgTypeId The (local) type id of the message
     * @param message The actual message pointer
     * @param metadata The metadata of the message
     */
    void (*postSend)(void *handle, const pubsub_interceptor_properties_t *properties, const char *messageType, uint32_t msgTypeId, const void *message, celix_properties_t *metadata);

    /**
     * @brief preReceive will be called when is message is received in a pubsub admin, but before the pubsub/subscriber callback is called.
     *
     * This function can be NULL.
     *
     * @param handle The service handle
     * @param properties The scope and topic of the sending publisher
     * @param messageType The fqn of the message
     * @param msgTypeId The (local) type id of the message
     * @param message The actual message pointer
     * @param metadata The metadata of the message
     * @return True if the pubsub/subsciber callback should be called.
     */
    bool (*preReceive)(void *handle, const pubsub_interceptor_properties_t *properties, const char *messageType, uint32_t msgTypeId, const void *message, celix_properties_t *metadata);

    /**
     * @brief postReceive will be called when is message is received in a pubsub admin and is called after the pubsub/subscriber callback is called.
     *
     * This function can be NULL.
     *
     * @param handle The service handle
     * @param properties The scope and topic of the sending publisher
     * @param messageType The fqn of the message
     * @param msgTypeId The (local) type id of the message
     * @param message The actual message pointer
     * @param metadata The metadata of the message
     */
    void (*postReceive)(void *handle, const pubsub_interceptor_properties_t *properties, const char *messageType, uint32_t msgTypeId, const void *message, celix_properties_t *metadata);
};

typedef struct pubsub_interceptor pubsub_interceptor_t;

#ifdef __cplusplus
}
#endif
#endif //__PUBSUB_INTERCEPTOR_H
