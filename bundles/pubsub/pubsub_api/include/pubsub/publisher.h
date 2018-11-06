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
#define PUBSUB_PUBLISHER_SERVICE_VERSION	    "3.0.0"
 
//properties
#define PUBSUB_PUBLISHER_TOPIC                  "topic"
#define PUBSUB_PUBLISHER_SCOPE                  "scope"
#define PUBSUB_PUBLISHER_CONFIG                 "pubsub.config"
 
#define PUBSUB_PUBLISHER_SCOPE_DEFAULT			"default"

 
 
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
 
};
typedef struct pubsub_publisher pubsub_publisher_t;
typedef struct pubsub_publisher* pubsub_publisher_pt;

#endif // __PUBSUB_PUBLISHER_H_
