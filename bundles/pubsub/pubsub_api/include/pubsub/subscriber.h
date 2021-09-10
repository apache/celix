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

#ifndef __PUBSUB_SUBSCRIBER_H_
#define __PUBSUB_SUBSCRIBER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "celix_properties.h"

#define PUBSUB_SUBSCRIBER_SERVICE_NAME          "pubsub.subscriber"
#define PUBSUB_SUBSCRIBER_SERVICE_VERSION       "3.0.0"
 
//properties
#define PUBSUB_SUBSCRIBER_TOPIC                "topic"
#define PUBSUB_SUBSCRIBER_SCOPE                "scope"
#define PUBSUB_SUBSCRIBER_CONFIG               "pubsub.config"

struct pubsub_subscriber_struct {
    void *handle;

    /**
     * Called to initialize the subscriber with the receiver thread.
     * Can be used to tweak the receiver thread attributes
     *
     * this method can be  NULL.
     */
    int (*init)(void *handle);
     

     /**
      * When a new message for a topic is available the receive will be called.
      *
      * msgType contains fully qualified name of the type and msgTypeId is a local id which presents the type for performance reasons.
      * Release can be used to instruct the pubsubadmin to release (free) the message when receive function returns. Set it to false to take
      * over ownership of the msg (e.g. take the responsibility to free it).
      *
      * The callbacks argument is only valid inside the receive function, use the getMultipart callback, with retain=true, to keep multipart messages in memory.
      * results of the localMsgTypeIdForMsgType callback are valid during the complete lifecycle of the component, not just a single receive call.
      *
      *
      * this method can be  NULL.
      *
      * @param handle       The subscriber handle
      * @param msgType      The fully qualified type name
      * @param msgTypeId    The local type id of the type, how this is calculated/created is up to the pubsub admin. (can be cached for improved performance)
      * @param msg          The pointer to the message
      * @param metadata     The meta data provided with the data. Can be NULL and is only valid during the callback.
      * @param release      Pointer to the release boolean, default is release is true.
      * @return Return 0 implies a successful handling. If return is not 0, the msg will always be released by the pubsubadmin.
      */
    int (*receive)(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *release);

};
typedef struct pubsub_subscriber_struct pubsub_subscriber_t;

#ifdef __cplusplus
}
#endif
#endif //  __PUBSUB_SUBSCRIBER_H_
