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
    #include <stdarg.h>
    
    #include "dyn_common.h"
    #include "dyn_type.h"

	static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
	    va_list ap;
	    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
	    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
	    va_start(ap, msg);
	    vfprintf(stderr, msg, ap);
	    fprintf(stderr, "\n");
	    va_end(ap);
	}

    static void runTest(const char *descriptorStr, const char *exName) {
        dyn_type *type;
        type = NULL;
        //printf("\n-- example %s with descriptor string '%s' --\n", exName, descriptorStr);
        int status = dynType_parseWithStr(descriptorStr, exName, NULL, &type);
        CHECK_EQUAL(0, status);

        //MEM check, to try to ensure no mem leaks/corruptions occur.
        int i;
        int j;
        int nrOfBurst = 10;
        int burst = 50;
        void *pointers[burst];
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

TEST_GROUP(DynTypeTests) {
	void setup() {
	    dynType_logSetup(stdLog, NULL, 1);
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

#define CREATE_EXAMPLES_TEST(DESC) \
    TEST(DynTypeTests, ParseTestExample ## DESC) { \
        runTest(DESC, #DESC); \
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

TEST(DynTypeTests, ParseRandomGarbageTest) {
    /*
    unsigned int seed = 4148;
    char *testRandom = getenv("DYN_TYPE_TEST_RANDOM");
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

TEST(DynTypeTests, AssignTest1) {
    struct ex1 {
        int32_t a;
        int32_t b;
        int32_t c;
    };
    struct ex1 inst;
    const char *desc = "{III a b c}";
    dyn_type *type = NULL;
    int status = dynType_parseWithStr(desc, NULL, NULL, &type);
    CHECK_EQUAL(0, status);
    int32_t val1 = 2;
    int32_t val2 = 4;
    int32_t val3 = 8;
    dynType_complex_setValueAt(type, 0,  &inst, &val1);
    CHECK_EQUAL(2, inst.a);
    dynType_complex_setValueAt(type, 1,  &inst, &val2);
    CHECK_EQUAL(4, inst.b);
    dynType_complex_setValueAt(type, 2,  &inst, &val3);
    CHECK_EQUAL(8, inst.c);

    dynType_destroy(type);
}

TEST(DynTypeTests, AssignTest2) {
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
    CHECK_EQUAL(0, status);
    int32_t a = 2;
    double b_a = 1.1;
    double b_b = 1.2;

    dynType_complex_setValueAt(type, 0,  &inst, &a);
    CHECK_EQUAL(2, inst.a);

    void *loc = NULL;
    dyn_type *subType = NULL;
    dynType_complex_valLocAt(type, 1, (void *)&inst, &loc);
    dynType_complex_dynTypeAt(type, 1, &subType);

    dynType_complex_setValueAt(subType, 0, &inst.b, &b_a);
    CHECK_EQUAL(1.1, inst.b.a);

    dynType_complex_setValueAt(subType, 1, &inst.b, &b_b);
    CHECK_EQUAL(1.2, inst.b.b);

    dynType_destroy(type);
}

TEST(DynTypeTests, AssignTest3) {
    int simple = 1;
    dyn_type *type = NULL;
    int rc = dynType_parseWithStr("N", NULL, NULL, &type);
    CHECK_EQUAL(0, rc);

    int newValue = 42;
    void *loc = &simple;
    void *input = &newValue;
    dynType_simple_setValue(type, loc, input);
    CHECK_EQUAL(42, simple);
    dynType_destroy(type);
}

TEST(DynTypeTests, MetaInfoTest) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("#a=t;{DD#longname=longvalue;D a b c}", NULL, NULL, &type);
    //rc = dynType_parseWithStr("{DDD a b c}", NULL, NULL, &type);

    CHECK_EQUAL(0, rc);

    const char *val = NULL;
    val = dynType_getMetaInfo(type, "a");
    CHECK(val != NULL);
    CHECK(strcmp("t", val) == 0);

    val = dynType_getMetaInfo(type, "longname");
    CHECK(val == NULL);

    val = dynType_getMetaInfo(type, "nonexisting");
    CHECK(val == NULL);

    dynType_destroy(type);
}

TEST(DynTypeTests, SequenceWithPointerTest) {
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
        long e;
    };

    struct item_sequence {
        uint32_t cap;
        uint32_t len;
        struct item **buf;
    };

    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("Tval={DD a b};Titem={Jtlval;DDJ a text val c d e};**[Litem;", NULL, NULL, &type);
    CHECK_EQUAL(0, rc);

    struct item_sequence *seq = NULL;
    rc = dynType_alloc(type, (void **)&seq);
    CHECK_EQUAL(0, rc);
    CHECK(seq != NULL);

    dynType_free(type, seq);

    /*


    struct item_sequence *items = (struct item_sequence *) calloc(1,sizeof(struct item_sequence));
    items->buf = (struct item **) calloc(2, sizeof(struct item *));
    items->cap = 2;
    items->len = 2;
    items->buf[0] = (struct item *)calloc(1, sizeof(struct item));
    items->buf[0]->text = strdup("boe");
    items->buf[1] = (struct item *)calloc(1, sizeof(struct item));
    items->buf[1]->text = strdup("boe2");

    dynType_free(type, items);
     */

    dynType_destroy(type);
}

TEST(DynTypeTests, EnumTest) {
    dyn_type *type = NULL;
    int rc = 0;
    rc = dynType_parseWithStr("{#v1=0;#v2=1;E#v1=9;#v2=10;E enum1 enum2}", NULL, NULL, &type);
    CHECK_EQUAL(0, rc);

    dynType_print(type, stdout);
    dynType_destroy(type);
}
