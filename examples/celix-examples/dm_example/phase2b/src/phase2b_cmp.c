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
/**
 * phase2b_cmp.c
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "celix_threads.h"
#include "phase2b_cmp.h"

#define SLEEPTIME 2000000

struct phase2b_cmp_struct {
    celix_thread_t thread;
    bool running;
    double currentValue;
    celix_thread_mutex_t mutex;
    const phase1_t* phase1Serv;
};

static void *phase2b_thread(void *data);

phase2b_cmp_t *phase2b_create(void) {
    phase2b_cmp_t *cmp = calloc(1, sizeof(*cmp));
    if (cmp != NULL) {
        cmp->currentValue = 0.0;
        cmp->running = false;
        celixThreadMutex_create(&cmp->mutex, NULL);
    }
    return cmp;
}

int phase2b_init(phase2b_cmp_t *cmp) {
    printf("init phase2b\n");
    return 0;
}

int phase2b_start(phase2b_cmp_t *cmp) {
    printf("start phase2b\n");
    cmp->running = true;
    celixThread_create(&cmp->thread, NULL, phase2b_thread, cmp);
    return 0;
}

int phase2b_stop(phase2b_cmp_t *cmp) {
    printf("stop phase2b\n");
    cmp->running = false;
    celixThread_kill(cmp->thread, SIGUSR1);
    celixThread_join(cmp->thread, NULL);
    return 0;
}

int phase2b_deinit(phase2b_cmp_t *cmp) {
    printf("deinit phase2b\n");
    return 0;
}

void phase2b_destroy(phase2b_cmp_t *cmp) {
    celixThreadMutex_destroy(&cmp->mutex);
    free(cmp);
    printf("destroy phase2b\n");
}

int phase2b_setPhase1(phase2b_cmp_t *cmp, const phase1_t* phase1) {
    celixThreadMutex_lock(&cmp->mutex);
    cmp->phase1Serv = phase1;
    celixThreadMutex_unlock(&cmp->mutex);
    return 0;
}

static void *phase2b_thread(void *data) {
    phase2b_cmp_t *cmp = data;
    unsigned int counter;

    while (cmp->running) {
        celixThreadMutex_lock(&cmp->mutex);
        if (cmp->phase1Serv != NULL) {
            cmp->phase1Serv->getData(cmp->phase1Serv->handle, &counter);
            cmp->currentValue = counter / 1000;
        }
        celixThreadMutex_unlock(&cmp->mutex);
        usleep(SLEEPTIME);
    }

    celixThread_exit(NULL);
    return NULL;
}

int phase2b_getData(phase2b_cmp_t *cmp, double *data) {
    celixThreadMutex_lock(&cmp->mutex);
    *data = cmp->currentValue;
    celixThreadMutex_unlock(&cmp->mutex);
    return 0;
}
