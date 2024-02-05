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

#include "celix_netstring.h"
#include <cerrno>
#include <gtest/gtest.h>
#include "malloc_ei.h"
#include "stdio_ei.h"

class CelixNetstringErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixNetstringErrorInjectionTestSuite() = default;

    ~CelixNetstringErrorInjectionTestSuite() override {
        celix_ei_expect_fprintf(nullptr, 0, 0);
        celix_ei_expect_fwrite(nullptr, 0, 0);
        celix_ei_expect_fputc(nullptr, 0, 0);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
    }
};

TEST_F(CelixNetstringErrorInjectionTestSuite, EncodefWithFprintfErrorInjectionTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    celix_ei_expect_fprintf((void *)&celix_netstring_encodef, 0, -1);
    int rc = celix_netstring_encodef(input, inputLen, stream);
    ASSERT_EQ(ENOSPC, rc);

    fclose(stream);
}

TEST_F(CelixNetstringErrorInjectionTestSuite, EncodefWithFwriteErrorInjectionTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    celix_ei_expect_fwrite((void *)&celix_netstring_encodef, 0, -1);
    int rc = celix_netstring_encodef(input, inputLen, stream);
    ASSERT_EQ(ENOSPC, rc);

    fclose(stream);
}

TEST_F(CelixNetstringErrorInjectionTestSuite, EncodefWithFputcErrorInjectionTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    const char *input = "Hello World";
    size_t inputLen = strlen(input);

    celix_ei_expect_fputc((void *)&celix_netstring_encodef, 0, -1);
    int rc = celix_netstring_encodef(input, inputLen, stream);
    ASSERT_EQ(ENOSPC, rc);

    fclose(stream);
}

TEST_F(CelixNetstringErrorInjectionTestSuite, DecodefWithMallocErrorInjectionTest) {
    FILE *stream = fmemopen(nullptr, 128, "w+");
    ASSERT_TRUE(stream != nullptr);

    fputs("11:Hello World,", stream);

    rewind(stream);

    celix_ei_expect_malloc((void *)&celix_netstring_decodef, 0, nullptr);
    char *output = nullptr;
    size_t outputLen = 0;
    int rc = celix_netstring_decodef(stream, &output, &outputLen);
    ASSERT_EQ(ENOMEM, rc);

    fclose(stream);
}
