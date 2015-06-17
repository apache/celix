#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dyn_function.h"

#define EXAMPLE1_SCHEMA "example(III)I"
int32_t example1(int32_t a, int32_t b, int32_t c) {
    printf("example1 called with a:%i, b:%i, c:%i\n", a, b, c);
    return 1;
}

#define EXAMPLE2_SCHEMA "example(I{IID val1 val2 val3}D)D"
struct example2_arg {
    int32_t val1;
    int32_t val2;
    double val3;
};

double example2(int32_t arg1, struct example2_arg *arg2, double arg3) {
    printf("example2 called with arg1:%i, arg2:{val1:%i, val2:%i, val3:%f}, arg3:%f\n", arg1, arg2->val1, arg2->val2, arg2->val3, arg3);
    return 2.2;
}


int main() {
    dyn_function_type *dynFunc = NULL;
    int rc;

    rc = dynFunction_create(EXAMPLE1_SCHEMA, (void *)example1, &dynFunc);
    if (rc == 0 ) {
        int32_t a = 2;
        int32_t b = 4;
        int32_t c = 8;
        void *values[3];
        int32_t rVal = 0;
        values[0] = &a;
        values[1] = &b;
        values[2] = &c;
        dynFunction_call(dynFunc, &rVal, values);
        dynFunction_destroy(dynFunc);
        dynFunc = NULL;
    } else {
        printf("Example 1 failed\n");
    }

    rc = dynFunction_create(EXAMPLE2_SCHEMA, (void *)example2, &dynFunc);
    if (rc == 0 ) {
        int32_t arg1 = 2;
        struct example2_arg complexVal = { .val1 = 2, .val2 = 3, .val3 = 4.1 };
        struct example2_arg *arg2 = &complexVal;
        double arg3 = 8.1;
        double returnVal = 0;
        void *values[3];
        values[0] = &arg1;
        values[1] = &arg2;
        values[2] = &arg3;
        dynFunction_call(dynFunc, &returnVal, values);
        dynFunction_destroy(dynFunc);
        dynFunc = NULL;
    } else {
        printf("Example 3 failed\n");
    }
}
