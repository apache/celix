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
 * phase1_cmp.c
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "celix_threads.h"
#include "phase1_cmp.h"
#include "dm_component.h"

#define SLEEPTIME 1000

struct phase1_cmp_struct {
    celix_thread_t thread;
    bool running;
    unsigned int counter;
    celix_dm_component_t *component;
};

static void *phase1_thread(void *data);

phase1_cmp_t *phase1_create(void) {
    phase1_cmp_t *cmp = calloc(1, sizeof(*cmp));
    if (cmp != NULL) {
        cmp->counter = 0;
        cmp->running = false;
    }
    return cmp;
}

void phase1_setComp(phase1_cmp_t *cmp, celix_dm_component_t *dmCmp) {
    cmp->component = dmCmp;
}

int phase1_init(phase1_cmp_t *cmp) {
    printf("init phase1\n");
    return 0;
}

int phase1_start(phase1_cmp_t *cmp) {
    printf("start phase1\n");
    cmp->running = true;
    celixThread_create(&cmp->thread, NULL, phase1_thread, cmp);
    return 0;
}

int phase1_stop(phase1_cmp_t *cmp) {
    printf("stop phase1\n");
    cmp->running = false;
    celixThread_kill(cmp->thread, SIGUSR1);
    celixThread_join(cmp->thread, NULL);
    return 0;
}

int phase1_deinit(phase1_cmp_t *cmp) {
    printf("deinit phase1\n");
    return 0;
}

void phase1_destroy(phase1_cmp_t *cmp) {
    free(cmp);
    printf("destroy phase1\n");
}

static void *phase1_thread(void *data) {
    phase1_cmp_t *cmp = data;

    while (cmp->running) {
        cmp->counter += 1;
        if (cmp->counter == 2) {
            static char *runtime_interface = "DUMMY INTERFACE: DO NOT USE\n";
            celix_dmComponent_addInterface(cmp->component, "RUNTIME_ADDED_INTERFACE", "1.0.0", runtime_interface, NULL);
        }
        usleep(SLEEPTIME);
    }

    celixThread_exit(NULL);
    return NULL;
}

int phase1_getData(phase1_cmp_t *cmp, unsigned int *data) {
    *data = cmp->counter;
    return 0;
}
