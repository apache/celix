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

#include "gtest/gtest.h"

#include <stdarg.h>
#include "version.h"


extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "dyn_common.h"
#include "dyn_message.h"
#include "celix_err.h"
#include "celix_version.h"

static void checkMessageVersion(dyn_message_type* dynMsg, const char* v){
	int status = 0;

	const char* version = dynMessage_getVersionString(dynMsg);
	ASSERT_STREQ(v, version);
    const celix_version_t* msgVersion = nullptr;
    celix_version_t* localMsgVersion = nullptr;
	int cmpVersion = -1;
	version_createVersionFromString(version,&localMsgVersion);
    msgVersion = dynMessage_getVersion(dynMsg);
	ASSERT_EQ(0, status);
    cmpVersion = celix_version_compareTo(msgVersion, localMsgVersion);
	ASSERT_EQ(cmpVersion,0);
	version_destroy(localMsgVersion);

}


static void msg_test1(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/msg_example1.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(0, status);
	fclose(desc);

	const char* name = dynMessage_getName(dynMsg);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("poi", name);

	checkMessageVersion(dynMsg,"1.0.0");

	const char* annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.PointOfInterest", annVal);

	const char* nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	const dyn_type* msgType = dynMessage_getMessageType(dynMsg);
	ASSERT_TRUE(msgType != NULL);

	dynMessage_destroy(dynMsg);
}


static void msg_test2(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/msg_example2.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(0, status);
	fclose(desc);

	const char* name = dynMessage_getName(dynMsg);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("track", name);

	checkMessageVersion(dynMsg,"0.0.1");

	const char* annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.Track", annVal);

	const char* nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	const dyn_type* msgType = dynMessage_getMessageType(dynMsg);
	ASSERT_TRUE(msgType != NULL);

	dynMessage_destroy(dynMsg);
}

static void msg_test3(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/msg_example3.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(0, status);
	fclose(desc);

	const char* name = dynMessage_getName(dynMsg);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("logEntry", name);

	checkMessageVersion(dynMsg,"1.0.0");

	const char* annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.LogEntry", annVal);

	const char* nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	const dyn_type* msgType = dynMessage_getMessageType(dynMsg);
	ASSERT_TRUE(msgType != NULL);

	dynMessage_destroy(dynMsg);
}

static void msg_test4(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/msg_example4.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_TRUE(status != 0);
	fclose(desc);
}

static void msg_invalid(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/invalids/invalidMsgHdr.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

	desc = fopen("descriptors/invalids/invalidMsgMissingVersion.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

	desc = fopen("descriptors/invalids/invalidMsgInvalidSection.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

	desc = fopen("descriptors/invalids/invalidMsgInvalidName.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

	desc = fopen("descriptors/invalids/invalidMsgInvalidType.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

	desc = fopen("descriptors/invalids/invalidMsgInvalidVersion.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(1, status);
	fclose(desc);

    desc = fopen("descriptors/invalids/invalidMsgMissingName.descriptor", "r");
    assert(desc != NULL);
    status = dynMessage_parse(desc, &dynMsg);
    ASSERT_NE(0, status);
    fclose(desc);

    desc = fopen("descriptors/invalids/invalidMsgType.descriptor", "r");
    assert(desc != NULL);
    status = dynMessage_parse(desc, &dynMsg);
    ASSERT_NE(0, status);
    fclose(desc);

    desc = fopen("descriptors/invalids/invalidMsgMissingNewline.descriptor", "r");
    assert(desc != NULL);
    status = dynMessage_parse(desc, &dynMsg);
    ASSERT_NE(0, status);
    fclose(desc);
}

}


class DynMessageTests : public ::testing::Test {
public:
    DynMessageTests() {
    }
    ~DynMessageTests() override {
        celix_err_resetErrors();
    }

};

TEST_F(DynMessageTests, msg_test1) {
	msg_test1();
}

TEST_F(DynMessageTests, msg_test2) {
	msg_test2();
}

TEST_F(DynMessageTests, msg_test3) {
	msg_test3();
}

TEST_F(DynMessageTests, msg_test4) {
	msg_test4();
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynMessageTests, msg_invalid) {
	msg_invalid();
}

