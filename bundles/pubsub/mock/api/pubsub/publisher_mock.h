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
#ifndef PUBSUB_PUBLISHER_MOCK_H_
#define PUBSUB_PUBLISHER_MOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub/publisher.h" 

#define PUBSUB_PUBLISHERMOCK_SCOPE "pubsub_publisher"
#define PUBSUB_PUBLISHERMOCK_LOCAL_MSG_TYPE_ID_FOR_MSG_TYPE_METHOD "pubsub__publisherMock_localMsgTypeIdForMsgType"
#define PUBSUB_PUBLISHERMOCK_SEND_METHOD "pubsub__publisherMock_send"
#define PUBSUB_PUBLISHERMOCK_SEND_MULTIPART_METHOD "pubsub__publisherMock_sendMultipart"


/*============================================================================
  MOCK - intialize publisher mock
  ============================================================================*/
void pubsub_publisherMock_init(pubsub_publisher_t* srv, void* handle);
  
#ifdef __cplusplus
}
#endif

#endif //PUBSUB_PUBLISHER_MOCK_H_
