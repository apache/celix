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
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {
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
            CHECK_EQUAL(expectedType, dynType_type(type));
            dynType_destroy(type);
        } else {
            CHECK_EQUAL(1, 0);
        }
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
                    },\
                    { \
                        \"name\" : \"bb\", \
                        \"type\" : \"uint\" \
                    }\
                    ,\
                    { \
                        \"name\" : \"cc\", \
                        \"type\" : \"double\" \
                    }\
                    ] \
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


/*********************************************************************************
 * Type building tests
 */

TEST_GROUP(DynAvprTypeTests) {
	void setup() override {
	    dynAvprType_logSetup(stdLogA, nullptr, 3);
	}
};

TEST(DynAvprTypeTests, SimpleTest) {
    runTestA(theTestCase, "test.dt.uint", DYN_TYPE_SIMPLE);
}

TEST(DynAvprTypeTests, ComplexTest) {
    runTestA(theTestCase, "blah.test.dt.A", DYN_TYPE_COMPLEX);
}

TEST(DynAvprTypeTests, EnumTest2) {
    runTestA(theTestCase, "test.dt.EnumWithValue", DYN_TYPE_SIMPLE);
}

TEST(DynAvprTypeTests, EnumTest) {
    runTestA(theTestCase, "test.dt.EnumWithoutValue", DYN_TYPE_SIMPLE);
}

TEST(DynAvprTypeTests, ArrayTest) {
    runTestA(theTestCase, "test.dt.V", DYN_TYPE_COMPLEX);
}

TEST(DynAvprTypeTests, NestedArrayTest) {
    runTestA(nestedArray, "test.dt.NA", DYN_TYPE_COMPLEX);
}

TEST(DynAvprTypeTests, ComplexComplexTest) {
    runTestA(theTestCase, "test.dt.B", DYN_TYPE_COMPLEX);
}

TEST(DynAvprTypeTests, ReferenceTest) {
    runTestA(referenceTestCase, "test.dt.R", DYN_TYPE_COMPLEX);
}

/*********************************************************************************
 * Assignment tests + version testing
 */

void test_version(dyn_type* type, const std::string& version_string) {
    struct meta_entry *entry = nullptr;
    struct meta_properties_head *entries = nullptr;
    CHECK_EQUAL(0, dynType_metaEntries(type, &entries));
    CHECK_FALSE(TAILQ_EMPTY(entries));

    const std::string entry_value {"version"};
    bool compared = false;
    TAILQ_FOREACH(entry, entries, entries) {
        if (entry_value == entry->name) {
            STRCMP_EQUAL(version_string.c_str(), entry->value);
            compared = true;
        }
    }
    CHECK_TRUE_TEXT(compared, "Expect a comparison, otherwise no version meta info is available");
}

TEST_GROUP(DynAvprAssignTests) {
	void setup() override {
	    dynAvprType_logSetup(stdLogA, nullptr, 1);
	}
};

TEST(DynAvprAssignTests, AssignSimpleTest) {
    // Build type
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.uint");
    CHECK(type != nullptr);

    // set value
    uint32_t simple = 0;
    uint32_t nv = 42;
    dynType_simple_setValue(type, &simple, &nv);

    CHECK_EQUAL(42, simple);
    test_version(type, "2.1.9");
    dynType_destroy(type);
}

TEST(DynAvprAssignTests, AssignComplexTest) {
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
    CHECK(type != nullptr);

    // set example values
    uint32_t a_x_aa = 52;
    uint32_t a_x_bb = 42;
    double a_x_cc = 31.13;
    uint64_t a_y = 1001001;

    // Simple element in complex
    exA inst {{0, 0, 0.0}, 0};
    dynType_complex_setValueAt(type, 1, &inst, &a_y);
    CHECK_EQUAL(1001001, inst.y);

    // Complex element in complex type, first acquire subtype, then check similar to simple type
    void *loc = nullptr;
    dyn_type *subType = nullptr;
    dynType_complex_valLocAt(type, 0, static_cast<void*>(&inst), &loc);
    dynType_complex_dynTypeAt(type, 0, &subType);

    dynType_complex_setValueAt(subType, 0, &inst.x, &a_x_aa);
    CHECK_EQUAL(52, inst.x.aa);

    dynType_complex_setValueAt(subType, 1, &inst.x, &a_x_bb);
    CHECK_EQUAL(42, inst.x.bb);
    dynType_complex_setValueAt(subType, 2, &inst.x, &a_x_cc);
    CHECK_EQUAL(31.13, inst.x.cc);

    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST(DynAvprAssignTests, AssignPtrTest) {
    // Build type

    struct exNode {
        double data;
        exNode *ne_elem_left;
        exNode *ne_elem_right;
    };

    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "N.Node");
    CHECK(type != nullptr);

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
    CHECK_EQUAL(101.101, base.data);

    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST(DynAvprAssignTests, AssignEnumTest) {
    // Build type
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.EnumWithValue");
    dyn_type* type_2 = dynType_parseAvprWithStr(theTestCase, "test.dt.EnumWithoutValue");
    CHECK(type != nullptr);
    CHECK(type_2 != nullptr);

    // Print type
    dynType_print(type, stdout);
    dynType_print(type_2, stdout);

    // set value
    enum exEnumV {A=3, B=1234};
    enum exEnumV mEnum = B;
    enum exEnumV nv = A;

    dynType_simple_setValue(type, &mEnum, &nv);

    CHECK_EQUAL(A, mEnum);
    test_version(type, "1.1.9");
    dynType_destroy(type);
    dynType_destroy(type_2);
}

TEST(DynAvprAssignTests, AssignArrayTest) {
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
    CHECK(type != nullptr);

    // set value
    void *loc = nullptr;
    dyn_type *subType = nullptr;
    dynType_complex_valLocAt(type, 0, static_cast<void*>(&inst), &loc);
    dynType_complex_dynTypeAt(type, 0, &subType);
    int res = dynType_alloc(subType, reinterpret_cast<void**>(&s));
    CHECK_EQUAL(0, res);
    CHECK(s != nullptr);

    dynType_free(type, s);
    test_version(type, "1.1.9");
    dynType_destroy(type);
}

TEST(DynAvprAssignTests, AnnotationTest) {
    dyn_type* type = dynType_parseAvprWithStr(theTestCase, "test.dt.Anne");
    CHECK(type != nullptr);

    // get value for meta entry
    struct meta_entry *entry = nullptr;
    struct meta_properties_head *entries = nullptr;
    CHECK_EQUAL(0, dynType_metaEntries(type, &entries));
    CHECK_FALSE(TAILQ_EMPTY(entries));

    const std::string msg_id_entry {"MsgId"};
    bool compared = false;

    TAILQ_FOREACH(entry, entries, entries) {
        printf("Got an entry: %s\n", entry->name);
        if (msg_id_entry == entry->name) {
            STRCMP_EQUAL("1000", entry->value);
            compared = true;
        }
    }
    CHECK_TRUE_TEXT(compared, "Expect a comparison, otherwise no msg id entry available");

    dynType_destroy(type);
}

/*********************************************************************************
 * Invalid tests
 */
TEST_GROUP(DynAvprInvalidTests) {
	void setup() override {
	    dynAvprType_logSetup(stdLogA, nullptr, 1);
	}
};

TEST(DynAvprInvalidTests, InvalidJson) {
    dyn_type* type = dynType_parseAvprWithStr("{", "test.invalid.type"); // Json error
    CHECK(type == nullptr);
}

TEST(DynAvprInvalidTests, InvalidJsonObject) {
    dyn_type* type = dynType_parseAvprWithStr("[]", "test.invalid.type"); // Root should be object not list
    CHECK(type == nullptr);
    type = dynType_parseAvprWithStr("{}", "test.invalid.type"); // Root should have a namespace
    CHECK(type == nullptr);
    type = dynType_parseAvprWithStr(R"({"namespace":"nested"})", "test.invalid.type"); // Root should have types array
    CHECK(type == nullptr);
    type = dynType_parseAvprWithStr(
            R"({"namespace":"nested", "types" : [] })"
            , "test.invalid.type"); // types is empty, so not found
    CHECK(type == nullptr);
    type = dynType_parseAvprWithStr(
            "{\"namespace\":\"nested\", \"types\" : [\
            { \"type\" : \"record\", \"name\" : \"IncompleteStruct\", \"fields\" : [\
            { \"name\" : \"Field1\", \"type\" : \"NonExisting\" } ] } \
            ] }"
            , "nested.IncompleteStruct"); // struct misses definition
    CHECK(type == nullptr);
    type = dynType_parseAvprWithStr(
            "{\"namespace\":\"nested\", \"types\" : [\
            { \"type\" : \"record\", \"name\" : \"IncompleteStruct\", \"fields\" : [\
            { \"name\" : \"Field1\" } ] } \
            ] }"
            , "nested.IncompleteStruct"); // struct entry misses type
    CHECK(type == nullptr);
    // None of the above testcases should crash the parser
}

