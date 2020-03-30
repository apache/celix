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
#ifndef PUBSUB_INTERCEPTORS_HANDLER_H
#define PUBSUB_INTERCEPTORS_HANDLER_H

#include <stdint.h>

#include "celix_errno.h"
#include "celix_array_list.h"
#include "pubsub_interceptor.h"
#include "celix_properties.h"

typedef struct pubsub_interceptors_handler pubsub_interceptors_handler_t;

celix_status_t pubsubInterceptorsHandler_create(celix_bundle_context_t *ctx, const char *scope, const char *topic, pubsub_interceptors_handler_t **handler);
celix_status_t pubsubInterceptorsHandler_destroy(pubsub_interceptors_handler_t *handler);

bool pubsubInterceptorHandler_invokePreSend(pubsub_interceptors_handler_t *handler, const char *messageType, const uint32_t messageId, const void *message, celix_properties_t **metadata);
void pubsubInterceptorHandler_invokePostSend(pubsub_interceptors_handler_t *handler, const char *messageType, const uint32_t messageId, const void *message, celix_properties_t *metadata);
bool pubsubInterceptorHandler_invokePreReceive(pubsub_interceptors_handler_t *handler, const char *messageType, const uint32_t messageId, const void *message, celix_properties_t **metadata);
void pubsubInterceptorHandler_invokePostReceive(pubsub_interceptors_handler_t *handler, const char *messageType, const uint32_t messageId, const void *message, celix_properties_t *metadata);

#endif //PUBSUB_INTERCEPTORS_HANDLER_H
