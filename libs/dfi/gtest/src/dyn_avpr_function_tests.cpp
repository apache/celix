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

#include <celix_utils.h>

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
                    }, \
                    \"stringInFunc\" : {\
                        \"request\" : [{\
                            \"name\" : \"arg1\",\
                            \"type\" : \"string\" \
                        } ],\
                        \"response\" : \"Void\"\
                    }\
                }\
            }";


class DynAvprFunctionTests : public ::testing::Test {
public:
    DynAvprFunctionTests() {
        int lvl = 1;
        dynAvprFunction_logSetup(stdLog, nullptr, lvl);
        dynAvprType_logSetup(stdLog, nullptr, lvl);
    }
    ~DynAvprFunctionTests() override {
    }

};

// Test 1, simple function with three arguments and a return type
static int avpr_example1(__attribute__((unused)) void* handle, int32_t a, int32_t b, int32_t c, int32_t * out) {
    EXPECT_EQ(2, a);
    EXPECT_EQ(4, b);
    EXPECT_EQ(8, c);
    *out = 1;
    return 0;
}

TEST_F(DynAvprFunctionTests, Example1) {
    auto fp = (void (*)()) avpr_example1;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.simpleFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_EQ(1, out);
    ASSERT_EQ(0, rVal);
    dynFunction_destroy(dynFunc);
}


// Test 2, function with complex argument
struct avpr_example2_arg2 {
    int32_t val1;
    int32_t val2;
    double val3;
};

static int avpr_example2(__attribute__((unused)) void* handle, int32_t arg1, struct avpr_example2_arg2 arg2, double arg3, double* out) {
    EXPECT_EQ(2, arg1);
    EXPECT_EQ(2, arg2.val1);
    EXPECT_EQ(3, arg2.val2);
    EXPECT_EQ(4.1, arg2.val3);
    EXPECT_EQ(8.1, arg3);
    *out = 2.2;
    return 0;
}

TEST_F(DynAvprFunctionTests, Example2) {
    auto fp = (void (*)()) avpr_example2;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "nested.test.dt.structFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, rVal);
    ASSERT_EQ(2.2, out);
    dynFunction_destroy(dynFunc);
}

// Test 3, Test access of functions, see if arguments and result type can be accessed
TEST_F(DynAvprFunctionTests, Example3) {
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.accessFunc");
    ASSERT_TRUE(dynFunc != nullptr);

    int nrOfArgs = dynFunction_nrOfArguments(dynFunc);
    ASSERT_EQ(3+1+1, nrOfArgs);

    dyn_type *arg1 = dynFunction_argumentTypeForIndex(dynFunc, 2);
    ASSERT_TRUE(arg1 != nullptr);
    ASSERT_EQ('{', (char) dynType_descriptorType(arg1));

    dyn_type *nonExist = dynFunction_argumentTypeForIndex(dynFunc, 10);
    ASSERT_TRUE(nonExist == nullptr);

    dyn_type *returnType = dynFunction_returnType(dynFunc);
    ASSERT_EQ('N', (char) dynType_descriptorType(returnType));

    dynFunction_destroy(dynFunc);
}


// Test 4, with void pointer and native output
/*
static int avpr_example4(__attribute__((unused)) void *handle, void *ptr, double a, double *out, int *out_2) {
    auto b = (double *)ptr;
    ASSERT_EQ(2.0, *b);
    ASSERT_EQ(2.0, a);
    *out = *b * a; // => out parameter
    *out_2 = 3;
    return 0;
}

TEST(DynAvprFunctionTests, Example4) {
    auto fp = (void(*)()) avpr_example4;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.outFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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

    ASSERT_EQ(0, rc);
    ASSERT_EQ(4.0, result);
    ASSERT_EQ(3, out);

    auto inMemResult = (double *)calloc(1, sizeof(double));
    a = 2.0;
    ptr = &a;
    args[1] = &ptr;
    args[2] = &a;
    args[3] = &inMemResult;
    rVal = 0;
    rc = dynFunction_call(dynFunc, fp, &rVal, args);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(4.0, result);
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
    EXPECT_EQ(4, seq.cap);
    EXPECT_EQ(2, seq.len);
    EXPECT_EQ(1.1, seq.buf[0]);
    EXPECT_EQ(2.2, seq.buf[1]);
    return 0;
}

TEST_F(DynAvprFunctionTests, Example5) {
    auto fp = (void(*)()) avpr_example5;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.seqFunc");

    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_EQ(0, retArg);

    dynFunction_destroy(dynFunc);
}

// Test 6, Test data accessible with handle
struct info {
    char * name;
    int id;
};

static int avpr_example6(void *handle, int32_t id_modifier, double* out) {
    auto fake_this = (struct info *) handle;
    EXPECT_TRUE(fake_this != nullptr);
    EXPECT_TRUE(fake_this->name != nullptr);
    EXPECT_EQ(0, strcmp(fake_this->name, "test_name"));
    EXPECT_EQ(42, fake_this->id);
    *out = (double)(fake_this->id + id_modifier);
    return 0;
}

TEST_F(DynAvprFunctionTests, Example6) {
    auto fp = (void(*)()) avpr_example6;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.infoFunc");
    ASSERT_TRUE(dynFunc != nullptr);

    info handle {strdup("test_name"), 42};
    ASSERT_TRUE(handle.name != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_EQ(100.0, out);
    ASSERT_EQ(0, rVal);
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
    EXPECT_EQ(2, seq_in.cap);
    EXPECT_EQ(2, seq_in.len);
    *out = seq_in.buf[0] + seq_in.buf[1];
    return 0;
}

TEST_F(DynAvprFunctionTests, Example7) {
    auto fp = (void(*)()) avpr_example7;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.arrayInFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_NEAR(3.303, out, 0.001);

    dynFunction_destroy(dynFunc);
}

//Test 8, Test function with array as return value
static int avpr_example8(__attribute__((unused))void *handle, double arg1, struct double_seq** out) {
    EXPECT_NEAR(2.0, arg1, 0.001);
    EXPECT_EQ(3, (*out)->cap);
    (*out)->buf[0] = 0.0;
    (*out)->buf[1] = 1.1;
    (*out)->buf[2] = 2.2;
    return 0;
}

TEST_F(DynAvprFunctionTests, Example8) {
    auto fp = (void(*)()) avpr_example8;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.arrayOutFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_NEAR(0.0, buf[0], 0.001);
    ASSERT_NEAR(1.1, buf[1], 0.001);
    ASSERT_NEAR(2.2, buf[2], 0.001);

    dynFunction_destroy(dynFunc);
}

// Test 9, Test function with string as return value
static int avpr_example9(__attribute__((unused))void *handle, char** out) {
    *out = strdup("result_out");
    EXPECT_TRUE(*out != nullptr);
    return 0;
}

TEST_F(DynAvprFunctionTests, Example9) {
    auto fp = (void(*)()) avpr_example9;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.stringOutFunc");
    ASSERT_TRUE(dynFunc != nullptr);

    int handle = 0;
    int* handle_ptr = &handle;

    char * out = nullptr;
    char ** out_ptr = &out;

    void *args[2];
    args[0] = &handle_ptr;
    args[1] = &out_ptr;
    int rVal = 1;

    int rc = dynFunction_call(dynFunc, fp, &rVal, args);
    ASSERT_EQ(0, rc);
    ASSERT_STREQ("result_out", out);

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
    EXPECT_TRUE((*out)->name != nullptr);
    (*out)->id = 132;
    return 0;
}

TEST_F(DynAvprFunctionTests, Example10) {
    auto fp = (void(*)()) avpr_example10;
    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.structStringOutFunc");
    ASSERT_TRUE(dynFunc != nullptr);

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
    ASSERT_EQ(0, rc);
    ASSERT_EQ(132, out.id);
    ASSERT_STREQ("my_new_char", out.name);

    free(out.name);
    dynFunction_destroy(dynFunc);
}

//FIXME issue #179
//extern "C" {
//static int avpr_example11(void *handle, char *arg1) {
//    if (handle != nullptr && strncmp("input string test", arg1, 1024) == 0) {
//        return 0;
//    }
//    return 1;
//}
//
//static bool test_example11() {
//    auto fp = (void(*)()) avpr_example11;
//    dyn_function_type * dynFunc = dynFunction_parseAvprWithStr(theAvprFile, "test.dt.stringInFunc");
//
//    int handle = 0;
//    void* handle_ptr = &handle;
//
//    char *input = celix_utils_strdup("input string test");
//
//    void *args[2];
//    args[0] = &handle_ptr;
//    args[1]= &input;
//    int rVal = 1;
//
//    int rc = 0;
//    if (dynFunc != nullptr) {
//        rc = dynFunction_call(dynFunc, fp, &rVal, args);
//        dynFunction_destroy(dynFunc);
//    }
//    free(input);
//
//    return dynFunc != nullptr && rc == 0 && rVal == 0;
//}
//}
//
//TEST_F(DynAvprFunctionTests, Example11) {
//    //NOTE only using libffi with extern C, because combining libffi with EXPECT_*/ASSERT_* call leads to
//    //corrupted memory. Note that libffi is a function for interfacing with C not C++
//    EXPECT_TRUE(test_example11());
//}
