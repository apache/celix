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

#include "json_rpc.h"
#include "dyn_function.h"
#include "celix_err.h"
#include "jansson_ei.h"

#include <gtest/gtest.h>

class JsonRpcErrorInjectionTestSuite : public ::testing::Test {
public:
    JsonRpcErrorInjectionTestSuite() {

    }
    ~JsonRpcErrorInjectionTestSuite() override {
        celix_ei_expect_json_dumps(nullptr, 0, nullptr);
        celix_ei_expect_json_array_append_new(nullptr, 0, -1);
        celix_ei_expect_json_string(nullptr, 0, nullptr);
        celix_ei_expect_json_array(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(JsonRpcErrorInjectionTestSuite, prepareErrorTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    char *result = nullptr;

    void *handle = nullptr;
    double arg1 = 1.0;
    double arg2 = 2.0;

    void *args[4];
    args[0] = &handle;
    args[1] = &arg1;
    args[2] = &arg2;

    celix_ei_expect_json_array((void*)jsonRpc_prepareInvokeRequest, 0, nullptr);
    rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
    ASSERT_NE(0, rc);
    EXPECT_STREQ("Error adding arguments array for 'add'", celix_err_popLastError());

    celix_ei_expect_json_string((void*)jsonRpc_prepareInvokeRequest, 0, nullptr);
    rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
    ASSERT_NE(0, rc);
    EXPECT_STREQ("Error setting method name 'add'", celix_err_popLastError());

    celix_ei_expect_json_array_append_new((void*)jsonRpc_prepareInvokeRequest, 0, -1);
    rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
    ASSERT_NE(0, rc);
    EXPECT_STREQ("Error adding argument (1) for 'add'", celix_err_popLastError());

    celix_ei_expect_json_dumps((void*)jsonRpc_prepareInvokeRequest, 0, nullptr);
    rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
    ASSERT_NE(0, rc);

    dynFunction_destroy(dynFunc);
}