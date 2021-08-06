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

#ifndef __PUBSUB_PUBLISHER_H_
#define __PUBSUB_PUBLISHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "celix_properties.h"

#define PUBSUB_PUBLISHER_SERVICE_NAME           "pubsub.publisher"
#define PUBSUB_PUBLISHER_SERVICE_VERSION        "3.0.0"
 
//properties
#define PUBSUB_PUBLISHER_TOPIC                  "topic"
#define PUBSUB_PUBLISHER_SCOPE                  "scope"
#define PUBSUB_PUBLISHER_CONFIG                 "pubsub.config"

struct pubsub_publisher {
    void *handle;

    /**
     * Every msg is identifiable by msg type string. Because msg type string are performance wise not preferable (string compares),
     * a "local" (int / platform dependent) unique id will be generated runtime
     * with use of a distributed key/value store or communication between  participation parties.
     * this is called the local message type id. This local message type id can be requested with the localMsgIdForMsgType method.
     * When return is successful the msgTypeId is always greater than 0. (Note this can be used to specify/detect uninitialized msg type ids in the consumer code).
     *
     * @param handle        The publisher handle.
     * @param msgType       The fully qualified type name.
     * @param msgTypeId     Output argument for the local type id, how this is calculated/created is up to the pubsub admin.
     * @return              Returns 0 on success.
     */
    int (*localMsgTypeIdForMsgType)(void *handle, const char *msgType, unsigned int *msgTypeId);

    /**
     * send block until the message is either copied or serialized.
     * Whether the message is already provided to the network layer or whether send through the network is up to the pubsubadmins.
     * If and how meta data is handled can be different between pubsubadmins.
     *
     * Caller keeps ownership of the provided msg and can safely free the msg when the send function returns.
     * Callee will take ownership of the metadata.
     *
     * @param handle        The publisher handle.
     * @param msgTypeId     The local type id. Is used to find the correct message serializer and to identify the message on the wire.
     * @param msg           The message to send.
     * @param metadata      The meta data to send along with this message. Can be NULL.
     * @return              Returns 0 on success.
     */
    int (*send)(void *handle, unsigned int msgTypeId, const void *msg, celix_properties_t *metadata);
 
};
typedef struct pubsub_publisher pubsub_publisher_t;

#ifdef __cplusplus
}
#endif
#endif // __PUBSUB_PUBLISHER_H_
