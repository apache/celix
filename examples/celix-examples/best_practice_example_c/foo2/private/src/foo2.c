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

#include "foo2.h"

#include "array_list.h"

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

static void* foo2_thread(void*);

struct foo2_struct {
    celix_array_list_t *examples;
    pthread_t thread;
    bool running;
};

foo2_t* foo2_create(void) {
    foo2_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        self->examples = NULL;
        arrayList_create(&self->examples);
        self->running = false;
    } else {
        //log error
    }
    return self;
};

void foo2_destroy(foo2_t *self) {
    assert(!self->running);
    arrayList_destroy(self->examples);
    free(self);
}

int foo2_start(foo2_t *self) {
    printf("starting foo2\n");
    self->running = true;
    pthread_create(&self->thread, NULL, foo2_thread, self);
    return OK;
}

int foo2_stop(foo2_t *self) {
    printf("stopping foo2\n");
    self->running = false;
    pthread_kill(self->thread, SIGUSR1);
    pthread_join(self->thread, NULL);
    return OK;
}

int foo2_addExample(foo2_t *self, const example_t *example) {
    //NOTE foo2 is suspended -> thread is not running  -> safe to update
    int status = OK;
    printf("Adding example %p for foo2\n", example);
    status = arrayList_add(self->examples, (void *)example);
    return status;
}

int foo2_removeExample(foo2_t *self, const example_t *example) {
    //NOTE foo2 is suspended -> thread is not running  -> safe to update
    int status = OK;
    printf("Removing example %p for foo2\n", example);
    status = arrayList_removeElement(self->examples, (void*)example);
    return status;
}

static void* foo2_thread(void *userdata) {
    foo2_t *self = userdata;
    double result;
    int rc;
    while (self->running) {
        unsigned int size = arrayList_size(self->examples);
        int i;
        for (i = 0; i < size; i += 1) {
            const example_t* example = arrayList_get(self->examples, i);
            rc = example->method(example->handle, 1, 2.0, &result);
            if (rc == 0) {
                printf("Result is %f\n", result);
            } else {
                printf("Error invoking method for example\n");
            }
        }
        usleep(15000000);
    }
    return NULL;
}