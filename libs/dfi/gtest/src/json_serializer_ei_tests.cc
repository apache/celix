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

#include "json_serializer.h"
#include "dyn_type.h"
#include "celix_err.h"
#include "malloc_ei.h"

#include <gtest/gtest.h>

class JsonSerializerErrorInjectionTestSuite : public ::testing::Test {
public:
    JsonSerializerErrorInjectionTestSuite() = default;
    ~JsonSerializerErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(JsonSerializerErrorInjectionTestSuite, SerilizationError) {
    int rc;
    dyn_type *type;
    void *inst;

    //simple string
    type = nullptr;
    inst = nullptr;
    rc = dynType_parseWithStr("t", nullptr, nullptr, &type);
    ASSERT_EQ(0, rc);
    auto inputStr = R"("hello")";
    celix_ei_expect_calloc((void*) dynType_alloc, 0, nullptr);
    rc = jsonSerializer_deserialize(type, inputStr, strlen(inputStr), &inst);
    ASSERT_NE(0, rc);
    EXPECT_STREQ("Error cannot deserialize json. Input is '\"hello\"'", celix_err_popLastError());
    EXPECT_STREQ("Error allocating memory for type 't'", celix_err_popLastError());
    dynType_destroy(type);
}