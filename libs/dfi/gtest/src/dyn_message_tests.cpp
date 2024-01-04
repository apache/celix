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
#include "celix_version.h"


extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "dyn_common.h"
#include "dyn_message.h"

#if NO_MEMSTREAM_AVAILABLE
#include "open_memstream.h"
#include "fmemopen.h"
#endif

static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
	va_list ap;
	const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
	fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static void checkMessageVersion(dyn_message_type* dynMsg, const char* v){
	int status = 0;

	char *version = NULL;
	status = dynMessage_getVersionString(dynMsg, &version);
	ASSERT_EQ(0, status);
	ASSERT_STREQ(v, version);
	celix_version_t* msgVersion = nullptr;
        celix_version_t* localMsgVersion = celix_version_createVersionFromString(version);
	status = dynMessage_getVersion(dynMsg,&msgVersion);
	ASSERT_EQ(0, status);
        int cmpVersion = celix_version_compareTo(msgVersion, localMsgVersion);
	ASSERT_EQ(cmpVersion,0);
	celix_version_destroy(localMsgVersion);
}


static void msg_test1(void) {
	int status = 0;
	dyn_message_type *dynMsg = NULL;
	FILE *desc = fopen("descriptors/msg_example1.descriptor", "r");
	assert(desc != NULL);
	status = dynMessage_parse(desc, &dynMsg);
	ASSERT_EQ(0, status);
	fclose(desc);

	char *name = NULL;
	status = dynMessage_getName(dynMsg, &name);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("poi", name);

	checkMessageVersion(dynMsg,"1.0.0");

	char *annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.PointOfInterest", annVal);

	char *nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	dyn_type *msgType = NULL;
	status = dynMessage_getMessageType(dynMsg, &msgType);
	ASSERT_EQ(0, status);
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

	char *name = NULL;
	status = dynMessage_getName(dynMsg, &name);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("track", name);

	checkMessageVersion(dynMsg,"0.0.1");

	char *annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.Track", annVal);

	char *nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	dyn_type *msgType = NULL;
	status = dynMessage_getMessageType(dynMsg, &msgType);
	ASSERT_EQ(0, status);
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

	char *name = NULL;
	status = dynMessage_getName(dynMsg, &name);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("logEntry", name);

	checkMessageVersion(dynMsg,"1.0.0");

	char *annVal = NULL;
	status = dynMessage_getAnnotationEntry(dynMsg, "classname", &annVal);
	ASSERT_EQ(0, status);
	ASSERT_STREQ("org.example.LogEntry", annVal);

	char *nonExist = NULL;
	status = dynMessage_getHeaderEntry(dynMsg, "nonExisting", &nonExist);
	ASSERT_TRUE(status != 0);
	ASSERT_TRUE(nonExist == NULL);

	dyn_type *msgType = NULL;
	status = dynMessage_getMessageType(dynMsg, &msgType);
	ASSERT_EQ(0, status);
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

}

}


class DynMessageTests : public ::testing::Test {
public:
    DynMessageTests() {
        int level = 1;
        dynCommon_logSetup(stdLog, NULL, level);
        dynType_logSetup(stdLog, NULL, level);
        dynMessage_logSetup(stdLog, NULL, level);
    }
    ~DynMessageTests() override {
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
}

TEST_F(DynMessageTests, msg_invalid) {
	msg_invalid();
}

