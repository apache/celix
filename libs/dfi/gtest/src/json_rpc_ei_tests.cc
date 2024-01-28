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
#include "json_rpc_test.h"
#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "celix_err.h"
#include "jansson_ei.h"
#include "malloc_ei.h"

#include <gtest/gtest.h>

class JsonRpcErrorInjectionTestSuite : public ::testing::Test {
public:
    JsonRpcErrorInjectionTestSuite() {

    }
    ~JsonRpcErrorInjectionTestSuite() override {
        celix_ei_expect_json_dumps(nullptr, 0, nullptr);
        celix_ei_expect_json_object(nullptr, 0, nullptr);
        celix_ei_expect_json_real(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
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

struct tst_serv {
    void *handle;
    int (*add)(void *, double, double, double *);
    int (*sub)(void *, double, double, double *);
    int (*sqrt)(void *, double, double *);
    int (*stats)(void *, struct tst_seq, struct tst_StatsResult **);
};

TEST_F(JsonRpcErrorInjectionTestSuite, preallocationFailureTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    celix_ei_expect_calloc((void*)dynType_alloc, 0, nullptr);
    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    EXPECT_STREQ("Error allocating memory for pre-allocated output argument of add(DD)D", celix_err_popLastError());
    EXPECT_STREQ("Error allocating memory for type 'D'", celix_err_popLastError());
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcErrorInjectionTestSuite, preOutParamterSerializationFailureTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    celix_ei_expect_json_real((void*)jsonSerializer_serializeJson, 2, nullptr);
    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    EXPECT_STREQ("Error serializing result for add(DD)D", celix_err_popLastError());
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcErrorInjectionTestSuite, outParamterSerializationFailureTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, nullptr, nullptr, nullptr, stats};

    celix_ei_expect_json_object((void*)jsonSerializer_serializeJson, 3, nullptr);
    rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [[1.0,2.0]]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_STREQ("Error serializing result for stats([D)LStatsResult;", celix_err_popLastError());

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcErrorInjectionTestSuite, responsePayloadGenerationErrorTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    celix_ei_expect_json_object((void*)jsonRpc_call, 0, nullptr);
    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    EXPECT_STREQ("Error generating response payload for add(DD)D", celix_err_popLastError());
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcErrorInjectionTestSuite, responseRenderingErrorTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    celix_ei_expect_json_dumps((void*)jsonRpc_call, 0, nullptr);
    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    dynInterface_destroy(intf);
}