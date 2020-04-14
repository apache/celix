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
#ifndef CELIX_SECOND_INTERCEPTOR_PRIVATE_H
#define CELIX_SECOND_INTERCEPTOR_PRIVATE_H

#include <stdint.h>

#include "remote_interceptor.h"

typedef struct second_interceptor {
} second_interceptor_t;

celix_status_t secondInterceptor_create(second_interceptor_t **interceptor);
celix_status_t secondInterceptor_destroy(second_interceptor_t *interceptor);

bool secondInterceptor_preExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
void secondInterceptor_postExportCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
bool secondInterceptor_preProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
void secondInterceptor_postProxyCall(void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);

#endif //CELIX_SECOND_INTERCEPTOR_PRIVATE_H
