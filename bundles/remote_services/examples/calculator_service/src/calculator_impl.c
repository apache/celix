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


#include <math.h>

#include <stdlib.h>
#include <stdio.h>

#include "calculator_impl.h"

calculator_t* calculator_create(void) {
    struct calculator *calc = calloc(1, sizeof(*calc));
    return calc;
}

void calculator_destroy(calculator_t *calculator) {
    free(calculator);
}

int calculator_add(calculator_t *calculator __attribute__((unused)), double a, double b, double *result) {
    int status = CELIX_SUCCESS;

    *result = a + b;
    printf("CALCULATOR: Add: %f + %f = %f\n", a, b, *result);

    return status;
}

int calculator_sub(calculator_t *calculator __attribute__((unused)), double a, double b, double *result) {
    int status = CELIX_SUCCESS;

    *result = a - b;
    printf("CALCULATOR: Sub: %f - %f = %f\n", a, b, *result);

    return status;
}

int calculator_sqrt(calculator_t *calculator __attribute__((unused)), double a, double *result) {
    int status = CELIX_SUCCESS;

    if (a > 0) {
        *result = sqrt(a);
        printf("CALCULATOR: Sqrt: %f = %f\n", a, *result);
    } else {
        printf("CALCULATOR: Sqrt: %f = ERR\n", a);
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}
