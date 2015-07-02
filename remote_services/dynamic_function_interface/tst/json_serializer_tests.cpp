/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ffi.h>

#include "dyn_type.h"
#include "json_serializer.h"

static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
}

/*********** example 1 ************************/
/** struct type ******************************/
const char *example1_descriptor = "{DJISF a b c d e}";

const char *example1_input = "{ \
    \"a\" : 1.0, \
    \"b\" : 22, \
    \"c\" : 32, \
    \"d\" : 42, \
    \"e\" : 4.4 \
}";

struct example1 {
    double a;   //0
    int64_t b;  //1
    int32_t c;  //2
    int16_t d;  //3
    float e;    //4
};

static void print_example1(void *data) {
    struct example1 *ex = (struct example1 *)data;
    printf("example1: a:%f, b:%li, c:%i, d:%i, e:%f\n", ex->a, ex->b, ex->c, ex->d, ex->e);
}

/*********** example 2 ************************/
const char *example2_descriptor = "{BJJDFD byte long1 long2 double1 float1 double2}";

const char *example2_input = "{ \
    \"byte\" : 42, \
    \"long1\" : 232, \
    \"long2\" : 242, \
    \"double1\" : 4.2, \
    \"float1\" : 3.2, \
    \"double2\" : 4.4 \
}";

struct example2 {
    char byte;      //0
    int64_t long1;     //1
    int64_t long2;     //2
    double double1; //3
    float float1;   //4
    double double2; //5
};

static void print_example2(void *data) {
    struct example2 *ex = (struct example2 *)data;
    printf("example2: byte:%i, long1:%li, long2:%li, double1:%f, float1:%f, double2:%f\n", ex->byte, ex->long1, ex->long2, ex->double1, ex->float1, ex->double2);
}


/*********** example 3 ************************/
/** sequence with a simple type **************/
const char *example3_descriptor = "{[I numbers}";

const char *example3_input = "{ \
    \"numbers\" : [22,32,42] \
}";

struct example3 {
    struct {
        uint32_t _cap;
        uint32_t _len;
        int32_t *buf;
    } numbers;
};

static void print_example3(void *data) {
    struct example3 *ex = (struct example3 *)data;
    printf("example3: numbers length is %u and cap is %u and pointer buf is %p\n", ex->numbers._len, ex->numbers._cap, ex->numbers.buf);
    int i;
    for (i = 0; i < ex->numbers._len; i += 1) {
        printf("\telement %i : %i\n", i, ex->numbers.buf[i]);
    }
}

/*********** example 4 ************************/
/** structs within a struct (by reference)*******/
//TODO think about references in descriptor e.g "{Lleaf;Lleaf; left right}\nleaf{IDD index val1 val2}"
const char *example4_descriptor = "{{IDD index val1 val2}{IDD index val1 val2} left right}";

static const char *example4_input =  "{ \
    \"left\" : {\"index\":1, \"val1\":1.0, \"val2\":2.0 }, \
    \"right\" : {\"index\":2, \"val1\":5.0, \"val2\":4.0 } \
}";

struct leaf {
    int32_t index;
    double val1;
    double val2;
};

struct example4 {
    struct leaf left;
    struct leaf right;
};

static void print_example4(void *data) {
    struct example4 *ex = (struct example4 *)data;
    printf("example4: left { index:%i, val1:%f, val2:%f }, right { index;%i, val1:%f, val2:%f }\n", ex->left.index, ex->left.val1, ex->left.val2, ex->right.index, ex->right.val1, ex->right.val2);
}

static void tests() {
    printf("Starting json serializer tests\n");
    dyn_type *type;
    void *inst;
    int rc;

    type = NULL;
    inst = NULL;
    rc = dynType_create(example1_descriptor, &type);    
    CHECK_EQUAL(0, rc);
    rc = json_deserialize(type, example1_input, &inst); 
    CHECK_EQUAL(0, rc);
    print_example1(inst);
    printf("--\n\n");


    type = NULL;
    inst = NULL;
    rc = dynType_create(example2_descriptor, &type);
    CHECK_EQUAL(0, rc);
    rc = json_deserialize(type, example2_input, &inst); 
    CHECK_EQUAL(0, rc);
    print_example2(inst);
    printf("--\n\n");

    type = NULL;
    inst = NULL;
    rc = dynType_create(example3_descriptor, &type);
    CHECK_EQUAL(0, rc);
    rc = json_deserialize(type, example3_input, &inst); 
    CHECK_EQUAL(0, rc);
    print_example3(inst);

    type = NULL;
    inst = NULL;
    rc = dynType_create(example4_descriptor, &type);
    CHECK_EQUAL(0, rc);
    rc = json_deserialize(type, example4_input, &inst); 
    CHECK_EQUAL(0, rc);
    print_example4(inst);
}
}

TEST_GROUP(JsonSerializerTests) {
    void setup() {
    }
};

TEST(JsonSerializerTests, Test1) {
    //TODO split up
    tests();
}
