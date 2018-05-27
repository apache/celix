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

#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pubsub/publisher_mock.h"


static pubsub_publisher_t mockSrv;
static void* mockHandle = (void*)0x42;


TEST_GROUP(pubsubmock) {
    void setup(void) {
    	//setup mock
    	pubsub_publisherMock_init(&mockSrv, mockHandle);
    }

    void teardown() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(pubsubmock, publishermock) {
    const char* mockFqn = "example.Msg";
    unsigned int mockOutputTypeId = 11;

    mock(PUBSUB_PUBLISHERMOCK_SCOPE).expectOneCall(PUBSUB_PUBLISHERMOCK_LOCAL_MSG_TYPE_ID_FOR_MSG_TYPE_METHOD)
        .withParameter("handle", mockHandle)
	.withParameter("msgType", mockFqn)
	.withOutputParameterReturning("msgTypeId", &mockOutputTypeId, sizeof(mockOutputTypeId));

    mock(PUBSUB_PUBLISHERMOCK_SCOPE).expectOneCall(PUBSUB_PUBLISHERMOCK_SEND_METHOD)
        .withParameter("handle", mockHandle)
        .withParameter("msgTypeId", mockOutputTypeId)
        .ignoreOtherParameters();

    //This should normally be code which should be tested, for now it code to verify the mock
    pubsub_publisher_t* srv = &mockSrv;
    const char* msgFqn = "example.Msg";
    unsigned int msgId = 0;

    //get type id (normally only if type id is not yet set (e.g. 0))
    srv->localMsgTypeIdForMsgType(srv->handle, msgFqn, &msgId);
    CHECK(msgId != 0);

    //set msg
    void *dummyMsg = (void*)0x43; 
    srv->send(srv->handle, msgId, dummyMsg); //should satify the expectOneCalls 
    //srv->send(srv->handle, msgId, dummyMsg); //enabling this should fail the test

}

