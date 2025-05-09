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

#include <float.h>
#include <assert.h>

extern "C" {
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ffi.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "json_serializer.h"
#include "json_rpc.h"
#include "json_rpc_test.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_compiler.h"
#include "celix_errno.h"
#include "celix_err.h"

#include <jansson.h>


    void prepareTest(void) {
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

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
        ASSERT_EQ(0, rc);

        //printf("result is %s\n", result);

        ASSERT_TRUE(strstr(result, "\"add\"") != nullptr);
        ASSERT_TRUE(strstr(result, "1.0") != nullptr);
        ASSERT_TRUE(strstr(result, "2.0") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareCharTest(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("setName(#am=handle;Pt)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        char *arg1 = strdup("hello char");
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "setName", args, &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "\"setName\"") != nullptr);
        ASSERT_TRUE(strstr(result, "hello char") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareConstCharTest(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("setConstName(#am=handle;P#const=true;t)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        const char *arg1 = "hello const char";
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "setConstName", args, &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "\"setConstName\"") != nullptr);
        ASSERT_TRUE(strstr(result, "hello const char") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareTestFailed(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("action(#am=handle;PP)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        void *arg1 = nullptr;
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "action", args, &result);
        ASSERT_EQ(1, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void handleTestPre(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        const char *reply = "{\"r\":2.2}";
        double result = -1.0;
        double *out = &result;
        void *args[4];
        args[3] = &out;
        int rsErrno = 0;
        rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        EXPECT_DOUBLE_EQ(2.2, result);

        dynFunction_destroy(dynFunc);
    }


    void handleTestInvalidReply(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        double result = -1.0;
        double *out = &result;
        void *args[4];
        args[3] = &out;
        int rsErrno = 0;
        rc = jsonRpc_handleReply(dynFunc, "invalid", args, &rsErrno);
        ASSERT_EQ(1, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynFunction_destroy(dynFunc);
    }

    int addFailed(void*, double , double , double *) {
        return CELIX_CUSTOMER_ERROR_MAKE(0,1);// return customer error
    }

    int getName_example4(void*, char** result) {
        *result = strdup("allocatedInFunction");
        return 0;
    }

    struct item {
        double a;
        double b;
    };

    struct item_seq {
        uint32_t  cap;
        uint32_t  len;
        struct item **buf;
    };

    struct tst_serv {
        void *handle;
        int (*add)(void *, double, double, double *);
        int (*sub)(void *, double, double, double *);
        int (*sqrt)(void *, double, double *);
        int (*stats)(void *, struct tst_seq, struct tst_StatsResult **);
    };

    struct tst_serv_example4 {
        void *handle;
        int (*getName_example4)(void *, char** name);
        int (*setName_example4)(void *, char* name);
        int (*setConstName_example4)(void *, const char* name);
    };

    void callTestPreAllocated(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "3.0") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callFailedTestPreAllocated(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "e") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestOutput(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, nullptr, nullptr, nullptr, stats};

        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [[1.0,2.0]]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "1.5") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestInvalidRequest(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({)", &result);
        EXPECT_STREQ("Got json error: string or '}' expected near end of file", celix_err_popLastError());
        ASSERT_EQ(1, rc);
        celix_err_resetErrors();

        rc = jsonRpc_call(intf, &serv, R"({"a": [1.0,2.0]})", &result);
        EXPECT_STREQ("Error getting method signature", celix_err_popLastError());
        ASSERT_EQ(1, rc);
        celix_err_resetErrors();

        //request missing argument
        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;"})", &result);
        EXPECT_STREQ("Error getting arguments array for stats([D)LStatsResult;", celix_err_popLastError());
        ASSERT_EQ(1, rc);
        celix_err_resetErrors();

        //request non-array argument
        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": "hello"})", &result);
        EXPECT_STREQ("Error getting arguments array for stats([D)LStatsResult;", celix_err_popLastError());
        ASSERT_EQ(1, rc);
        celix_err_resetErrors();

        // argument number mismatch
        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": []})", &result);
        ASSERT_EQ(1, rc);
        EXPECT_STREQ("Wrong number of standard arguments for stats([D)LStatsResult;. Expected 1, got 0", celix_err_popLastError());
        celix_err_resetErrors();

        //request argument type mismatch
        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [1.0]})", &result);
        ASSERT_EQ(1, rc);
        EXPECT_STREQ("Error deserializing argument 1 for stats([D)LStatsResult;", celix_err_popLastError());
        EXPECT_STREQ("Expected json array type got '4'", celix_err_popLastError());
        celix_err_resetErrors();

        dynInterface_destroy(intf);
    }

    void handleTestOut(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        dyn_function_type *func = dynInterface_findMethod(intf, "stats([D)LStatsResult;")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({"r":{"input":[1.0,2.0],"max":2.0,"average":1.5,"min":1.0}})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        struct tst_StatsResult *result = nullptr;
        void *out = &result;
        args[2] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        ASSERT_EQ(1.5, result->average);

        free(result->input.buf);
        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestOutWithEmptyReply(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        dyn_function_type *func = dynInterface_findMethod(intf, "stats([D)LStatsResult;")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        struct tst_StatsResult *result = nullptr;
        void *out = &result;
        args[2] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_NE(0, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynInterface_destroy(intf);
    }

    void handleTestReplyError(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);


        dyn_function_type *func = dynInterface_findMethod(intf, "add(DD)D")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({"e":33554433})";

        void *args[4];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;
        args[3] = nullptr;

        double result = 0;
        void *out = &result;
        args[3] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(33554433, rsErrno);
        ASSERT_EQ(0, result);

        dynInterface_destroy(intf);
    }

    void callTestUnknownRequest(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"unknown", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(1, rc);
        EXPECT_STREQ("Cannot find method with sig 'unknown'", celix_err_popLastError());

        dynInterface_destroy(intf);
    }

    static void handleTestOutputSequence() {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example2.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        dyn_function_type *func = dynInterface_findMethod(intf, "example1")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({"r":[{"a":1.0,"b":1.5},{"a":2.0,"b":2.5}]})";

        void *args[2];
        args[0] = nullptr;
        args[1] = nullptr;

        struct item_seq *result = nullptr;
        void *out = &result;
        args[1] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        ASSERT_EQ(2, result->len);
        ASSERT_EQ(1.0, result->buf[0]->a);
        ASSERT_EQ(1.5, result->buf[0]->b);
        ASSERT_EQ(2.0, result->buf[1]->a);
        ASSERT_EQ(2.5, result->buf[1]->b);


        unsigned int i;
        for (i = 0; i < result->len; i +=1 ) {
            free(result->buf[i]);
        }
        free(result->buf);
        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestAction(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example5.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        dyn_function_type *func = dynInterface_findMethod(intf, "action(V)")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({})";

        void *args[1];
        args[0] = nullptr;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);

        dynInterface_destroy(intf);
    }

    void callTestOutChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, getName_example4, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m": "getName(V)t", "a": []})", &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "allocatedInFunction") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }


    void handleTestOutChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        dyn_function_type *func = dynInterface_findMethod(intf, "getName(V)t")->dynFunc;
        assert(func != nullptr);

        const char *reply = R"({"r": "this is a test string"})";
        char *result = nullptr;
        void *out = &result;

        void *args[2];
        args[0] = nullptr;
        args[1] = &out;

        int rsErrno = 0;
        jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rsErrno);

        ASSERT_STREQ("this is a test string", result);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, nullptr,[](void*, char *name)->int {
            EXPECT_STREQ(name, "hello");
            free(name);
            return 0;
        }, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m": "setName", "a":["hello"]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        dynInterface_destroy(intf);
    }

    void callTestConstChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, nullptr, nullptr, [](void*, const char *name)->int {
            EXPECT_STREQ(name, "hello const char");
            return 0;
        }};

        rc = jsonRpc_call(intf, &serv, R"({"m": "setConstName", "a":["hello const char"]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        dynInterface_destroy(intf);
    }
}

class JsonRpcTests : public ::testing::Test {
public:
    JsonRpcTests() {

    }
    ~JsonRpcTests() override {
        celix_err_resetErrors();
    }

};

TEST_F(JsonRpcTests, prepareTest) {
    prepareTest();
}

TEST_F(JsonRpcTests, prepareCharTest) {
    prepareCharTest();
}

TEST_F(JsonRpcTests, prepareConstCharTest) {
    prepareConstCharTest();
}

TEST_F(JsonRpcTests, prepareTestFailed) {
    prepareTestFailed();
}

TEST_F(JsonRpcTests, preparePropertiesTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("setProps(#am=handle;Pp)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    char *result = nullptr;

    void *handle = nullptr;
    auto props = celix_properties_create();//owner is callee
    ASSERT_NE(nullptr, props);
    celix_properties_set(props, "string", "value");
    celix_properties_setLong(props, "long", 42);
    celix_properties_setBool(props, "bool", true);
    void *args[2];
    args[0] = &handle;
    args[1] = &props;

    rc = jsonRpc_prepareInvokeRequest(dynFunc, "setProps", args, &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, R"("setProps")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("string")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("value")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("long")") != nullptr);
    ASSERT_TRUE(strstr(result, R"(42)") != nullptr);
    ASSERT_TRUE(strstr(result, R"("bool")") != nullptr);
    ASSERT_TRUE(strstr(result, R"(true)") != nullptr);

    free(result);
    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, prepareConstPropertiesTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("setProps(#am=handle;P#const=true;p)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    char *result = nullptr;

    void *handle = nullptr;
    celix_autoptr(celix_properties_t) props = celix_properties_create();//owner is caller
    ASSERT_NE(nullptr, props);
    celix_properties_set(props, "string", "value");
    celix_properties_setLong(props, "long", 42);
    celix_properties_setBool(props, "bool", true);
    void *args[2];
    args[0] = &handle;
    args[1] = &props;

    rc = jsonRpc_prepareInvokeRequest(dynFunc, "setProps", args, &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, R"("setProps")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("string")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("value")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("long")") != nullptr);
    ASSERT_TRUE(strstr(result, R"(42)") != nullptr);
    ASSERT_TRUE(strstr(result, R"("bool")") != nullptr);
    ASSERT_TRUE(strstr(result, R"(true)") != nullptr);

    free(result);
    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, prepareArrayListTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("setArrayList(#am=handle;Pa)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    char *result = nullptr;

    void *handle = nullptr;
    auto arrList = celix_arrayList_createStringArray();//owner is callee
    ASSERT_NE(nullptr, arrList);
    celix_arrayList_addString(arrList, "value1");
    celix_arrayList_addString(arrList, "value2");
    void *args[2];
    args[0] = &handle;
    args[1] = &arrList;

    rc = jsonRpc_prepareInvokeRequest(dynFunc, "setArrayList", args, &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, R"("setArrayList")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("value1")") != nullptr);
    ASSERT_TRUE(strstr(result, R"("value2")") != nullptr);

    free(result);
    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, prepareConstArrayListTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("setArrayList(#am=handle;P#const=true;a)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    char *result = nullptr;

    void *handle = nullptr;
    celix_autoptr(celix_array_list_t) arrList = celix_arrayList_createLongArray();//owner is caller
    ASSERT_NE(nullptr, arrList);
    celix_arrayList_addLong(arrList, 1);
    celix_arrayList_addLong(arrList, 2);
    void *args[2];
    args[0] = &handle;
    args[1] = &arrList;

    rc = jsonRpc_prepareInvokeRequest(dynFunc, "setArrayList", args, &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, R"("setArrayList")") != nullptr);
    ASSERT_TRUE(strstr(result, R"(1)") != nullptr);
    ASSERT_TRUE(strstr(result, R"(2)") != nullptr);

    free(result);
    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleTestPre) {
    handleTestPre();
}

TEST_F(JsonRpcTests, handleTestNullPre) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = "{\"r\":2.2}";
    double *out = NULL;
    void *args[4];
    args[3] = &out;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    //ASSERT_EQ(2.2, result);

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleTestInvalidReply) {
    handleTestInvalidReply();
}

TEST_F(JsonRpcTests, handleTestOut) {
    handleTestOut();
}

TEST_F(JsonRpcTests, handleTestNullOutResult) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    dyn_function_type *func = dynInterface_findMethod(intf, "stats([D)LStatsResult;")->dynFunc;
    ASSERT_TRUE(func != nullptr);

    const char *reply = R"({"r":null})";

    void *args[3];
    args[0] = nullptr;
    args[1] = nullptr;
    args[2] = nullptr;

    struct tst_StatsResult *result = nullptr;
    void *out = &result;
    args[2] = &out;

    int rsErrno = 0;
    rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_EQ(nullptr, result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, handleTestNullOut) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    dyn_function_type *func = dynInterface_findMethod(intf, "stats([D)LStatsResult;")->dynFunc;
    assert(func != nullptr);

    const char *reply = R"({"r":{"input":[1.0,2.0],"max":2.0,"average":1.5,"min":1.0}})";

    void *args[3];
    args[0] = nullptr;
    args[1] = nullptr;
    args[2] = nullptr;

    void *out = nullptr;
    args[2] = &out;

    int rsErrno = 0;
    rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, handlePropertiesOutTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("getProps(#am=handle;P#am=out;*p)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = R"({"r":{"string":"value","long":1,"bool":true}})";
    celix_autoptr(celix_properties_t) out = nullptr;
    auto outPtr = &out;
    void *args[2];
    args[1] = &outPtr;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_NE(nullptr, out);
    ASSERT_STREQ("value", celix_properties_get(out, "string", nullptr));
    ASSERT_EQ(1, celix_properties_getAsLong(out, "long", -1));
    ASSERT_TRUE(celix_properties_getAsBool(out, "bool", false));

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleNullPropertiesOutTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("getProps(#am=handle;P#am=out;*p)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = R"({"r":null})";
    celix_properties_t* out = nullptr;
    auto outPtr = &out;
    void *args[2];
    args[1] = &outPtr;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_EQ(nullptr, out);

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleArrayListOutTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("getArrayList(#am=handle;P#am=out;*a)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = R"({"r":[1,2,3]})";
    celix_autoptr(celix_array_list_t) out = nullptr;
    auto outPtr = &out;
    void *args[2];
    args[1] = &outPtr;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_NE(nullptr, out);
    ASSERT_EQ(1, celix_arrayList_getLong(out, 0));
    ASSERT_EQ(2, celix_arrayList_getLong(out, 1));
    ASSERT_EQ(3, celix_arrayList_getLong(out, 2));

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleEmptyArrayListOutTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("getArrayList(#am=handle;P#am=out;*a)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = R"({"r":[]})";
    celix_array_list_t* out = nullptr;
    auto outPtr = &out;
    void *args[2];
    args[1] = &outPtr;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_NE(0, rc);

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, handleNullArrayListOutTest) {
    dyn_function_type *dynFunc = nullptr;
    int rc = dynFunction_parseWithStr("getArrayList(#am=handle;P#am=out;*a)N", nullptr, &dynFunc);
    ASSERT_EQ(0, rc);

    const char *reply = R"({"r":null})";
    celix_array_list_t* out = nullptr;
    auto outPtr = &out;
    void *args[2];
    args[1] = &outPtr;
    int rsErrno = 0;
    rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_EQ(nullptr, out);

    dynFunction_destroy(dynFunc);
}

TEST_F(JsonRpcTests, callPreReference) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example7.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(strstr(result, "3.0") != nullptr);

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callPre) {
    callTestPreAllocated();
}

TEST_F(JsonRpcTests, callPreWithMismatchedArgumentNumber) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0,3.0]})", &result);
    EXPECT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    EXPECT_STREQ("Wrong number of standard arguments for add(DD)D. Expected 2, got 3", celix_err_popLastError());
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callFailedPre) {
    callFailedTestPreAllocated();
}

TEST_F(JsonRpcTests, callOut) {
    callTestOutput();
}

TEST_F(JsonRpcTests, callOutNullResult) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, nullptr, nullptr, nullptr, [](void*, struct tst_seq, struct tst_StatsResult **out)->int {
        assert(out != nullptr);
        assert(*out == nullptr);
        *out = nullptr;
        return 0;
    }};

    rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [[1.0,2.0]]})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_is_null(json_object_get(replyJson, "r")));
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callOutReference) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example7.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, nullptr, nullptr, nullptr, stats};

    rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [[1.0,2.0]]})", &result);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(strstr(result, "1.5") != nullptr);

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callTestInvalidRequest) {
    callTestInvalidRequest();
}

TEST_F(JsonRpcTests, callTestUnknownRequest) {
    callTestUnknownRequest();
}

TEST_F(JsonRpcTests, handleOutSeq) {
    handleTestOutputSequence();
}

TEST_F(JsonRpcTests, callTestOutText) {
    callTestOutChar();
}

TEST_F(JsonRpcTests, callTestOutNullTextResult) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example4.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example4 serv {nullptr, [](void *, char** result)->int {
        *result = nullptr;
        return 0;
        }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m": "getName(V)t", "a": []})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_is_null(json_object_get(replyJson, "r")));
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, handleOutText) {
    handleTestOutChar();
}

TEST_F(JsonRpcTests, handleNullOutTextResult) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example4.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    dyn_function_type *func = dynInterface_findMethod(intf, "getName(V)t")->dynFunc;
    assert(func != nullptr);

    const char *reply = R"({"r":null})";
    char *result = nullptr;
    void *out = &result;

    void *args[2];
    args[0] = nullptr;
    args[1] = &out;

    int rsErrno = 0;
    jsonRpc_handleReply(func, reply, args, &rsErrno);
    ASSERT_EQ(0, rsErrno);
    EXPECT_EQ(nullptr, result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, handleInvalidOutChar) {

    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example4.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    dyn_function_type *func = dynInterface_findMethod(intf, "getName(V)t")->dynFunc;
    assert(func != nullptr);

    const char *reply = R"({"r": 12345})";
    char *result = nullptr;
    void *out = &result;

    void *args[2];
    args[0] = nullptr;
    args[1] = &out;

    if (func != nullptr) { // Check needed just to satisfy Coverity
        int rsErrno = 0;
        int status = jsonRpc_handleReply(func, reply, args, &rsErrno);
        EXPECT_NE(0, status);
    }
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, handleReplyError) {
    handleTestReplyError();
}

TEST_F(JsonRpcTests, handleTestOutWithEmptyReply) {
    handleTestOutWithEmptyReply();
}

TEST_F(JsonRpcTests, handleTestAction) {
    handleTestAction();
}

TEST_F(JsonRpcTests, callTestChar) {
    callTestChar();
}

TEST_F(JsonRpcTests, callTestConstChar) {
    callTestConstChar();
}

TEST_F(JsonRpcTests, callWithTooManyArguments) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/invalids/methodWithTooManyArgs.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"add(DDDDDDDDDDDDDDD)D", "a": [1.0,2.0]})", &result);
    ASSERT_NE(0, rc);
    EXPECT_EQ(nullptr, result);
    EXPECT_STREQ("Too many arguments for add(DDDDDDDDDDDDDDD)D: 17 > 16", celix_err_popLastError());
    dynInterface_destroy(intf);
}

typedef struct tst_serv_example8 {
    void *handle;
    int (*getProps)(void* handle, celix_properties_t** props);
    int (*setProps)(void* handle, celix_properties_t* props);
    int (*getConstProps)(void* handle, const celix_properties_t* props);
} tst_serv_example8_t;

TEST_F(JsonRpcTests, callPropertiesOutTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, [](void*, celix_properties_t** props)->int {
        *props = celix_properties_create();
        celix_properties_set(*props, "string", "value");
        celix_properties_setLong(*props, "long", 42);
        celix_properties_setBool(*props, "bool", true);
        return 0;
    }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"getProps", "a": []})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    auto propsJson = json_object_get(replyJson, "r");
    ASSERT_TRUE(json_is_object(propsJson));
    ASSERT_STREQ("value", json_string_value(json_object_get(propsJson, "string")));
    ASSERT_EQ(42, json_integer_value(json_object_get(propsJson, "long")));
    ASSERT_TRUE(json_is_true(json_object_get(propsJson, "bool")));

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callPropertiesInputTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, nullptr, [](void*, celix_properties_t* props)->int {
        EXPECT_STREQ("value", celix_properties_get(props, "string", nullptr));
        EXPECT_EQ(1, celix_properties_getAsLong(props, "long", -1));
        EXPECT_TRUE(celix_properties_getAsBool(props, "bool", false));
        celix_properties_destroy(props);
        return 0;
    }, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setProps", "a": [{"string":"value","long":1,"bool":true}]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callConstPropertiesInputTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, nullptr, nullptr, [](void*, const celix_properties_t* props)->int {
        EXPECT_STREQ("value", celix_properties_get(props, "string", nullptr));
        EXPECT_EQ(1, celix_properties_getAsLong(props, "long", -1));
        EXPECT_TRUE(celix_properties_getAsBool(props, "bool", false));
        return 0;
    }};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setConstProps", "a": [{"string":"value","long":1,"bool":true}]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callPropertiesOutNullResultTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, [](void*, celix_properties_t** props)->int {
        *props = nullptr;
        return 0;
    }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"getProps", "a": []})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_is_null(json_object_get(replyJson, "r")));

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callPropertiesSetNullTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, nullptr, [](void*, celix_properties_t* props)->int {
        EXPECT_EQ(nullptr, props);
        return 0;
    }, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setProps", "a": [null]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callConstPropertiesSetNullTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example8.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example8 serv {nullptr, nullptr, nullptr, [](void*, const celix_properties_t* props)->int {
        EXPECT_EQ(nullptr, props);
        return 0;
    }};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setConstProps", "a": [null]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

typedef struct tst_serv_example9 {
    void *handle;
    int (*getArrayList)(void* handle, celix_array_list_t** arrList);
    int (*setArrayList)(void* handle, celix_array_list_t* arrList);
    int (*getConstArrayList)(void* handle, const celix_array_list_t* arrList);
} tst_serv_example9_t;

TEST_F(JsonRpcTests, callArrayListOutTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, [](void*, celix_array_list_t** arrList)->int {
        *arrList = celix_arrayList_createLongArray();
        celix_arrayList_addLong(*arrList, 1);
        celix_arrayList_addLong(*arrList, 2);
        celix_arrayList_addLong(*arrList, 3);
        return 0;
    }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"getArrayList", "a": []})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    auto arrListJson = json_object_get(replyJson, "r");
    ASSERT_TRUE(json_is_array(arrListJson));
    ASSERT_EQ(1, json_integer_value(json_array_get(arrListJson, 0)));
    ASSERT_EQ(2, json_integer_value(json_array_get(arrListJson, 1)));
    ASSERT_EQ(3, json_integer_value(json_array_get(arrListJson, 2)));

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callArrayListInputTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, nullptr, [](void*, celix_array_list_t* arrList)->int {
        EXPECT_EQ(1, celix_arrayList_getLong(arrList, 0));
        EXPECT_EQ(2, celix_arrayList_getLong(arrList, 1));
        EXPECT_EQ(3, celix_arrayList_getLong(arrList, 2));
        celix_arrayList_destroy(arrList);
        return 0;
    }, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setArrayList", "a": [[1,2,3]]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callConstArrayListInputTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, nullptr, nullptr, [](void*, const celix_array_list_t* arrList)->int {
        EXPECT_EQ(1, celix_arrayList_getLong(arrList, 0));
        EXPECT_EQ(2, celix_arrayList_getLong(arrList, 1));
        EXPECT_EQ(3, celix_arrayList_getLong(arrList, 2));
        return 0;
    }};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setConstArrayList", "a": [[1,2,3]]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callArrayListOutNullResultTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, [](void*, celix_array_list_t** arrList)->int {
        *arrList = nullptr;
        return 0;
    }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"getArrayList", "a": []})", &result);
    ASSERT_EQ(0, rc);

    json_auto_t* replyJson = json_loads(result, JSON_DECODE_ANY, nullptr);
    EXPECT_TRUE(json_is_null(json_object_get(replyJson, "r")));

    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callArrayListSetNullTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, nullptr, [](void*, celix_array_list_t* arrList)->int {
        EXPECT_EQ(nullptr, arrList);
        return 0;
    }, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setArrayList", "a": [null]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callConstArrayListSetNullTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, nullptr, nullptr, [](void*, const celix_array_list_t* arrList)->int {
        EXPECT_EQ(nullptr, arrList);
        return 0;
    }};

    rc = jsonRpc_call(intf, &serv, R"({"m":"setConstArrayList", "a": [null]})", &result);
    ASSERT_EQ(0, rc);
    free(result);
    dynInterface_destroy(intf);
}

TEST_F(JsonRpcTests, callEmptyArrayListOutTest) {
    dyn_interface_type *intf = nullptr;
    FILE *desc = fopen("descriptors/example9.descriptor", "r");
    ASSERT_TRUE(desc != nullptr);
    int rc = dynInterface_parse(desc, &intf);
    ASSERT_EQ(0, rc);
    fclose(desc);

    char *result = nullptr;
    tst_serv_example9 serv {nullptr, [](void*, celix_array_list_t** arrList)->int {
        *arrList = celix_arrayList_createLongArray();//empty list
        return 0;
    }, nullptr, nullptr};

    rc = jsonRpc_call(intf, &serv, R"({"m":"getArrayList", "a": []})", &result);
    ASSERT_NE(0, rc);

    dynInterface_destroy(intf);
}