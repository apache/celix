/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <stdarg.h>

    #include "dyn_common.h"
    #include "dyn_function.h"

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

const char* theAvprFile = "{ \
                \"protocol\" : \"types\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Void\", \
                    \"alias\" : \"void\", \
                    \"size\" : 4 \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"VoidPtr\", \
                    \"alias\" : \"void_ptr\", \
                    \"size\" : 4 \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Sint\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"NativeInt\", \
                    \"size\" : 4, \
                    \"alias\" : \"native_int\" \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"String\", \
                    \"alias\" : \"string\", \
                    \"size\" : 8 \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Double\", \
                    \"alias\" : \"double\", \
                    \"size\" : 8 \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"StructString\", \
                    \"fields\" : [ { \
                          \"name\" : \"name\", \
                          \"type\" : \"String\" \
                        } , { \
                          \"name\" : \"id\", \
                          \"type\" : \"Sint\" \
                    } ] \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"AStruct\", \
                    \"namespace\" : \"nested.test.dt\", \
                    \"fields\" : [ { \
                          \"name\" : \"val1\", \
                          \"type\" : \"test.dt.Sint\" \
                        } , { \
                          \"name\" : \"val2\", \
                          \"type\" : \"test.dt.Sint\" \
                        } , { \
                          \"name\" : \"val3\", \
                          \"type\" : \"test.dt.Double\" \
                    } ] \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"MArray\", \
                    \"fields\" : [ { \
                          \"name\" : \"array\", \
                          \"type\" : { \
                            \"type\" : \"array\",\
                            \"items\" : \"Double\"\
                          } \
                    } ] \
                  } ] ,\
                \"messages\" : { \
                    \"simpleFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Sint\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"Sint\" \
                            } , { \
                                \"name\" : \"arg3\", \
                                \"type\" : \"Sint\" \
                            } ],\
                        \"response\" : \"Sint\" \
                    }, \
                    \"structFunc\" : {\
                        \"namespace\" : \"nested.test.dt\", \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"test.dt.Sint\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"AStruct\" \
                            } , { \
                                \"name\" : \"arg3\", \
                                \"type\" : \"test.dt.Double\" \
                            } ],\
                        \"response\" : \"test.dt.Double\" \
                    }, \
                    \"accessFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Double\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"nested.test.dt.AStruct\" \
                            } , { \
                                \"name\" : \"arg3\", \
                                \"type\" : \"Double\" \
                            } ],\
                        \"response\" : \"test.dt.Void\" \
                    }, \
                    \"outFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"VoidPtr\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"Double\" \
                            } , { \
                                \"name\" : \"arg3\", \
                                \"type\" : \"Double\", \
                                \"ptr\" : true \
                            } ],\
                        \"response\" : \"NativeInt\" \
                    }, \
                    \"seqFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"MArray\" \
                            } ],\
                        \"response\" : \"Void\" \
                    }, \
                    \"infoFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"int\" \
                            } ],\
                        \"response\" : \"Double\" \
                    }, \
                    \"arrayInFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : { \
                                  \"type\" : \"array\",\
                                  \"items\" : \"Double\" \
                                } \
                            } ],\
                        \"response\" : \"Double\" \
                    }, \
                    \"arrayOutFunc\" : {\
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Double\" \
                            } ],\
                        \"response\" : { \
                              \"type\" : \"array\",\
                              \"items\" : \"Double\" \
                            } \
                    }, \
                    \"stringOutFunc\" : {\
                        \"request\" : [ ],\
                        \"response\" : \"string\" \
                    }, \
                    \"structStringOutFunc\" : {\
                        \"request\" : [ ],\
                        \"response\" : \"StructString\" \
                    } \
                }\
                }";

TEST_GROUP(DynAvprFunctionTests) {
    void setup() override {
        int lvl = 1;
        dynAvprFunction_logSetup(stdLog, nullptr, lvl);
        dynAvprType_logSetup(stdLog, nullptr, lvl);
    }
};

// Test 1, simple function with three arguments and a return type
static int avpr_example1(__attribute__((unused)) void* handle, int32_t a, int32_t b, int32_t c, int32_t * out) {
    CHECK_EQUAL(2, a);
    CHECK_EQUAL(4, b);
    CHECK_EQUAL(8, c);
    *out = 1;
    return 0;
}

TEST(DynAvprFunctionTests, Example1) {
    auto fp = (void (*)()) avpr_example1;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.simpleFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;
    int32_t a = 2;
    int32_t b = 4;
    int32_t c = 8;
    int32_t out = 0;
    int32_t * p_out = &out;
    void *values[5];
    values[0] = &handle_ptr;
    values[1] = &a;
    values[2] = &b;
    values[3] = &c;
    values[4] = &p_out;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, values);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(1, out);
    CHECK_EQUAL(0, rVal);
    dynFunction_destroy(dynFunc);
}


// Test 2, function with complex argument
struct avpr_example2_arg2 {
    int32_t val1;
    int32_t val2;
    double val3;
};

static int avpr_example2(__attribute__((unused)) void* handle, int32_t arg1, struct avpr_example2_arg2 arg2, double arg3, double* out) {
    CHECK_EQUAL(2, arg1);
    CHECK_EQUAL(2, arg2.val1);
    CHECK_EQUAL(3, arg2.val2);
    CHECK_EQUAL(4.1, arg2.val3);
    CHECK_EQUAL(8.1, arg3);
    *out = 2.2;
    return 0;
}

TEST(DynAvprFunctionTests, Example2) {
    auto fp = (void (*)()) avpr_example2;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "nested.test.dt.structFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;
    int32_t a = 2;
    avpr_example2_arg2 arg2 {2, 3, 4.1};

    double c = 8.1;
    double out = 0.0;
    double* p_out = &out;
    void *values[5];
    values[0] = &handle_ptr;
    values[1] = &a;
    values[2] = &arg2;
    values[3] = &c;
    values[4] = &p_out;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, values);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(0, rVal);
    CHECK_EQUAL(2.2, out);
    dynFunction_destroy(dynFunc);
}

// Test 3, Test access of functions, see if arguments and result type can be accessed
TEST(DynAvprFunctionTests, Example3) {
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.accessFunc");
    CHECK(dynFunc != nullptr);

    int nrOfArgs = dynFunction_nrOfArguments(dynFunc);
    CHECK_EQUAL(3+1+1, nrOfArgs);

    dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 2);
    CHECK(arg1 != nullptr);
    CHECK_EQUAL('{', (char) dynType_descriptorType(arg1));

    dyn_type *nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
    CHECK(nonExist == nullptr);

    dyn_type *returnType = dynFunction_returnType(dynFunc);
    CHECK_EQUAL('N', (char) dynType_descriptorType(returnType));

    dynFunction_destroy(dynFunc);
}


// Test 4, with void pointer and native output
/*
static int avpr_example4(__attribute__((unused)) void *handle, void *ptr, double a, double *out, int *out_2) {
    auto b = (double *)ptr;
    CHECK_EQUAL(2.0, *b);
    CHECK_EQUAL(2.0, a);
    *out = *b * a; // => out parameter
    *out_2 = 3;
    return 0;
}

TEST(DynAvprFunctionTests, Example4) {
    auto fp = (void(*)()) avpr_example4;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.outFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;
    double result = -1.0;
    double *input = &result;
    double a = 2.0;
    void *ptr = &a;
    void *args[5];
    int out = 0;
    int* out_ptr = &out;
    args[0] = &handle_ptr;
    args[1] = &ptr;
    args[2] = &a;
    args[3] = &input;
    args[4] = &out_ptr;
    int rVal = 1;
    int rc = dynFunction_call(dynFunc, fp, &rVal, args);

    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(4.0, result);
    CHECK_EQUAL(3, out);

    auto inMemResult = (double *)calloc(1, sizeof(double));
    a = 2.0;
    ptr = &a;
    args[1] = &ptr;
    args[2] = &a;
    args[3] = &inMemResult;
    rVal = 0;
    rc = dynFunction_call(dynFunc, fp, &rVal, args);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(4.0, result);
    free(inMemResult);
    dynFunction_destroy(dynFunc);
}
*/

// Test5, testing with sequence as argument
struct tst_seq {
    uint32_t cap;
    uint32_t len;
    double *buf;
};

static int avpr_example5(__attribute__((unused)) void* handle, struct tst_seq seq, __attribute__((unused)) void* out)  {
    CHECK_EQUAL(4, seq.cap);
    CHECK_EQUAL(2, seq.len);
    CHECK_EQUAL(1.1, seq.buf[0]);
    CHECK_EQUAL(2.2, seq.buf[1]);
    return 0;
}

TEST(DynAvprFunctionTests, Example5) {
    auto fp = (void(*)()) avpr_example5;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.seqFunc");

    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;
    double buf[4];
    buf[0] = 1.1;
    buf[1] = 2.2;
    struct tst_seq seq {4, 2, buf};
    int out = 0;
    int* out_ptr = &out;

    void *args[3];
    args[0] = &handle_ptr;
    args[1] = &seq;
    args[2] = &out_ptr;
    int retArg = 1;

    int rc = dynFunction_call(dynFunc, fp, &retArg, args);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(0, retArg);

    dynFunction_destroy(dynFunc);
}

// Test 6, Test data accessible with handle
struct info {
    char * name;
    int id;
};

static int avpr_example6(void *handle, int32_t id_modifier, double* out) {
    auto fake_this = (struct info *) handle;
    CHECK(fake_this != nullptr);
    CHECK(fake_this->name != nullptr);
    CHECK_EQUAL(0, strcmp(fake_this->name, "test_name"));
    CHECK_EQUAL(42, fake_this->id);
    *out = (double)(fake_this->id + id_modifier);
    return 0;
}

TEST(DynAvprFunctionTests, Example6) {
    auto fp = (void(*)()) avpr_example6;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.infoFunc");
    CHECK(dynFunc != nullptr);

    info handle {strdup("test_name"), 42};
    CHECK(handle.name != nullptr);

    struct info * handle_ptr = &handle;
    int32_t argument = 58;
    double out = 0.0;
    double * out_ptr = &out;

    void *values[3];
    values[0] = &handle_ptr;
    values[1] = &argument;
    values[2] = &out_ptr;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, values);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(100.0, out);
    CHECK_EQUAL(0, rVal);
    dynFunction_destroy(dynFunc);
    free(handle.name);
}

//Test 7, Test function with array as parameter
struct double_seq {
    uint32_t cap;
    uint32_t len;
    double *buf;
};

static int avpr_example7(__attribute__((unused)) void *handle, struct double_seq seq_in, double* out) {
    CHECK_EQUAL(2, seq_in.cap);
    CHECK_EQUAL(2, seq_in.len);
    *out = seq_in.buf[0] + seq_in.buf[1];
    return 0;
}

TEST(DynAvprFunctionTests, Example7) {
    auto fp = (void(*)()) avpr_example7;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.arrayInFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;

    double buf[2];
    buf[0] = 1.101;
    buf[1] = 2.202;
    double_seq s_seq {2, 2, buf};

    double out = 0.0;
    double *out_ptr = &out;

    void *args[3];
    args[0] = &handle_ptr;
    args[1] = &s_seq;
    args[2] = &out_ptr;
    int rVal = 0;

    int rc = dynFunction_call(dynFunc, fp, &rVal, args);
    CHECK_EQUAL(0, rc);
    DOUBLES_EQUAL(3.303, out, 0.00001);

    dynFunction_destroy(dynFunc);
}

//Test 8, Test function with array as return value
static int avpr_example8(__attribute__((unused))void *handle, double arg1, struct double_seq** out) {
    DOUBLES_EQUAL(2.0, arg1, 0.0001);
    CHECK_EQUAL(3, (*out)->cap);
    (*out)->buf[0] = 0.0;
    (*out)->buf[1] = 1.1;
    (*out)->buf[2] = 2.2;
    return 0;
}

TEST(DynAvprFunctionTests, Example8) {
    auto fp = (void(*)()) avpr_example8;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.arrayOutFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;

    double arg1 = 2.0;

    double buf[3];
    buf[0] = -1.1;
    buf[1] = 0.0;
    buf[2] = 0.0;
    double_seq dseq {3, 3, buf};
    struct double_seq * out_ptr = &dseq;
    struct double_seq ** out_ptr_ptr = &out_ptr;

    void *args[3];
    args[0] = &handle_ptr;
    args[1] = &arg1;
    args[2] = &out_ptr_ptr;
    int rVal = 0;

    int rc = dynFunction_call(dynFunc, fp, &rVal, args);
    CHECK_EQUAL(0, rc);
    DOUBLES_EQUAL(0.0, buf[0], 0.0001);
    DOUBLES_EQUAL(1.1, buf[1], 0.0001);
    DOUBLES_EQUAL(2.2, buf[2], 0.0001);

    dynFunction_destroy(dynFunc);
}

// Test 9, Test function with string as return value
static int avpr_example9(__attribute__((unused))void *handle, char** out) {
    *out = strdup("result_out");
    CHECK(*out != nullptr);
    return 0;
}

TEST(DynAvprFunctionTests, Example9) {
    auto fp = (void(*)()) avpr_example9;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.stringOutFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;

    char * out = nullptr;
    char ** out_ptr = &out;

    void *args[2];
    args[0] = &handle_ptr;
    args[1] = &out_ptr;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, args);
    CHECK_EQUAL(0, rc);
    STRCMP_EQUAL("result_out", out);

    free(out);
    dynFunction_destroy(dynFunc);
}

// Test 10, Test function with object with string as return value
extern "C" {
    struct tst_10 {
        char * name;
        int32_t id;
    };
}

static int avpr_example10(__attribute__((unused))void *handle, struct tst_10 ** out) {
    (*out)->name = strdup("my_new_char");
    CHECK((*out)->name != nullptr);
    (*out)->id = 132;
    return 0;
}

TEST(DynAvprFunctionTests, Example10) {
    auto fp = (void(*)()) avpr_example10;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.structStringOutFunc");
    CHECK(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;

    tst_10 out {nullptr, 0};
    struct tst_10 * out_ptr = &out;
    struct tst_10 ** out_ptr_ptr = &out_ptr;

    void *args[2];
    args[0] = &handle_ptr;
    args[1] = &out_ptr_ptr;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, args);
    CHECK_EQUAL(0, rc);
    CHECK_EQUAL(132, out.id);
    STRCMP_EQUAL("my_new_char", out.name);

    free(out.name);
    dynFunction_destroy(dynFunc);
}
