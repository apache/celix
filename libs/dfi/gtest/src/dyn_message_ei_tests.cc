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

#include "dyn_message.h"
#include "celix_err.h"
#include "malloc_ei.h"

#include <gtest/gtest.h>

class DynMessageErrorInjectionTestSuite : public ::testing::Test {
public:
    DynMessageErrorInjectionTestSuite() = default;
    ~DynMessageErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    }
};

TEST_F(DynMessageErrorInjectionTestSuite, ParseError) {
    int status = 0;
    dyn_message_type *dynMsg = NULL;
    FILE *desc = fopen("descriptors/msg_example1.descriptor", "r");
    assert(desc != NULL);
    //not enough memory for dyn_message_type
    celix_ei_expect_calloc((void*) dynMessage_parse, 0, nullptr);
    status = dynMessage_parse(desc, &dynMsg);
    ASSERT_NE(0, status);
    fclose(desc);
    ASSERT_STREQ("Error allocating memory for dynamic message", celix_err_popLastError());
}