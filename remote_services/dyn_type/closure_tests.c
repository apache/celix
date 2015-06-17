#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dyn_function.h"

#define EXAMPLE1_SCHEMA "example(III)I"
void example1_binding(void *userData, void* args[], void *out) {
    printf("example1 closure called\n");
    int32_t a = *((int32_t *)args[0]);
    int32_t b = *((int32_t *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    int32_t *ret = out;
    *ret = a + b + c;
}

#define EXAMPLE2_SCHEMA "example(I{DDD val1 val2 val3}I)D"
struct example2_arg2 {
    double val1;
    double val2;
    double val3;
};
void example2_binding(void *userData, void* args[], void *out) {
    printf("example2 closure called\n");
    int32_t a = *((int32_t *)args[0]);
    struct example2_arg2 *b =  *((struct example2_arg2 **)args[1]);
    int32_t c = *((int32_t *)args[2]);
    int32_t *ret = out;
    *ret = a + b->val1 + b->val2 + b->val3 + c;
}


#define EXAMPLE3_SCHEMA "example(III){III sum max min}"
struct example3_ret {
    int32_t sum;
    int32_t max;
    int32_t min;
};

void example3_binding(void *userData, void* args[], void *out) {
    printf("exampleelosure called\n");
    int32_t a = *((int32_t *)args[0]);
    int32_t b = *((int32_t *)args[1]);
    int32_t c = *((int32_t *)args[2]);
    struct example3_ret *result = calloc(1,sizeof(*result));
    result->sum = a + b + c;
    result->min = a <= b ? a : b;
    result->max = a >= b ? a : b;
    result->min = result->min <= c ? result->min : c;
    result->max = result->max >= c ? result->max : c;

    struct example3_ret **ret = out;
    (*ret) = result;
}

int main() {
    dyn_closure_type *dynClosure = NULL;
    int rc;

    rc = dynClosure_create(EXAMPLE1_SCHEMA, example1_binding, NULL, &dynClosure);
    if (rc == 0) {
        int32_t (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        dynClosure_getFnPointer(dynClosure, (void *)&func);
        int32_t ret = func(2,3,4);
        printf("Return value for example1 is %i\n", ret);
        dynClosure_destroy(dynClosure);
    } else {
        printf("example1 failed\n");
    }

    dynClosure = NULL;
    rc = dynClosure_create(EXAMPLE2_SCHEMA, example2_binding, NULL, &dynClosure);
    if (rc == 0) {
        double (*func)(int32_t a, struct example2_arg2 *b, int32_t c) = NULL;
        dynClosure_getFnPointer(dynClosure, (void *)&func);
        struct example2_arg2 b = { .val1 = 1.0, .val2 = 1.5, .val3 = 2.0 };
        double ret = func(2,&b,4);
        printf("Return value for example2 is %f\n", ret);
        dynClosure_destroy(dynClosure);
    } else {
        printf("example2 failed\n");
    }

    dynClosure = NULL;
    rc = dynClosure_create(EXAMPLE3_SCHEMA, example3_binding, NULL, &dynClosure);
    if (rc == 0) {
        struct example3_ret * (*func)(int32_t a, int32_t b, int32_t c) = NULL;
        dynClosure_getFnPointer(dynClosure, (void *)&func);
        struct example3_ret *ret = func(2,8,4);
        printf("Return value for example3 is {sum:%i, max:%i, min:%i}\n", ret->sum, ret->max, ret->min);
        dynClosure_destroy(dynClosure);
        free(ret);
    } else {
        printf("example2 failed\n");
    }
}
