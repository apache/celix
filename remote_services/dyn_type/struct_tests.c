#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ffi.h>

#include "dyn_type.h"
#include "json_serializer.h"

/*********** example 1 ************************/
/** struct type ******************************/
const char *example1_schema = "{DJISF a b c d e}";

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

void print_example1(void *data) {
    struct example1 *ex = data;
    printf("example1: a:%f, b:%li, c:%i, d:%i, e:%f\n", ex->a, ex->b, ex->c, ex->d, ex->e);
}

/*********** example 2 ************************/
/** struct type with some extra whitespace*/
const char *example2_schema = "  {  BJJDFD byte long1 long2 double1   float1 double2  } ";

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

void print_example2(void *data) {
    struct example2 *ex = data;
    printf("example2: byte:%i, long1:%li, long2:%li, double1:%f, float1:%f, double2:%f\n", ex->byte, ex->long1, ex->long2, ex->double1, ex->float1, ex->double2);
}


/*********** example 3 ************************/
/** sequence with a simple type **************/
const char *example3_schema = "{[I numbers}";

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

void print_example3(void *data) {
    struct example3 *ex = data;
    printf("example3: numbers length is %u and cap is %u and pointer buf is %p\n", ex->numbers._len, ex->numbers._cap, ex->numbers.buf);
    int i;
    for (i = 0; i < ex->numbers._len; i += 1) {
        printf("\telement %i : %i\n", i, ex->numbers.buf[i]);
    }
}

/*********** example 4 ************************/
/** structs within a struct (by reference)*******/
//TODO think about references in schema e.g "{Lleaf;Lleaf; left right}\nleaf{IDD index val1 val2}"
const char *example4_schema = "{{IDD index val1 val2}{IDD index val1 val2} left right}";

const char *example4_input =  "{ \
    \"left\" : {\"index\":1, \"val1\":1.0, \"val2\":2.0 }, \
    \"right\" : {\"index\":2, \"val1\":5.0, \"val2\":4.0 } \
}";

struct leaf {
    int32_t index;
    double val1;
    double val2;
};

struct example4 {
    struct leaf *left;
    struct leaf *right;
};

void print_example4(void *data) {
    struct example4 *ex = data;
    printf("example4: left { index:%i, val1:%f, val2:%f }, right { index;%i, val1:%f, val2:%f }\n", ex->left->index, ex->left->val1, ex->left->val2, ex->right->index, ex->right->val1, ex->right->val2);
}

int main() {
    printf("Starting ffi struct test\n");
    dyn_type *type;
    void *inst;

    type = NULL;
    inst = NULL;
    dynType_create(example1_schema, &type);
    printf("-- example 1\n");
    dynType_print(type);
    json_deserialize(type, example1_input, &inst); 
    print_example1(inst);
    printf("--\n\n");


    type = NULL;
    inst = NULL;
    dynType_create(example2_schema, &type);
    printf("-- example 2\n");
    dynType_print(type);
    json_deserialize(type, example2_input, &inst); 
    print_example2(inst);
    printf("--\n\n");

    type = NULL;
    inst = NULL;
    dynType_create(example3_schema, &type);
    printf("-- example 3\n");
    dynType_print(type);
    json_deserialize(type, example3_input, &inst); 
    print_example3(inst);
    printf("--\n\n");

    type = NULL;
    inst = NULL;
    dynType_create(example4_schema, &type);
    printf("-- example 4\n");
    dynType_print(type);
    json_deserialize(type, example4_input, &inst); 
    print_example4(inst);
    printf("--\n\n");

    return 0;
}
