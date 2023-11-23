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

#include <gtest/gtest.h>

#include "celix_err.h"

class ErrTestSuite : public ::testing::Test {
public:
    ErrTestSuite() {
        celix_err_resetErrors();
    }
};

TEST_F(ErrTestSuite, AddAndPopErrorTest) {
    EXPECT_EQ(0, celix_err_getErrorCount());
    celix_err_push("error message");
    celix_err_pushf("error message from %s", "test");
    EXPECT_EQ(2, celix_err_getErrorCount());

    auto* m = celix_err_popLastError();
    EXPECT_STREQ("error message from test", m);
    EXPECT_EQ(1, celix_err_getErrorCount());

    m = celix_err_popLastError();
    EXPECT_STREQ("error message", m);
    EXPECT_EQ(0, celix_err_getErrorCount());
}

TEST_F(ErrTestSuite, ResetErrorTest) {
    EXPECT_EQ(0, celix_err_getErrorCount());
    celix_err_push("error message");
    celix_err_push("error message");
    EXPECT_EQ(2, celix_err_getErrorCount());

    celix_err_resetErrors();
    EXPECT_EQ(0, celix_err_getErrorCount());
    EXPECT_EQ(nullptr, celix_err_popLastError());
}

TEST_F(ErrTestSuite, EdgeCaseTest) {
    //test only "" error messages, 1 char per message so max error count is CELIX_ERR_BUFFER_SIZE
    for (int i = 0; i < CELIX_ERR_BUFFER_SIZE + 10; ++i) {
        celix_err_push("");
    }
    EXPECT_EQ(CELIX_ERR_BUFFER_SIZE, celix_err_getErrorCount());
    for (int i = 0; i < CELIX_ERR_BUFFER_SIZE; ++i) {
        EXPECT_STREQ("", celix_err_popLastError());
    }
    EXPECT_EQ(0, celix_err_getErrorCount());

    //test only "[0-9]" error messages, 2 char per message so max error count is CELIX_ERR_BUFFER_SIZE/2
    for (int i = 0; i < (CELIX_ERR_BUFFER_SIZE/2) + 10; ++i) {
        celix_err_pushf("%i", i % 10);
    }
    EXPECT_EQ(CELIX_ERR_BUFFER_SIZE/2, celix_err_getErrorCount());
    for (int i = (CELIX_ERR_BUFFER_SIZE/2) - 1; i >= 0; --i) {
        EXPECT_STREQ(std::to_string(i % 10).c_str(), celix_err_popLastError());
    }
    EXPECT_EQ(0, celix_err_getErrorCount());
}

TEST_F(ErrTestSuite, PrintErrorsTest) {
    EXPECT_EQ(0, celix_err_getErrorCount());
    celix_err_push("error message1");
    celix_err_push("error message2");
    EXPECT_EQ(2, celix_err_getErrorCount());

    char* buf = NULL;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);
    celix_err_printErrors(stream, nullptr, nullptr);
    fclose(stream);
    EXPECT_STREQ("error message2\nerror message1\n", buf);
    EXPECT_EQ(0, celix_err_getErrorCount());
    free(buf);


    celix_err_push("error message1");
    celix_err_push("error message2");
    buf = NULL;
    bufLen = 0;
    stream = open_memstream(&buf, &bufLen);
    celix_err_printErrors(stream, "PRE", "POST");
    fclose(stream);
    EXPECT_STREQ("PREerror message2POSTPREerror message1POST", buf);
    EXPECT_EQ(0, celix_err_getErrorCount());
    free(buf);
}

#define ERR_TEST_PREFIX "<"
#define ERR_TEST_POSTFIX ">"

TEST_F(ErrTestSuite, DumpErrorsTest) {
    char buf[CELIX_ERR_BUFFER_SIZE] = {0};
    // empty ERR
    EXPECT_EQ(0, celix_err_dump(buf, CELIX_ERR_BUFFER_SIZE, NULL, NULL));

    // log messages
    int logCnt = 0;
    for (logCnt = 0; logCnt < 5; ++logCnt) {
        celix_err_pushf("celix error message%d", logCnt);
    }
    EXPECT_EQ(115, celix_err_dump(buf, CELIX_ERR_BUFFER_SIZE, ERR_TEST_PREFIX, ERR_TEST_POSTFIX"\n"));
    char* p = buf;
    char data[32]={0};
    while(logCnt--)
    {
        sscanf(p,"%[^\n]",data);
        char expected[64];
        sprintf(expected, "<celix error message%d>", logCnt);
        EXPECT_STREQ(data, expected);
        p = p+strlen(data)+1;//skip '\n'
    }
}

TEST_F(ErrTestSuite, DumpedErrorsAlwaysNulTerminatedTest) {
    char buf[CELIX_ERR_BUFFER_SIZE] = {0};
    //celix error messages total length is greater than 512
    int logCnt = 0;
    for (logCnt = 0; logCnt < 64; ++logCnt) {
        celix_err_pushf("celix error message%d", logCnt);
    }
    EXPECT_LE(CELIX_ERR_BUFFER_SIZE, celix_err_dump(buf, CELIX_ERR_BUFFER_SIZE, ERR_TEST_PREFIX, ERR_TEST_POSTFIX"\n"));
    EXPECT_EQ('\0', buf[CELIX_ERR_BUFFER_SIZE-1]);
}
