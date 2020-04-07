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
#include "second_interceptor_private.h"

#include <stdlib.h>

celix_status_t secondInterceptor_create(second_interceptor_t **interceptor) {
    celix_status_t status = CELIX_SUCCESS;

    *interceptor = calloc(1, sizeof(**interceptor));
    if (!*interceptor) {
        status = CELIX_ENOMEM;
    } else {
    }

    return status;
}

celix_status_t secondInterceptor_destroy(second_interceptor_t *interceptor) {
    free(interceptor);
    return CELIX_SUCCESS;
}


bool secondInterceptor_preExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    printf("Invoked preExportCall on second interceptor\n");

    return true;
}

void secondInterceptor_postExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    printf("Invoked postExportCall on second interceptor\n");
}

bool secondInterceptor_preProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    printf("Invoked preProxyCall on second interceptor\n");

    return true;
}

void secondInterceptor_postProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    printf("Invoked postProxyCall on second interceptor\n");
}

