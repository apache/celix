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
 * phase3_cmp.c
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "celix_array_list.h"
#include "celix_threads.h"
#include "phase3_cmp.h"

#define SLEEPTIME 2000000

struct phase3_cmp_struct {
    celix_thread_t thread;
    bool running;
    double currentValue;
    celix_array_list_t *phase2Services; //phase2_t *
};

static void *phase3_thread(void *data);

phase3_cmp_t *phase3_create() {
    phase3_cmp_t *cmp = calloc(1, sizeof(*cmp));
    if (cmp != NULL) {
        cmp->currentValue = 0.0;
        cmp->running = false;
        cmp->phase2Services = celix_arrayList_createPointerArray();
    }
    return cmp;
}

int phase3_init(phase3_cmp_t *cmp) {
    printf("init phase3\n");
    return 0;
}

int phase3_start(phase3_cmp_t *cmp) {
    printf("start phase3\n");
    cmp->running = true;
    celixThread_create(&cmp->thread, NULL, phase3_thread, cmp);
    return 0;
}

int phase3_stop(phase3_cmp_t *cmp) {
    printf("stop phase3\n");
    cmp->running = false;
    celixThread_kill(cmp->thread, SIGUSR1);
    celixThread_join(cmp->thread, NULL);
    return 0;
}

int phase3_deinit(phase3_cmp_t *cmp) {
    printf("deinit phase3\n");
    return 0;
}

void phase3_destroy(phase3_cmp_t *cmp) {
    celix_arrayList_destroy(cmp->phase2Services);
    free(cmp);
    printf("destroy phase3\n");
}

int phase3_addPhase2(phase3_cmp_t *cmp, const phase2_t* phase2) {
    celix_arrayList_add(cmp->phase2Services, (void*)phase2);
    return 0;
}

int phase3_removePhase2(phase3_cmp_t *cmp, const phase2_t *phase2) {
    celix_arrayList_remove(cmp->phase2Services, (void*)phase2);
    return 0;
}


static void *phase3_thread(void *data) {
    phase3_cmp_t *cmp = data;
    int size;
    int i;
    double value;

    while (cmp->running) {
        size = celix_arrayList_size(cmp->phase2Services);
        for (i = 0; i < size; i += 1) {
            const phase2_t* serv = celix_arrayList_get(cmp->phase2Services, i);
            serv->getData(serv->handle, &value);
            printf("PHASE3: Data from %p is %f\n", serv, value);
        }
        usleep(SLEEPTIME);
    }

    celixThread_exit(NULL);
    return NULL;
}
