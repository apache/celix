/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {
    #include <string.h>
    #include <stdarg.h>
    
    #include "dyn_common.h"
    #include "dyn_type.h"

	static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
	    va_list ap;
	    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
	    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
	    va_start(ap, msg);
	    vfprintf(stderr, msg, ap);
	    fprintf(stderr, "\n");
	}

    static void runTest(const char *descriptorStr, const char *exName) {
        dyn_type *type;
        int i;
        type = NULL;
        printf("\n-- example %s with descriptor string '%s' --\n", exName, descriptorStr);
        int status = dynType_parseWithStr(descriptorStr, exName, NULL, &type);
        CHECK_EQUAL(0, status);
        if (status == 0) {
            dynType_print(type);
            dynType_destroy(type);
        }
        printf("--\n\n");
    }
}

TEST_GROUP(DynTypeTests) {
	void setup() {
	    dynType_logSetup(stdLog, NULL, 3);
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

TEST(DynTypeTests, ParseRandomGarbageTest) {
    return; //TODO enable
    unsigned int seed = 4148;
    char *testRandom = getenv("DYN_TYPE_TEST_RANDOM");
    if (testRandom != NULL && strcmp("true", testRandom) == 0) {
        seed = (unsigned int) time(NULL);
    } 
    srandom(seed);
    size_t nrOfTests = 10000;

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


//TODO allocating / freeing

//TODO sequences 
