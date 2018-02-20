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
 * publisher.h
 *
 *  \date       Jan 7, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef __PUBSUB_PUBLISHER_H_
#define __PUBSUB_PUBLISHER_H_

#include <stdlib.h>

#define PUBSUB_PUBLISHER_SERVICE_NAME           "pubsub.publisher"
#define PUBSUB_PUBLISHER_SERVICE_VERSION	    "2.0.0"
 
//properties
#define PUBSUB_PUBLISHER_TOPIC                  "topic"
#define PUBSUB_PUBLISHER_SCOPE                  "scope"
#define PUBSUB_PUBLISHER_CONFIG                 "pubsub.config"
 
#define PUBSUB_PUBLISHER_SCOPE_DEFAULT			"default"
//flags
#define PUBSUB_PUBLISHER_FIRST_MSG  01
#define PUBSUB_PUBLISHER_PART_MSG   02
#define PUBSUB_PUBLISHER_LAST_MSG   04

struct pubsub_release_callback_struct {
    void *handle;
    void (*release)(char *buf, void *handle);
};
typedef struct pubsub_release_callback_struct pubsub_release_callback_t;
typedef struct pubsub_release_callback_struct* pubsub_release_callback_pt;
 
 
struct pubsub_publisher {
    void *handle;
 
    /**
     * Every msg is identifiable by msg type string. Because masg type string are performance wise not preferable (string compares),
     * a "local" (int / platform dependent) unique id will be generated runtime
     * with use of a distributed key/value store or communication between  participation parties.
     * this is called the local message type id. This local message type id can be requested with the localMsgIdForMsgType method.
     * When return is successful the msgTypeId is always greater than 0. (Note this can be used to specify/detect uninitialized msg type ids in the consumer code).
     *
     * Returns 0 on success.
     */
    int (*localMsgTypeIdForMsgType)(void *handle, const char *msgType, unsigned int *msgTypeId);
  
    /**
     * send is a async function, but the msg can be safely deleted after send returns.
     * Returns 0 on success.
     */
    int (*send)(void *handle, unsigned int msgTypeId, const void *msg);
 
  
    /**
     * sendMultipart is a async function, but the msg can be safely deleted after send returns.
     * The first (primary) message of a multipart message must have the flag PUBLISHER_PRIMARY_MSG
     * The last message of a multipart message must have the flag PUBLISHER_LAST_MSG
     * Returns 0 on success.
     */
    int (*sendMultipart)(void *handle, unsigned int msgTypeId, const void *msg, int flags);
 
};
typedef struct pubsub_publisher pubsub_publisher_t;
typedef struct pubsub_publisher* pubsub_publisher_pt;

#endif // __PUBSUB_PUBLISHER_H_
