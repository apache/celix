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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <celix_api.h>
#include "celix_log_helper.h"

typedef struct user_data {
    pthread_t logger_thread;
    pthread_mutex_t lock;
    bool running;
    celix_log_helper_t *log_helper;
} user_data_t;

static void *loggerThread(void *userData);

celix_status_t activator_start(user_data_t *data, celix_bundle_context_t *ctx) {
    data->log_helper = celix_logHelper_create(ctx, "example");
    data->running = true;
    pthread_mutex_init(&data->lock, NULL);
    pthread_create(&data->logger_thread, NULL, loggerThread, data);
    return CELIX_SUCCESS;
}

celix_status_t activator_stop(user_data_t *data, celix_bundle_context_t *ctx __attribute__((unused))) {
    pthread_mutex_lock(&data->lock);
    data->running = false;
    pthread_mutex_unlock(&data->lock);
    pthread_join(data->logger_thread, NULL);
    celix_logHelper_destroy(data->log_helper);
    return CELIX_SUCCESS;
}


static void *loggerThread(void *userData) {
    user_data_t *data = userData;

    bool running = true;
    while (running) {
        celix_logHelper_info(data->log_helper, "My log message");
        sleep(1);
        pthread_mutex_lock(&data->lock);
        running = data->running;
        pthread_mutex_unlock(&data->lock);
    }
    return NULL;
}

CELIX_GEN_BUNDLE_ACTIVATOR(user_data_t, activator_start, activator_stop)

