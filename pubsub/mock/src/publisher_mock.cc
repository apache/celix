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
#include "pubsub/publisher_mock.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

/*============================================================================
  MOCK - mock function for pubsub_publisher->localMsgTypeIdForMsgType
  ============================================================================*/
static int pubsub__publisherMock_localMsgTypeIdForMsgType(void *handle, const char *msgType, unsigned int *msgTypeId) {
    return mock(PUBSUB_PUBLISHERMOCK_SCOPE)
        .actualCall(PUBSUB_PUBLISHERMOCK_LOCAL_MSG_TYPE_ID_FOR_MSG_TYPE_METHOD)
        .withPointerParameter("handle", handle)
        .withParameter("msgType", msgType)
        .withOutputParameter("msgTypeId", msgTypeId)
        .returnIntValue();
}

/*============================================================================
  MOCK - mock function for pubsub_publisher->send
  ============================================================================*/
static int pubsub__publisherMock_send(void *handle, unsigned int msgTypeId, const void *msg) {
    return mock(PUBSUB_PUBLISHERMOCK_SCOPE)
        .actualCall(PUBSUB_PUBLISHERMOCK_SEND_METHOD)
        .withPointerParameter("handle", handle)
        .withParameter("msgTypeId", msgTypeId)
        .withPointerParameter("msg", (void*)msg)
        .returnIntValue();
}

/*============================================================================
  MOCK - mock function for pubsub_publisher->sendMultipart
  ============================================================================*/
static int pubsub__publisherMock_sendMultipart(void *handle, unsigned int msgTypeId, const void *msg, int flags) {
    return mock(PUBSUB_PUBLISHERMOCK_SCOPE)
        .actualCall(PUBSUB_PUBLISHERMOCK_SEND_MULTIPART_METHOD)
        .withPointerParameter("handle", handle)
        .withParameter("msgTypeId", msgTypeId)
        .withPointerParameter("msg", (void*)msg)
        .withParameter("flags", flags)
        .returnIntValue();
}

/*============================================================================
  MOCK - mock setup for publisher service
  ============================================================================*/
void pubsub_publisherMock_init(pubsub_publisher_t* srv, void* handle) {
    srv->handle = handle;
    srv->localMsgTypeIdForMsgType = pubsub__publisherMock_localMsgTypeIdForMsgType;
    srv->send = pubsub__publisherMock_send;
    srv->sendMultipart = pubsub__publisherMock_sendMultipart;
}
