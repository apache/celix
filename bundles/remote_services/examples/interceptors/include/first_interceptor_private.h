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
#ifndef CELIX_FIRST_INTERCEPTOR_PRIVATE_H
#define CELIX_FIRST_INTERCEPTOR_PRIVATE_H

#include <stdint.h>

#include "remote_interceptor.h"

typedef struct first_interceptor {
    uint64_t sequenceNumber;

} first_interceptor_t;

static const char *const SEQUENCE_NUMBER = "sequence.number";

celix_status_t firstInterceptor_create(first_interceptor_t **interceptor);
celix_status_t firstInterceptor_destroy(first_interceptor_t *interceptor);

bool firstInterceptor_preExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
void firstInterceptor_postExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
bool firstInterceptor_preProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
void firstInterceptor_postProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);

#endif //CELIX_FIRST_INTERCEPTOR_PRIVATE_H
