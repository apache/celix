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

extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <stdarg.h>
    #include <assert.h>
    #include <float.h>

    #include <ffi.h>

    #include "dyn_common.h"
    #include "dyn_type.h"
    #include "dyn_interface.h"

    #include "json_serializer.h"
    #include "json_rpc.h"

    static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
        va_list ap;
        const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
        fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }
}

extern "C" {
    const char* sourceAvprFile = "{ \
                \"protocol\" : \"calculator\", \
                \"namespace\" : \"test.rpc\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Double\", \
                    \"alias\": \"double\", \
                    \"size\" : 8 \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"String\", \
                    \"alias\": \"string\", \
                    \"size\" : 8 \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"TwoDoubles\", \
                    \"fields\" : [ { \
                          \"name\" : \"a\", \
                          \"type\" : \"Double\" \
                        } , { \
                          \"name\" : \"b\", \
                          \"type\" : \"Double\" \
                        } ] \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"StatsResult\", \
                    \"fields\" : [ { \
                          \"name\" : \"average\", \
                          \"type\" : \"Double\" \
                        } , { \
                          \"name\" : \"min\", \
                          \"type\" : \"Double\" \
                        } , { \
                          \"name\" : \"max\", \
                          \"type\" : \"Double\" \
                        } , { \
                          \"name\" : \"input\", \
                          \"type\" : { \
                            \"type\" : \"array\",\
                            \"items\" : \"Double\"\
                          } \
                        } ] \
                  } ] ,\
                \"messages\" : { \
                    \"add\" : {\
                        \"index\" : 0, \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Double\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"Double\" \
                            } ],\
                        \"response\" : \"Double\" \
                    }, \
                    \"sub\" : {\
                        \"index\" : 1, \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Double\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"Double\" \
                            } ],\
                        \"response\" : \"Double\" \
                    }, \
                    \"sqrt\" : {\
                        \"index\" : 2, \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Double\" \
                            } ],\
                        \"response\" : \"Double\" \
                    }, \
                    \"stats\" : {\
                        \"index\" : 3, \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : { \
                                  \"type\" : \"array\",\
                                  \"items\" : \"Double\"\
                                } \
                            } ],\
                        \"response\" : \"StatsResult\" \
                    }, \
                    \"retArray\" : {\
                        \"index\" : 0, \
                        \"request\" : [ ],\
                        \"response\" : { \
                                  \"type\" : \"array\",\
                                  \"items\" : \"TwoDoubles\"\
                                } \
                    }, \
                    \"getName\" : {\
                        \"index\" : 0, \
                        \"request\" : [ ],\
                        \"response\" : \"String\" \
                    } \
                } \
            }";
}

class JsonAvprRpcTests : public ::testing::Test {
public:
    JsonAvprRpcTests() {
        int lvl = 1;
        dynCommon_logSetup(stdLog, nullptr, lvl);
        dynType_logSetup(stdLog, nullptr,lvl);
        dynFunction_logSetup(stdLog, nullptr,lvl);
        dynInterface_logSetup(stdLog, nullptr,lvl);
        jsonSerializer_logSetup(stdLog, nullptr, lvl);
        jsonRpc_logSetup(stdLog, nullptr, lvl);
    }
    ~JsonAvprRpcTests() override {
    }

};

TEST_F(JsonAvprRpcTests, prep) {
    dyn_function_type * func = dynFunction_parseAvprWithStr(sourceAvprFile, "test.rpc.add");
    ASSERT_TRUE(func != nullptr);
    int nof_args = dynFunction_nrOfArguments(func);
    ASSERT_EQ(4, nof_args);

    // Msg
    char *result = nullptr;
    void *handle = nullptr;

    double arg1 = 1.0;
    double arg2 = 2.0;

    void *args[4];
    args[0] = &handle;
    args[1] = &arg1;
    args[2] = &arg2;

    int rc = jsonRpc_prepareInvokeRequest(func, "add", args, &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, "\"add\"") != nullptr);
    ASSERT_TRUE(strstr(result, "1.0") != nullptr);
    ASSERT_TRUE(strstr(result, "2.0") != nullptr);

    // Reply
    const char *reply = "{\"r\": 2.2}";
    double calc_result = -1.0;
    double *out = &calc_result;
    //double **out_ptr = &out;
    args[3] = &out;

    int rsErrno = 0;
    rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_NEAR(2.2, calc_result, 0.001);

    free(result);
    dynFunction_destroy(func);
}

// Test handle out
extern "C" {
    struct tst_seq {
        uint32_t cap;
        uint32_t len;
        double *buf;
    };

    struct tst_StatsResult {
        double average;
        double min;
        double max;
        tst_seq input;
    };
}

TEST_F(JsonAvprRpcTests, handle_out) {
    dyn_interface_type * intf = dynInterface_parseAvprWithStr(sourceAvprFile);
    ASSERT_TRUE(intf != nullptr);

    methods_head *head;
    dynInterface_methods(intf, &head);

    dyn_function_type *func = nullptr;
    method_entry *entry = nullptr;
    TAILQ_FOREACH(entry, head, entries) {
        if (strcmp(entry->name, "stats") == 0) {
            func = entry->dynFunc;
            break;
        }
    }
    ASSERT_TRUE(func != nullptr);
    int nof_args = dynFunction_nrOfArguments(func);
    ASSERT_EQ(3, nof_args);

    const char *reply = R"({"r":{"input":[1.0,2.0],"max":2.0,"average":1.5,"min":1.0}})";

    void *args[3];
    args[0] = nullptr;
    args[1] = nullptr;
    args[2] = nullptr;

    struct tst_StatsResult *result = nullptr;
    void *out = &result;
    args[2] = &out;

    int rsErrno = 0;
    int rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rsErrno);
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(1.5, result->average);

    free(result->input.buf);
    free(result);
    dynInterface_destroy(intf);
}


// Test pre allocated

extern "C" {
    struct tst_serv {
        void *handle;
        int (*add)(void *, double, double, double *);
        int (*sub)(void *, double, double, double *);
        int (*sqrt)(void *, double, double *);
        int (*stats)(void *, tst_seq, tst_StatsResult **);
    };

    int xadd(void*, double a, double b, double *result) {
        *result = a + b;
        return 0;
    }
}

TEST_F(JsonAvprRpcTests, preallocated) {
    dyn_interface_type * intf = dynInterface_parseAvprWithStr(sourceAvprFile);
    ASSERT_TRUE(intf != nullptr);

    char *result = nullptr;
    tst_serv serv {nullptr, xadd, nullptr, nullptr, nullptr};

    int rc = jsonRpc_call(intf, &serv, R"({"m": "add", "a": [1.0,2.0]})", &result);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(result != nullptr);
    ASSERT_TRUE(strstr(result, "3.0") != nullptr);

    free(result);
    dynInterface_destroy(intf);
}


// Test output
extern "C" {
    int xstats(void*, struct tst_seq input, struct tst_StatsResult **out) {
        assert(out != nullptr);
        assert(*out == nullptr);
        double total = 0.0;
        unsigned int count = 0;
        auto max = DBL_MIN;
        auto min = DBL_MAX;

        unsigned int i;
        for (i = 0; i<input.len; i += 1) {
            total += input.buf[i];
            count += 1;
            if (input.buf[i] > max) {
                max = input.buf[i];
            }
            if (input.buf[i] < min) {
                min = input.buf[i];
            }
        }

        auto result = static_cast<tst_StatsResult *>(calloc(1, sizeof(tst_StatsResult)));
        EXPECT_TRUE(result != nullptr);
        if (count>0) {
            result->average = total / count;
        }
        result->min = min;
        result->max = max;
        auto buf = static_cast<double *>(calloc(input.len, sizeof(double)));
        EXPECT_TRUE(buf != nullptr);
        memcpy(buf, input.buf, input.len * sizeof(double));
        result->input.len = input.len;
        result->input.cap = input.len;
        result->input.buf = buf;

        *out = result;
        return 0;
    }
}

TEST_F(JsonAvprRpcTests, output) {
    dyn_interface_type * intf = dynInterface_parseAvprWithStr(sourceAvprFile);
    ASSERT_TRUE(intf != nullptr);

    char *result = nullptr;
    tst_serv serv {nullptr, nullptr, nullptr, nullptr, xstats};

    int rc = jsonRpc_call(intf, &serv, R"({"m":"stats", "a": [[1.0, 2.0]]})", &result);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(strstr(result, "1.5") != nullptr);

    free(result);
    dynInterface_destroy(intf);
}

// Test char array as output
extern "C" {
    struct tst_service_ex_output_char {
        void *handle;
        int (*getName_x)(void *, char ** result);
    };

    int getName_ex(void*, char** result) {
        *result = strdup("allocatedInFunction");
        return 0;
    }
}

TEST_F(JsonAvprRpcTests, output_char) {
    dyn_interface_type * intf = dynInterface_parseAvprWithStr(sourceAvprFile);
    ASSERT_TRUE(intf != nullptr);

    char *result = nullptr;
    tst_service_ex_output_char serv {nullptr, getName_ex};

    int rc = jsonRpc_call(intf, &serv, R"({"m" : "getName", "a": []})", &result);
    ASSERT_EQ(0, rc);

    ASSERT_TRUE(strstr(result, "allocatedInFunction") != nullptr);

    free(result);
    dynInterface_destroy(intf);
}

