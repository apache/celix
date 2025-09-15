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
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"

extern "C" {
    #include "dyn_common.h"
    #include "dyn_type.h"
    #include "celix_err.h"

    static void runTest(const char *descriptorStr, const char *exName) {
        dyn_type *type;
        type = NULL;
        //printf("\n-- example %s with descriptor string '%s' --\n", exName, descriptorStr);
        int status = dynType_parseWithStr(descriptorStr, exName, NULL, &type);
        ASSERT_EQ(0, status);
        ASSERT_STREQ(exName, dynType_getName(type));

        //MEM check, to try to ensure no mem leaks/corruptions occur.
        int i;
        int j;
        int nrOfBurst = 10;
        void *pointers[50];
        int burst = sizeof(pointers) / sizeof(void *);
        for (j = 0; j < nrOfBurst; j += 1) {
            for (i = 0; i < burst ; i +=1 ) {
                pointers[i] = NULL;
                dynType_alloc(type, &pointers[i]);
            }
            for (i = 0; i < burst ; i +=1 ) {
                dynType_free(type, pointers[i]);
            }
        }

        FILE *stream = fopen("/dev/null", "w");
        dynType_print(type, stream);
        fclose(stream);
        dynType_destroy(type);
        //printf("--\n\n");
    }
}

class DynTypeTests : public ::testing::Test {
public:
    DynTypeTests() {
    }
    ~DynTypeTests() override {
        celix_err_resetErrors();
    }

};

#define EX1 "{BbJjIiSsDFNN arg1 arg2 arg3 arg4 arg5 arg6 arg7 arg8 arg9 arg10 arg11 arg12}"
#define EX2 "{D{DD b_1 b_2}I a b c}"
#define EX3 "Tsub={DD b_1 b_2};{DLsub;I a b c}"
#define EX4 "{[I numbers}"
#define EX5 "[[{DD{iii val3_1 val3_2 val3_3} val1 val2 val3}"
#define EX6 "Tsample={DD vala valb};Lsample;"
#define EX7 "Tsample={DD vala valb};[Lsample;"
#define EX8 "[Tsample={DD a b};Lsample;"
#define EX9 "*D"
#define EX10 "Tsample={DD a b};******Lsample;"
#define EX11 "Tsample=D;Lsample;"
#define EX12 "Tnode={Lnode;Lnode; left right};{Lnode; head}" //note recursive example
#define EX13 "Ttype={DDDDD a b c d e};{ltype;Ltype;ltype;Ltype; byVal1 byRef1 byVal2 ByRef2}" 
#define EX14 "{DD{FF{JJ}{II*{ss}}}}"  //unnamed fields
#define EX15 "Tsample={jDD time val1 val2};Tresult={jDlsample; time result sample};Lresult;"
#define EX16 "Tpoi={BDD id lat lon};Lpoi;"
#define EX17 "{#v1=0;#v2=1;E#v1=9;#v2=10;E enum1 enum2}"
#define EX18 "Ttext=t;ltext;"
#define EX19 "Tsample={DD vala valb};Tref=lsample;;lref;"
#define EX20 "TINTEGER=I;Tsample={DlINTEGER; vala valb};Tref=lsample;;lref;"
#define EX21 "Tprops=p;lprops;"
#define EX22 "Tarraylist=a;larraylist;"

#define CREATE_EXAMPLES_TEST(DESC) \
    TEST_F(DynTypeTests, ParseTestExample ## DESC) { \
        runTest(DESC, #DESC); \
    } \
    TEST_F(DynTypeTests, ParseTestExampleNoName ## DESC) { \
            runTest(DESC, nullptr); \
    }

CREATE_EXAMPLES_TEST(EX1)
CREATE_EXAMPLES_TEST(EX2)
CREATE_EXAMPLES_TEST(EX3)
CREATE_EXAMPLES_TEST(EX4)
CREATE_EXAMPLES_TEST(EX5)
CREATE_EXAMPLES_TEST(EX6)
CREATE_EXAMPLES_TEST(EX7)
CREATE_EXAMPLES_TEST(EX8)
CREATE_EXAMPLES_TEST(EX9)
CREATE_EXAMPLES_TEST(EX10)
CREATE_EXAMPLES_TEST(EX11)
CREATE_EXAMPLES_TEST(EX12)
CREATE_EXAMPLES_TEST(EX13)
CREATE_EXAMPLES_TEST(EX14)
CREATE_EXAMPLES_TEST(EX15)
CREATE_EXAMPLES_TEST(EX16)
CREATE_EXAMPLES_TEST(EX17)
CREATE_EXAMPLES_TEST(EX18)
CREATE_EXAMPLES_TEST(EX19)
CREATE_EXAMPLES_TEST(EX20)
CREATE_EXAMPLES_TEST(EX21)
CREATE_EXAMPLES_TEST(EX22)

TEST_F(DynTypeTests, ParseRandomGarbageTest) {
    /*
    unsigned int seed = 4148;
    char *testRandom = getenv("DYN_TYPE_TEST_F_RANDOM");
    if (testRandom != NULL && strcmp("true", testRandom) == 0) {
        seed = (unsigned int) time(NULL);
    } 
    srandom(seed);
    size_t nrOfTests = 100;

    printf("\nStarting test with random seed %i and nrOfTests %zu.\n", seed, nrOfTests);

    int i;
    int k;
    int c;
    int sucesses = 0;
    char descriptorStr[32];
    descriptorStr[31] = '\0';
    for(i = 0; i < nrOfTests; i += 1) {  
        for(k = 0; k < 31; k += 1) {
            do {
                c = (char) (((random() * 128) / RAND_MAX) - 1);
            } while (!isprint(c));
            descriptorStr[k] = c;
            if (c == '\0') { 
                break;
            }
        }

        //printf("ParseRandomGarbageTest iteration %i with descriptor string '%s'\n", k, descriptorStr); 
        dyn_type *type = NULL;	
        int status = dynType_parseWithStr(descriptorStr, NULL, NULL, &type);
        if (status == 0) {
            dynType_destroy(type);
        }
    }
     */
}

TEST_F(DynTypeTests, AssignTest1) {
    struct ex1 {
        int32_t a;
        int32_t b;
        int32_t c;
    };
    struct ex1 inst;
    const char *desc = "{III a b c}";
    dyn_type *type = NULL;
    int status = dynType_parseWithStr(desc, NULL, NULL, &type);
    ASSERT_EQ(0, status);
    int32_t val1 = 2;
    int32_t val2 = 4;
    int32_t val3 = 8;
    EXPECT_EQ(0, dynType_complex_indexForName(type, "a"));
    dynType_complex_setValueAt(type, 0,  &inst, &val1);
    ASSERT_EQ(2, inst.a);

    EXPECT_EQ(1, dynType_complex_indexForName(type, "b"));
    dynType_complex_setValueAt(type, 1,  &inst, &val2);
    ASSERT_EQ(4, inst.b);

    EXPECT_EQ(2, dynType_complex_indexForName(type, "c"));
    dynType_complex_setValueAt(type, 2,  &inst, &val3);
    ASSERT_EQ(8, inst.c);

    EXPECT_EQ(-1, dynType_complex_indexForName(type, nullptr));
    EXPECT_EQ(-1, dynType_complex_indexForName(type, "none"));
    dynType_destroy(type);
}

TEST_F(DynTypeTests, AssignTest2) {
    struct ex {
        int32_t a;
        struct {
            double a;
            double b;
        } b;
    };
    struct ex inst;
    const char *desc = "{I{DD a b} a b}";
    dyn_type *type = NULL;
    int status = dynType_parseWithStr(desc, NULL, NULL,  &type);
    ASSERT_EQ(0, status);
    int32_t a = 2;
    double b_a = 1.1;
    double b_b = 1.2;

    dynType_complex_setValueAt(type, 0,  &inst, &a);
    ASSERT_EQ(2, inst.a);

    (void)dynType_complex_valLocAt(type, 1, (void *)&inst);
    const dyn_type* subType = dynType_complex_dynTypeAt(type, 1);

    dynType_complex_setValueAt(subType, 0, &inst.b, &b_a);
    ASSERT_EQ(1.1, inst.b.a);

    dynType_complex_setValueAt(subType, 1, &inst.b, &b_b);
    ASSERT_EQ(1.2, inst.b.b);

    dynType_destroy(type);
}

TEST_F(DynTypeTests, AssignTest3) {
    int simple = 1;
    dyn_type *type = NULL;
    int rc = dynType_parseWithStr("N", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    int newValue = 42;
    void *loc = &simple;
    void *input = &newValue;
    dynType_simple_setValue(type, loc, input);
    ASSERT_EQ(42, simple);
    dynType_destroy(type);
}

TEST_F(DynTypeTests, MetaInfoTest) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("#a=t;{DD#longname=longvalue;D a b c}", NULL, NULL, &type);
    //rc = dynType_parseWithStr("{DDD a b c}", NULL, NULL, &type);

    ASSERT_EQ(0, rc);

    auto entries = dynType_metaEntries(type);
    struct meta_entry* entry = NULL;
    size_t nbEntries = 0;
    TAILQ_FOREACH(entry, entries, entries) {
        nbEntries++;
        ASSERT_STREQ("a", entry->name);
        ASSERT_STREQ("t", entry->value);
    }
    ASSERT_EQ(1, nbEntries);


    const char *val = NULL;
    val = dynType_getMetaInfo(type, "a");
    ASSERT_TRUE(val != NULL);
    ASSERT_TRUE(strcmp("t", val) == 0);

    val = dynType_getMetaInfo(type, "longname");
    ASSERT_TRUE(val == NULL);

    val = dynType_getMetaInfo(type, "nonexisting");
    ASSERT_TRUE(val == NULL);

    dynType_destroy(type);
}

TEST_F(DynTypeTests, SequenceWithPointerTest) {
    struct val {
        double a;
        double b;
    };

    struct item {
        int64_t a;
        const char *text;
        struct val val;
        double c;
        double d;
        int64_t e;
    };

    struct item_sequence {
        uint32_t cap;
        uint32_t len;
        struct item **buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("Tval={DD a b};Titem={Jtlval;DDJ a text val c d e};[Litem;", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    struct item_sequence *seq = NULL;
    rc = dynType_alloc(type, (void **)&seq);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(seq != NULL);

    dynType_sequence_init(type, seq);
    rc = dynType_sequence_alloc(type, seq, 1);
    ASSERT_EQ(0, rc);
    struct item **loc = NULL;
    rc = dynType_sequence_increaseLengthAndReturnLastLoc(type, seq, (void **)&loc);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(loc, &seq->buf[0]);
    ASSERT_EQ(sizeof(struct item), dynType_size(dynType_typedPointer_getTypedType(dynType_sequence_itemType(type))));
    rc = dynType_alloc(dynType_typedPointer_getTypedType(dynType_sequence_itemType(type)), (void**)loc);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(seq->buf[0], *loc);
    dynType_free(type, seq);

    rc = dynType_sequence_alloc(type, nullptr, 1);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error null sequence", celix_err_popLastError());

    dynType_destroy(type);
}


TEST_F(DynTypeTests, SequenceReserve) {

    struct val {
        double a;
        double b;
    };

    struct item {
        int64_t a;
        const char *text;
        struct val val;
        double c;
        double d;
        int64_t e;
    };

    struct item_sequence {
        uint32_t cap;
        uint32_t len;
        struct item **buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("Tval={DD a b};Titem={Jtlval;DDJ a text val c d e};[Litem;", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    struct item_sequence *seq = NULL;

    // sequence buffer should be zeroed
    rc = dynType_alloc(type, (void **)&seq);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(seq != NULL);
    rc = dynType_sequence_reserve(type, seq, 2);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(nullptr, seq->buf[0]);
    ASSERT_EQ(nullptr, seq->buf[1]);
    ASSERT_EQ(2, seq->cap);
    ASSERT_EQ(0, seq->len);

    // no need to expand capacity
    rc = dynType_sequence_reserve(type, seq, 1);
    ASSERT_EQ(0, rc);
    dynType_free(type, seq);

    // try to reverse for null seq
    rc = dynType_sequence_reserve(type, nullptr, 2);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error null sequence", celix_err_popLastError());

    dynType_destroy(type);
}

TEST_F(DynTypeTests, FillSequenceTest) {
    struct double_sequence {
        uint32_t cap;
        uint32_t len;
        double* buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("[D", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    struct double_sequence *seq = NULL;
    rc = dynType_alloc(type, (void **)&seq);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(seq != NULL);

    rc = dynType_sequence_alloc(type, seq, 1);
    ASSERT_EQ(0, rc);

    void* loc;
    rc = dynType_sequence_increaseLengthAndReturnLastLoc(type, seq, (void **)&loc);
    ASSERT_EQ(0, rc);

    double val = 2.0;
    dynType_simple_setValue(dynType_sequence_itemType(type), loc, &val);
    ASSERT_EQ(val, seq->buf[0]);

    rc = dynType_sequence_increaseLengthAndReturnLastLoc(type, seq, (void **)&loc);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Cannot increase sequence length beyond capacity (1)", celix_err_popLastError());

    rc = dynType_sequence_reserve(type, seq, 2);
    ASSERT_EQ(0, rc);
    rc = dynType_sequence_locForIndex(type, seq, 1, (void **)&loc);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Requesting index (1) outsize defined length (1) but within capacity", celix_err_popLastError());


    rc = dynType_sequence_locForIndex(type, seq, 2, (void **)&loc);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Requested index (2) is greater than capacity (2) of sequence", celix_err_popLastError());

    dynType_free(type, seq);
    dynType_destroy(type);
}

TEST_F(DynTypeTests, EnumTest) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("{#v1=0;#v2=1;E#v1=9;#v2=10;E enum1 enum2}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);

    dynType_print(type, stdout);
    dynType_destroy(type);
}


TEST_F(DynTypeTests, PrintNullTypeTest) {
    celix_autofree char* buf = nullptr;
    size_t bufSize = 0;
    celix_autoptr(FILE) result = open_memstream(&buf, &bufSize);
    dynType_print(nullptr, result);
    fflush(result);
    ASSERT_STREQ("invalid type\n", buf);
}

TEST_F(DynTypeTests, TextTest) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("t", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(DYN_TYPE_TEXT, dynType_type(type));

    char** val = nullptr;
    rc = dynType_alloc(type, (void**)&val);
    ASSERT_EQ(0, rc);
    rc = dynType_text_allocAndInit(type, val, "test");
    ASSERT_EQ(0, rc);
    ASSERT_STREQ("test", *val);
    dynType_free(type, val);
    dynType_destroy(type);
}

TEST_F(DynTypeTests, NrOfEntriesTest) {
    dyn_type *type = NULL;
    int rc = dynType_parseWithStr("{DD}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(2, dynType_complex_nrOfEntries(type));
    dynType_destroy(type);

    type = NULL;
    rc = dynType_parseWithStr("{DDJJ}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_EQ(4, dynType_complex_nrOfEntries(type));
    dynType_destroy(type);
}

TEST_F(DynTypeTests, ComplexHasEmptyName) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"({II a })", nullptr, nullptr, &type);
    ASSERT_EQ(1, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, ComplexTypeMissingClosingBrace) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"({II a b)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error parsing complex type, expected '}'", celix_err_popLastError());
}

TEST_F(DynTypeTests, ComplexTypeWithMoreNamesThanFields) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"({II a b c})", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Error parsing complex type, expected '}'", celix_err_popLastError());
}

TEST_F(DynTypeTests, SchemaEndsWithoutNullTerminator) {
    dyn_type *type = NULL;
    //ends with '-'
    auto rc = dynType_parseWithStr(R"({II a b}-)", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, MetaInfoMissingName) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"(#=1;)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Failed to parse meta properties", celix_err_popLastError());
    ASSERT_STREQ("Parsed empty name", celix_err_popLastError());
}

TEST_F(DynTypeTests, MetaInfoMissingEquality) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"(#testMetaInfo 1;)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, MetaInfoMissingSemicolon) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"(#testMetaInfo=1 )", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, MetaInfoMissingValue) {
    dyn_type *type = NULL;
    auto rc = dynType_parseWithStr(R"(#testMetaInfo=;)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, ParseNestedTypeFailed) {
    dyn_type *type = NULL;
    int rc;
    //missing name
    rc = dynType_parseWithStr(R"(T={DD a b};)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Parsed empty name", celix_err_popLastError());

    //missing '='
    rc = dynType_parseWithStr(R"(Ttype)", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);

    //missing value
    rc = dynType_parseWithStr(R"(Ttype=)", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);

    //missing ';'
    rc = dynType_parseWithStr(R"(Ttype={DD a b})", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, ParseReferenceFailed) {
    dyn_type *type = NULL;
    int rc;
    // missing name
    rc = dynType_parseWithStr(R"(Ttype={DD a b};l;)", nullptr, nullptr, &type);
    ASSERT_NE(0, rc);
    ASSERT_STREQ("Parsed empty name", celix_err_popLastError());
    //missing ';'
    rc = dynType_parseWithStr(R"(Ttype={DD a b};ltype)", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
    //missing ';'
    rc = dynType_parseWithStr(R"(Ttype={DD a b};Ltype)", nullptr, nullptr, &type);
    ASSERT_EQ(3, rc);
    celix_err_printErrors(stderr, nullptr, nullptr);
}

TEST_F(DynTypeTests, ParseSequenceFailed) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("Tval={DD a b};Titem={Jtlval;DDJ a text val c d e};**[Lite;", NULL, NULL, &type);
    ASSERT_NE(0, rc);
}

TEST_F(DynTypeTests, TrivialityTesT) {
    dyn_type *type = NULL;
    int rc = 0;

    // non pointer simple type is trivial
    rc = dynType_parseWithStr("I", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(dynType_isTrivial(type));
    dynType_destroy(type);

    // untyped pointer is non-trivial
    rc = dynType_parseWithStr("P", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // reference to non-pointer simple type is trivial
    rc = dynType_parseWithStr("Ttype=I;ltype;", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_TRUE(dynType_isTrivial(type));
    dynType_destroy(type);

    // typed pointer is non-trivial
    rc = dynType_parseWithStr("*D", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    ASSERT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // sequence type is non-trivial
    rc = dynType_parseWithStr("[I", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // text is non-trivial
    rc = dynType_parseWithStr("t", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // a complex consisting of non-pointer scalars is trivial
    rc = dynType_parseWithStr("{II a b}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_TRUE(dynType_isTrivial(type));
    dynType_destroy(type);

    // a complex having a pointer scalar is non-trivial
    rc = dynType_parseWithStr("{IP a b}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // a complex consisting of non-pointer scalar and trivial complex is trivial
    rc = dynType_parseWithStr("{II{II b c}}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_TRUE(dynType_isTrivial(type));
    dynType_destroy(type);

    // a complex consisting of non-pointer scalar and non-trivial complex is non-trivial
    rc = dynType_parseWithStr("{II{IP b c}}", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // celix_properties_t* is non-trivial
    rc = dynType_parseWithStr("p", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);

    // celix_array_list_t* is non-trivial
    rc = dynType_parseWithStr("a", NULL, NULL, &type);
    ASSERT_EQ(0, rc);
    EXPECT_FALSE(dynType_isTrivial(type));
    dynType_destroy(type);
}