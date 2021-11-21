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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "celix_errno.h"
#include "celix_array_list.h"
#include "pubsub_interceptor.h"
#include "celix_properties.h"

typedef struct pubsub_interceptors_handler pubsub_interceptors_handler_t;

/**
 * @brief Creates a pubsub interceptor handler for a specific scope, topic, psa type and serialization type.
 *
 * A interceptor handler will track pubsub interceptors and will call these interceptors when using the
 * invokePreSend, invokePostSend, invokePreReceive and invokePostReceive functions.
 *
 * The interceptor handler will forward the topic, scope, psa type and serialization type info to every interceptor
 * functions as pubsub_interceptor_properties_t.
 *
 */
pubsub_interceptors_handler_t* pubsubInterceptorsHandler_create(celix_bundle_context_t* ctx, const char* scope, const char* topic,  const char* psaType, const char* serializationType);

/**
 * @brief Destroy the interceptor handler
 * @param handler
 */
void pubsubInterceptorsHandler_destroy(pubsub_interceptors_handler_t *handler);

/**
 * @brief Calls all the tracked interceptor service preSend functions.
 */
bool pubsubInterceptorHandler_invokePreSend(pubsub_interceptors_handler_t *handler, const char* messageType, uint32_t messageId, const void* message, celix_properties_t** metadata);

/**
 * @brief Calls all the tracked interceptor service postSend functions.
 */
void pubsubInterceptorHandler_invokePostSend(pubsub_interceptors_handler_t *handler, const char* messageType, uint32_t messageId, const void* message, celix_properties_t* metadata);

/**
 * @brief Calls all the tracked interceptor service preReceive functions.
 */
bool pubsubInterceptorHandler_invokePreReceive(pubsub_interceptors_handler_t *handler, const char* messageType, uint32_t messageId, const void* message, celix_properties_t** metadata);

/**
 * @brief Calls all the tracked interceptor service postReceive functions.
 */
void pubsubInterceptorHandler_invokePostReceive(pubsub_interceptors_handler_t *handler, const char* messageType, uint32_t messageId, const void* message, celix_properties_t* metadata);

/**
 * @brief Return the nr of interceptors currently tracked.
 */
size_t pubsubInterceptorHandler_nrOfInterceptors(pubsub_interceptors_handler_t *handler);

#ifdef __cplusplus
}
#endif
#endif //PUBSUB_INTERCEPTORS_HANDLER_H
