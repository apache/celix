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

#include <stdarg.h>

#include "dyn_common.h"
#include "dyn_type.h"

static void stdLogA(void*, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static void runTestA(const char *descriptorStr, const char *exName, int expectedType) {
    dyn_type *type;
    type = dynType_parseAvprWithStr(descriptorStr, exName);

    if (type != nullptr) {
        ASSERT_EQ(expectedType, dynType_type(type));
        dynType_destroy(type);
    } else {
        ASSERT_EQ(1, 0);
    }
}

const char* theTestCase = "{ \
                \"protocol\" : \"types\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.1.9\", \
                \"CpfCInclude\" : \"time.h\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"uint\", \
                    \"version\" : \"2.1.9\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"ulong\", \
                    \"size\" : 8, \
                    \"namespace\" : \"blah.test.dt\" \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"double\", \
                    \"alias\" : \"double\", \
                    \"size\" : 8 \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"B\", \
                    \"fields\" : [ \
                    { \
                        \"name\" : \"aa\", \
                        \"type\" : \"uint\" \
                    }, { \
                        \"name\" : \"bb\", \
                        \"type\" : \"uint\" \
                    }, { \
                        \"name\" : \"cc\", \
                        \"type\" : \"double\" \
                    }] \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Alias\", \
                    \"alias\" : \"other.dt.long\", \
                    \"size\" : 7 \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"long\", \
                    \"size\" : 8, \
                    \"namespace\" : \"other.dt\", \
                    \"signed\" : true \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"A\", \
                    \"namespace\" : \"blah.test.dt\", \
                    \"fields\" : [\
                    { \
                        \"name\" : \"x\", \
                        \"type\" : \"test.dt.B\" \
                    },\
                    { \
                        \"name\" : \"y\", \
                        \"type\" : \"ulong\" \
                    }\
                    ] \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"V\", \
                    \"fields\" : [\
                    {\
                        \"name\" : \"elem\", \
                        \"type\" : {\
                            \"type\" : \"array\",\
                            \"items\" : \"uint\",\
                            \"static\" : 3\
                        }\
                    }\
                    ]\
                  }, { \
                    \"type\" : \"enum\", \
                    \"name\" : \"EnumWithValue\", \
                    \"EnumValues\" : [ \"A = 3\", \"B= 1234\" ],\
                    \"symbols\" : [\"A\", \"B\"]\
                  }, { \
                    \"type\" : \"enum\", \
                    \"name\" : \"EnumWithoutValue\", \
                    \"symbols\" : [\"A\", \"B\"]\
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"RecordWithSimpleTypes\", \
                    \"fields\" : [ {\
                        \"name\" : \"data\", \
                        \"type\" :\"int\" \
                    } ]\
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"Anne\", \
                    \"alias\" : \"CoolType\", \
                    \"MsgId\" : 1000, \
                    \"Invalid\" : {}, \
                    \"fields\" : [ {\
                        \"name\" : \"data\", \
                        \"type\" :\"test.dt.double\" \
                    } ]\
                  }, { \
                    \"type\" : \"record\", \
                    \"namespace\" : \"N\", \
                    \"name\" : \"Node\", \
                    \"fields\" : [ {\
                        \"name\" : \"data\", \
                        \"type\" :\"test.dt.double\" \
                      }, {\
                        \"name\" : \"ne_elem_left\", \
                        \"type\" : \"Node\", \
                        \"ptr\" : true \
                      }, {\
                        \"name\" : \"ne_elem_right\", \
                        \"type\" : \"Node\", \
                        \"ptr\" : true \
                    } ]\
                  } ] ,\
                \"messages\" : {} \
            }";

const char* nestedArray = "{\
                \"protocol\" : \"types\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"uint\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"NA\", \
                    \"fields\" : [\
                    { \
                        \"name\" : \"members\", \
                        \"type\" : {\
                            \"type\" : \"array\",\
                            \"items\" : {\
                                \"type\" : \"array\",\
                                \"items\" : \"uint\",\
                                \"static\" : 3\
                            },\
                            \"static\" : 3\
                        }\
                    } ] \
                  }] , \
                \"messages\" : {} \
                           }";

const char* referenceTestCase = "{\
                \"protocol\" : \"types\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"uint\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"X\", \
                    \"fields\" : [\
                    { \
                        \"name\" : \"xx\", \
                        \"type\" : \"uint\" \
                    }, { \
                        \"name\" : \"level_a\", \
                        \"type\" : \"X\", \
                        \"ptr\" : true \
                    }, { \
                        \"name\" : \"level_b\", \
                        \"type\" : \"X\", \
                        \"ptr\" : true \
                    } ] \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"R\", \
                    \"fields\" : [\
                    { \
                        \"name\" : \"data\", \
                        \"type\" : \"X\", \
                        \"ptr\" : true \
                    } ] \
                  }] , \
                \"messages\" : {} \
           }";

const char* aliasTestCase = "{\
                \"protocol\" : \"types\", \
                \"namespace\" : \"common.dt\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Rad\", \
                    \"size\" : 4, \
                    \"alias\" : \"common.dt.Float\" \
                  }, { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Float\", \
                    \"size\" : 4, \
                    \"alias\" : \"float\" \
                  }, { \
                    \"type\" : \"record\", \
                    \"name\" : \"Polar_2d\", \
                    \"fields\" : [ { \
                        \"name\" : \"range\", \
                        \"type\" : \"Float\" \
                    }, { \
                        \"name\" : \"azimuth\", \
                        \"type\" : \"Rad\" \
                    } ] \
                  } ] , \
                \"messages\" : {} \
           }";

const char* arrayIntTestCase = "{\
                \"protocol\" : \"types\", \
                \"namespace\" : \"common.dt\", \
                \"version\" : \"1.0.0\", \
                \"types\" : [ { \
                    \"type\" : \"record\", \
                    \"name\" : \"SimpleArray\", \
                    \"fields\" : [ { \
                        \"name\" : \"array\", \
                        \"type\" : {\
                            \"type\" : \"array\",\
                            \"items\" : \"int\" \
                        } \
                    } ] \
                  } ] , \
                \"messages\" : {} \
           }";


/*********************************************************************************
 * Type building tests
 */

class DynAvprTypeTests : public ::testing::Test {
public:
    DynAvprTypeTests() {
        dynAvprType_logSetup(stdLogA, nullptr, 1);
    }
    ~DynAvprTypeTests() override {
    }

};

TEST_F(DynAvprTypeTests, SimpleTest) {
    runTestA(theTestCase, "test.dt.uint", DYN_TYPE_SIMPLE);
}

TEST_F(DynAvprTypeTests, SimpleSimpleTest) {
    runTestA(theTestCase, "test.dt.RecordWithSimpleTypes", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, ComplexTest) {
    runTestA(theTestCase, "blah.test.dt.A", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, EnumTest2) {
    runTestA(theTestCase, "test.dt.EnumWithValue", DYN_TYPE_SIMPLE);
}

TEST_F(DynAvprTypeTests, EnumTest) {
    runTestA(theTestCase, "test.dt.EnumWithoutValue", DYN_TYPE_SIMPLE);
}

TEST_F(DynAvprTypeTests, ArrayTest) {
    runTestA(theTestCase, "test.dt.V", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, NestedArrayTest) {
    runTestA(nestedArray, "test.dt.NA", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, ComplexComplexTest) {
    runTestA(theTestCase, "test.dt.B", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, ReferenceTest) {
    runTestA(referenceTestCase, "test.dt.R", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, AliasTest) {
    runTestA(theTestCase, "test.dt.Alias", DYN_TYPE_SIMPLE);
}

TEST_F(DynAvprTypeTests, ComplexAliasTest) {
    runTestA(aliasTestCase, "common.dt.Polar_2d", DYN_TYPE_COMPLEX);
}

TEST_F(DynAvprTypeTests, ArrayWithSimpleTest) { //TODO: fix this testcase
    runTestA(arrayIntTestCase, "common.dt.SimpleArray", DYN_TYPE_COMPLEX);
}

/*********************************************************************************
 * Assignment tests + version testing
 */

void test_version(dyn_type* type, const std::string& version_string) {
    struct meta_entry *entry = nullptr;
    struct meta_properties_head *entries = nullptr;
    ASSERT_EQ(0, dynType_metaEntries(type, &entries));
    ASSERT_FALSE(TAILQ_EMPTY(entries));

    const std::string entry_value {"version"};
    bool compared = false;
    TAILQ_FOREACH(entry, entries, entries) {
        if (entry_value == entry->name) {
            ASSERT_STREQ(version_string.c_str(), entry->value);
            compared = true;
        }
    }
    ASSERT_TRUE(compared); //"Expect a comparison, otherwise no version meta info is available");
}

class DynAvprAssignTests : public ::testing::Test {
public:
    DynAvprAssignTests() {
        dynAvprType_logSetup(stdLogA, nullptr, 1);
    }
    ~DynAvprAssignTests() override {
    }

};

TEST_F(DynAvprAssignTests, AssignSimpleTest) {
    // Build type
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.uint");
    ASSERT_TRUE(type != nullptr);

    // set value
    uint32_t simple = 0;
    uint32_t nv = 42;
    dynType_simple_setValue(type, &simple, &nv);

    ASSERT_EQ(42, simple);
    test_version(type, "2.1.9");
    dynType_destroy(type);
}

TEST_F(DynAvprAssignTests, AssignComplexTest) {
    // Build type
    struct exA {
        struct {
            uint32_t aa;
            uint32_t bb;
            double cc;
        } x;
        uint64_t y;
    };


    dyn_type *type = dynType_parseAvprWithStr(theTestCase, "blah.test.dt.A");
    ASSERT_TRUE(type != nullptr);

    // set example values
    uint32_t a_x_aa = 52;
    uint32_t a_x_bb = 42;
    double a_x_cc = 31.13;
    uint64_t a_y = 1001001;

    // Simple element in complex
    exA inst {{0, 0, 0.0}, 0};
    dynType_complex_setValueAt(type, 1, &inst, &a_y);
    ASSERT_EQ(1001001, inst.y);

    // Complex element in complex type, first acquire subtype, then check similar to simple type
    void *loc = nullptr;
    dyn_type *subType = nullptr;
    dynType_complex_valLocAt(type, 0, static_cast<void*>(&inst), &loc);
    dynType_complex_dynTypeAt(type, 0, &subType);

    dynType_complex_setValueAt(subType, 0, &inst.x, &a_x_aa);
    ASSERT_EQ(52, inst.x.aa);

    dynType_complex_setValueAt(subType, 1, &inst.x, &a_x_bb);
    ASSERT_EQ(42, inst.x.bb);
    dynType_complex_setValueAt(subType, 2, &inst.x, &a_x_cc);
    ASSERT_EQ(31.13, inst.x.cc);

    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST_F(DynAvprAssignTests, AssignComplexSimpleTest) {
    // Build type
    struct exB {
        int32_t b;
    };

    dyn_type *type = dynType_parseAvprWithStr(theTestCase, "test.dt.RecordWithSimpleTypes");
    ASSERT_TRUE(type != nullptr);

    // set example values
    int32_t b_b = 5301;

    exB inst {0};
    dynType_complex_setValueAt(type, 0, &inst, &b_b);
    ASSERT_EQ(5301, inst.b);

    test_version(type, "1.1.9");
    dynType_destroy(type);
}


TEST_F(DynAvprAssignTests, AssignPtrTest) {
    // Build type

    struct exNode {
        double data;
        exNode *ne_elem_left;
        exNode *ne_elem_right;
    };

    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "N.Node");
    ASSERT_TRUE(type != nullptr);

    // right tree
    exNode rightrightright {2.0, nullptr, nullptr};
    exNode rightleft {0.5, nullptr, nullptr};
    exNode rightright {1.5, nullptr, &rightrightright};
    exNode right {1.0, &rightleft, &rightright};

    // left tree
    exNode leftleft {-1.5, nullptr, nullptr};
    exNode leftright {-0.5, nullptr, nullptr};
    exNode left {-1.0, &leftleft, &leftright};

    // Base
    exNode base {0.0, &left, &right};

    double nv = 101.101;
    dynType_complex_setValueAt(type, 0, &base, &nv);
    ASSERT_EQ(101.101, base.data);

    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST_F(DynAvprAssignTests, AssignEnumTest) {
    // Build type
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.EnumWithValue");
    dyn_type* type_2 = dynType_parseAvprWithStr(theTestCase, "test.dt.EnumWithoutValue");
    ASSERT_TRUE(type != nullptr);
    ASSERT_TRUE(type_2 != nullptr);

    // Print type
    dynType_print(type, stdout);
    dynType_print(type_2, stdout);

    // set value
    enum exEnumV {A=3, B=1234};
    enum exEnumV mEnum = B;
    enum exEnumV nv = A;

    dynType_simple_setValue(type, &mEnum, &nv);

    ASSERT_EQ(A, mEnum);
    test_version(type, "1.1.9");
    dynType_destroy(type);
    dynType_destroy(type_2);
}

TEST_F(DynAvprAssignTests, AssignArrayTest) {
    // Build type
    struct exV_sequence {
        uint32_t cap;
        uint32_t len;
        uint32_t **buf;
    };

    struct exV {
        struct exV_sequence elem;
    };

    exV inst {exV_sequence{0,0, nullptr}};
    exV_sequence *s = &inst.elem;

    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.V");
    ASSERT_TRUE(type != nullptr);

    // set value
    void *loc = nullptr;
    dyn_type *subType = nullptr;
    dynType_complex_valLocAt(type, 0, static_cast<void*>(&inst), &loc);
    dynType_complex_dynTypeAt(type, 0, &subType);
    int res = dynType_alloc(subType, reinterpret_cast<void**>(&s));
    ASSERT_EQ(0, res);
    ASSERT_TRUE(s != nullptr);

    dynType_free(type, s);
    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST_F(DynAvprAssignTests, AnnotationTest) {
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.Anne");
    ASSERT_TRUE(type != nullptr);

    // get value for meta entry
    struct meta_entry *entry = nullptr;
    struct meta_properties_head *entries = nullptr;
    ASSERT_EQ(0, dynType_metaEntries(type, &entries));
    ASSERT_FALSE(TAILQ_EMPTY(entries));

    const std::string msg_id_entry {"MsgId"};
    bool compared = false;

    TAILQ_FOREACH(entry, entries, entries) {
        printf("Got an entry: %s\n", entry->name);
        if (msg_id_entry == entry->name) {
            ASSERT_STREQ("1000", entry->value);
            compared = true;
        }
    }
    ASSERT_TRUE(compared); //"Expect a comparison, otherwise no msg id entry available");

    dynType_destroy(type);
}

/*********************************************************************************
 * Invalid tests
 */

class DynAvprInvalidTests : public ::testing::Test {
public:
    DynAvprInvalidTests() {
        dynAvprType_logSetup(stdLogA, nullptr, 1);
    }
    ~DynAvprInvalidTests() override {
    }

};

TEST_F(DynAvprInvalidTests, InvalidJson) {
    dyn_type* type = dynType_parseAvprWithStr("{", "test.invalid.type"); // Json error
    ASSERT_TRUE(type == nullptr);
}

TEST_F(DynAvprInvalidTests, InvalidJsonObject) {
    dyn_type* type = dynType_parseAvprWithStr("[]", "test.invalid.type"); // Root should be object not list
    ASSERT_TRUE(type == nullptr);
    type = dynType_parseAvprWithStr("{}", "test.invalid.type"); // Root should have a namespace
    ASSERT_TRUE(type == nullptr);
    type = dynType_parseAvprWithStr(R"({"namespace":"nested"})", "test.invalid.type"); // Root should have types array
    ASSERT_TRUE(type == nullptr);
    type = dynType_parseAvprWithStr(
            R"({"namespace":"nested", "types" : [] })"
            , "test.invalid.type"); // types is empty, so not found
    ASSERT_TRUE(type == nullptr);
    type = dynType_parseAvprWithStr(
            "{\"namespace\":\"nested\", \"types\" : [\
            { \"type\" : \"record\", \"name\" : \"IncompleteStruct\", \"fields\" : [\
            { \"name\" : \"Field1\", \"type\" : \"NonExisting\" } ] } \
            ] }"
            , "nested.IncompleteStruct"); // struct misses definition
    ASSERT_TRUE(type == nullptr);
    type = dynType_parseAvprWithStr(
            "{\"namespace\":\"nested\", \"types\" : [\
            { \"type\" : \"record\", \"name\" : \"IncompleteStruct\", \"fields\" : [\
            { \"name\" : \"Field1\" } ] } \
            ] }"
            , "nested.IncompleteStruct"); // struct entry misses type
    ASSERT_TRUE(type == nullptr);
    // None of the above testcases should crash the parser
}