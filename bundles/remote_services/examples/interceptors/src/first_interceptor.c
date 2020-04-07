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
#include "first_interceptor_private.h"

#include <stdlib.h>
#include <inttypes.h>

celix_status_t firstInterceptor_create(first_interceptor_t **interceptor) {
    celix_status_t status = CELIX_SUCCESS;

    *interceptor = calloc(1, sizeof(**interceptor));
    if (!*interceptor) {
        status = CELIX_ENOMEM;
    } else {
        (*interceptor)->sequenceNumber = 0;
    }

    return status;
}

celix_status_t firstInterceptor_destroy(first_interceptor_t *interceptor) {
    free(interceptor);
    return CELIX_SUCCESS;
}


bool firstInterceptor_preExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    uint64_t sequence = celix_properties_getAsLong(metadata, SEQUENCE_NUMBER, 0);
    printf("Invoked preExportCall on first interceptor, for message with sequenceNumber [%"PRIu64"]\n", sequence);

    return true;
}

void firstInterceptor_postExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    uint64_t sequence = celix_properties_getAsLong(metadata, SEQUENCE_NUMBER, 0);
    printf("Invoked postExportCall on first interceptor, for message with sequenceNumber [%"PRIu64"]\n", sequence);
}

bool firstInterceptor_preProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    first_interceptor_t *interceptor = handle;
    celix_properties_setLong((celix_properties_t *) metadata, SEQUENCE_NUMBER, interceptor->sequenceNumber++);

    uint64_t sequence = celix_properties_getAsLong(metadata, SEQUENCE_NUMBER, 0);
    printf("Invoked preProxyCall on first interceptor, for message with sequenceNumber [%"PRIu64"]\n", sequence);

    return true;
}

void firstInterceptor_postProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    uint64_t sequence = celix_properties_getAsLong(metadata, SEQUENCE_NUMBER, 0);
    printf("Invoked postProxyCall on first interceptor, for message with sequenceNumber [%"PRIu64"]\n", sequence);
}

