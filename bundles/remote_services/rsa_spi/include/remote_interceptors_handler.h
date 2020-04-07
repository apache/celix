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
#ifndef CELIX_REMOTE_INTERCEPTORS_HANDLER_H
#define CELIX_REMOTE_INTERCEPTORS_HANDLER_H

#include <stdint.h>
#include <celix_types.h>

#include "celix_errno.h"
#include "celix_array_list.h"
#include "remote_interceptor.h"
#include "celix_properties.h"

typedef struct remote_interceptors_handler remote_interceptors_handler_t;

celix_status_t remoteInterceptorsHandler_create(celix_bundle_context_t *ctx, remote_interceptors_handler_t **handler);
celix_status_t remoteInterceptorsHandler_destroy(remote_interceptors_handler_t *handler);

bool remoteInterceptorHandler_invokePreExportCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t **metadata);
void remoteInterceptorHandler_invokePostExportCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);
bool remoteInterceptorHandler_invokePreProxyCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t **metadata);
void remoteInterceptorHandler_invokePostProxyCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata);

#endif //CELIX_REMOTE_INTERCEPTORS_HANDLER_H
