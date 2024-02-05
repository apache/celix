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
#include <cerrno>
#include "celix_netstring.h"

class CelixNetstringTestSuite : public ::testing::Test {
public:
    static void TestDecodeFromStreamWithInvalidNetstring(const char *netstring) {
        FILE *stream = fmemopen(nullptr, 128, "w+");
        ASSERT_TRUE(stream != nullptr);

        fputs(netstring, stream);

        rewind(stream);

        char *output = nullptr;
        size_t outputLen = 0;
        auto rc = celix_netstring_decodef(stream, &output, &outputLen);
        ASSERT_EQ(EINVAL, rc);
        ASSERT_TRUE(output == nullptr);
        ASSERT_TRUE(outputLen == 0);

        fclose(stream);
    }
};

TEST_F(CelixNetstringTestSuite, EncodeToStreamTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    int rc = celix_netstring_encodef(input, inputLen, stream);
    ASSERT_EQ(0, rc);

    rewind(stream);

    char *output = nullptr;
    size_t outputLen = 0;
    rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen, outputLen);
    ASSERT_STREQ(input, output);

    free(output);
    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, EncodeToBufferTest) {
    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    char buffer[128];
    size_t offset = 0;

    int rc = celix_netstring_encodeb(input, inputLen, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(0, rc);

    char *output = nullptr;
    size_t outputLen = 0;
    rc = celix_netstring_decodeb(buffer, offset, (const char **) &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen, outputLen);
    char *outString = (char*)calloc(1, outputLen + 1);
    memcpy(outString, output, outputLen);
    ASSERT_STREQ(input, outString);

    free(outString);
}

TEST_F(CelixNetstringTestSuite, EncodeMutipleStringToStreamTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    //encode 2 strings to stream
    const char *input1 = "Hello World";
    size_t inputLen1 = strlen(input1);

    int rc = celix_netstring_encodef(input1, inputLen1, stream);
    ASSERT_EQ(0, rc);

    const char *input2 = "Hello World 2";
    size_t inputLen2 = strlen(input2);

    rc = celix_netstring_encodef(input2, inputLen2, stream);
    ASSERT_EQ(0, rc);

    rewind(stream);

    //decode 2 strings from stream
    char *output = nullptr;
    size_t outputLen = 0;
    rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen1, outputLen);
    ASSERT_STREQ(input1, output);

    free(output);

    rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen2, outputLen);
    ASSERT_STREQ(input2, output);

    free(output);
    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, EncodeMutipleStringToBufferTest) {
    //encode 2 strings to buffer
    const char *input1 = "Hello World";
    size_t inputLen1 = strlen(input1);

    const char *input2 = "Hello World 2";
    size_t inputLen2 = strlen(input2);

    char buffer[128];
    size_t offset = 0;

    int rc = celix_netstring_encodeb(input1, inputLen1, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(0, rc);

    rc = celix_netstring_encodeb(input2, inputLen2, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(0, rc);


    //decode 2 strings from buffer
    const char *output = nullptr;
    size_t outputLen = 0;
    size_t netstringLen = sizeof(buffer);
    const char *netstringStart = buffer;
    rc = celix_netstring_decodeb(netstringStart, netstringLen, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen1, outputLen);
    char *outString = (char*)calloc(1, outputLen + 1);
    memcpy(outString, output, outputLen);
    ASSERT_STREQ(input1, outString);

    free(outString);

    netstringLen -= (output - netstringStart + outputLen + 1/*,*/);
    netstringStart = output + outputLen + 1/*,*/;
    rc = celix_netstring_decodeb(netstringStart, netstringLen, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen2, outputLen);
    outString = (char*)calloc(1, outputLen + 1);
    memcpy(outString, output, outputLen);
    ASSERT_STREQ(input2, outString);

    free(outString);
}

TEST_F(CelixNetstringTestSuite, EncodeEmptyStringToStreamTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "";
    size_t inputLen = strlen(input);

    int rc = celix_netstring_encodef(input, inputLen, stream);
    ASSERT_EQ(0, rc);

    rewind(stream);

    char *output = nullptr;
    size_t outputLen = 0;
    rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen, outputLen);
    ASSERT_TRUE(output == nullptr);

    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, EncodeToStreamWithInvalidArgumentsTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    int rc = celix_netstring_encodef(nullptr, inputLen, stream);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_encodef(input, inputLen, nullptr);
    ASSERT_EQ(EINVAL, rc);

    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, DecodeFromStreamWithInvalidArgumentsTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    char *output = nullptr;
    size_t outputLen = 0;
    int rc = celix_netstring_decodef(nullptr, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_decodef(stream, nullptr, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_decodef(stream, &output, nullptr);
    ASSERT_EQ(EINVAL, rc);

    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, DecodeFromStreamWithInvalidNetstringTest) {
    TestDecodeFromStreamWithInvalidNetstring("invalid");
    TestDecodeFromStreamWithInvalidNetstring("11");
    TestDecodeFromStreamWithInvalidNetstring("5hello,");//missing ':' after size
    TestDecodeFromStreamWithInvalidNetstring("9999999999:Hello World,");//size too large
    TestDecodeFromStreamWithInvalidNetstring("11:Hello World ");//missing ','
    TestDecodeFromStreamWithInvalidNetstring("129:Hello,");//missing contents
    TestDecodeFromStreamWithInvalidNetstring(":,");//missing size
}

TEST_F(CelixNetstringTestSuite, DecodeFromStreamWithInvalidStreamModeTest) {
    FILE *stream = fmemopen(nullptr, 128, "w");//write only
    ASSERT_TRUE(stream != nullptr);

    fputs("11:Hello World,", stream);

    rewind(stream);

    char *output = nullptr;
    size_t outputLen = 0;
    int rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_NE(0, rc);

    fclose(stream);
}

TEST_F(CelixNetstringTestSuite, EncodeEmptyStringToBufferTest) {
    const char *input = "";
    size_t inputLen = strlen(input);

    char buffer[128];
    size_t offset = 0;

    int rc = celix_netstring_encodeb(input, inputLen, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(memcmp("0:,", buffer, 3) == 0);

    char *output = nullptr;
    size_t outputLen = 0;
    rc = celix_netstring_decodeb(buffer, offset, (const char **) &output, &outputLen);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(inputLen, outputLen);
}

TEST_F(CelixNetstringTestSuite, EncodeToBufferWithInvalidArgumentsTest) {
    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    char buffer[128];
    size_t offset = 0;

    int rc = celix_netstring_encodeb(nullptr, inputLen, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_encodeb(input, inputLen, nullptr, sizeof(buffer), &offset);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_encodeb(input, inputLen, buffer, sizeof(buffer), nullptr);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_encodeb(input, inputLen, buffer, 0, &offset);
    ASSERT_EQ(EINVAL, rc);
}

TEST_F(CelixNetstringTestSuite, EncodeBufferTooSmallTest) {
    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    char buffer[10];
    size_t offset = 0;

    int rc = celix_netstring_encodeb(input, inputLen, buffer, sizeof(buffer), &offset);
    ASSERT_EQ(ENOMEM, rc);
}

TEST_F(CelixNetstringTestSuite, DecodeFromBufferWithInvalidArgumentsTest) {
    const char *output = nullptr;
    size_t outputLen = 0;
    const char *input = "5:hello,";
    int rc = celix_netstring_decodeb(nullptr, 0, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_decodeb(input, 0, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_decodeb(input, strlen(input), nullptr, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    rc = celix_netstring_decodeb(input, strlen(input), &output, nullptr);
    ASSERT_EQ(EINVAL, rc);
}

TEST_F(CelixNetstringTestSuite, DecodeFromBufferWithInvalidNetstringTest) {
    const char *input = "invalid";
    size_t inputLen = strlen(input);

    const char *output = nullptr;
    size_t outputLen = 0;
    int rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = "11";
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = "5hello,";//missing ':' after size
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = "9999999999:Hello World,";//size too large
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = "11:Hello World ";//missing ','
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = "129:Hello,";//missing contents
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);

    input = ":,";//missing size
    inputLen = strlen(input);
    rc = celix_netstring_decodeb(input, inputLen, &output, &outputLen);
    ASSERT_EQ(EINVAL, rc);
}