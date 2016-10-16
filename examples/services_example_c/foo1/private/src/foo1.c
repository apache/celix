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

#include "foo1.h"

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

static void* foo1_thread(void*);

struct foo1_struct {
    const example_t *example;
    pthread_mutex_t mutex; //protecting example
    pthread_t thread;
    bool running;
};

foo1_t* foo1_create(void) {
    foo1_t *self = calloc(1, sizeof(*self));
    if (self != NULL) {
        pthread_mutex_init(&self->mutex, NULL);
        self->running = false;
    } else {
        //log error
    }
    return self;
};

void foo1_destroy(foo1_t *self) {
    assert(!self->running);
    pthread_mutex_destroy(&self->mutex);
    free(self);
}

int foo1_start(foo1_t *self) {
    printf("starting foo1\n");
    self->running = true;
    pthread_create(&self->thread, NULL, foo1_thread, self);
    return OK;
}

int foo1_stop(foo1_t *self) {
    printf("stopping foo1\n");
    self->running = false;
    pthread_kill(self->thread, SIGUSR1);
    pthread_join(self->thread, NULL);
    return OK;
}

int foo1_setExample(foo1_t *self, const example_t *example) {
    printf("Setting example %p for foo1\n", example);
    pthread_mutex_lock(&self->mutex);
    self->example = example; //NOTE could be NULL if req is not mandatory
    pthread_mutex_unlock(&self->mutex);
    return OK;
}

static void* foo1_thread(void *userdata) {
    foo1_t *self = userdata;
    double result;
    int rc;
    while (self->running) {
        pthread_mutex_lock(&self->mutex);
        if (self->example != NULL) {
            rc = self->example->method(self->example->handle, 1, 2.0, &result);
            if (rc == 0) {
                printf("Result is %f\n", result);
            } else {
                printf("Error invoking method for example\n");
            }
        }
        pthread_mutex_unlock(&self->mutex);
        usleep(30000000);
    }
    return NULL;
}