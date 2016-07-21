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

#include "bar.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>


#define OK 0
#define ERROR 1

struct bar_struct {
    double prefValue;
};

bar_t* bar_create(void) {
    bar_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        self->prefValue = 42;
    } else {
        //log error
    }
    return self;
};

void bar_destroy(bar_t *self) {
    free(self);
}

int bar_method(bar_t *self, int arg1, double arg2, double *out) {
    double update = (self->prefValue + arg1) * arg2;
    self->prefValue = update;
    *out = update;
    return OK;
}