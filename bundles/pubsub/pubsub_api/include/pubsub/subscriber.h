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
 * subscriber.h
 *
 *  \date       Jan 7, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef __PUBSUB_SUBSCRIBER_H_
#define __PUBSUB_SUBSCRIBER_H_

#include <stdbool.h>

#define PUBSUB_SUBSCRIBER_SERVICE_NAME          "pubsub.subscriber"
#define PUBSUB_SUBSCRIBER_SERVICE_VERSION       "2.0.0"
 
//properties
#define PUBSUB_SUBSCRIBER_TOPIC                "topic"
#define PUBSUB_SUBSCRIBER_SCOPE                "scope"
#define PUBSUB_SUBSCRIBER_CONFIG               "pubsub.config"

#define PUBSUB_SUBSCRIBER_SCOPE_DEFAULT        "default"
 
struct pubsub_multipart_callbacks_struct {
    void *handle;
    int (*localMsgTypeIdForMsgType)(void *handle, const char *msgType, unsigned int *msgId);
    int (*getMultipart)(void *handle, unsigned int msgTypeId, bool retain, void **part);
};
typedef struct pubsub_multipart_callbacks_struct pubsub_multipart_callbacks_t;
typedef struct pubsub_multipart_callbacks_struct* pubsub_multipart_callbacks_pt;
 
struct pubsub_subscriber_struct {
    void *handle;
     
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
     * Return 0 implies a successful handling. If return is not 0, the msg will always be released by the pubsubadmin.
     *
     * this method can be  NULL.
     */
    int (*receive)(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, pubsub_multipart_callbacks_t *callbacks, bool *release);

};
typedef struct pubsub_subscriber_struct pubsub_subscriber_t;
typedef struct pubsub_subscriber_struct* pubsub_subscriber_pt;


#endif //  __PUBSUB_SUBSCRIBER_H_
