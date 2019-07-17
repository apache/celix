/**
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
/*
 * activator.c
 *
 *  \date       Aug 20, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include "bundle_activator.h"

#include "test.h"
#include "test2.h"

struct userData {
    char * word;
};

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    *userData = malloc(sizeof(struct userData));
    if (*userData != NULL) {
        ((struct userData *)(*userData))->word = "World";
    } else {
        status = CELIX_START_ERROR;
    }
    return status;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    struct userData * data = (struct userData *) userData;
    printf("Hello %s\n", data->word);

    doo();
    bar();

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    struct userData * data = (struct userData *) userData;
    printf("Goodbye %s\n", data->word);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
    free(userData);
    return CELIX_SUCCESS;
}
