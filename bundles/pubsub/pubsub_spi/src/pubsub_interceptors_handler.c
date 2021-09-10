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
#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_array_list.h"
#include "celix_utils.h"

#include "pubsub_interceptors_handler.h"

typedef struct entry {
    const celix_properties_t *properties;
    pubsub_interceptor_t *interceptor;
} entry_t;

struct pubsub_interceptors_handler {
    pubsub_interceptor_properties_t properties;

    celix_array_list_t *interceptors;

    long interceptorsTrackerId;

    celix_bundle_context_t *ctx;

    celix_thread_mutex_t lock;
};

static int referenceCompare(const void *a, const void *b);

static void pubsubInterceptorsHandler_addInterceptor(void *handle, void *svc, const celix_properties_t *props);
static void pubsubInterceptorsHandler_removeInterceptor(void *handle, void *svc, const celix_properties_t *props);

pubsub_interceptors_handler_t* pubsubInterceptorsHandler_create(celix_bundle_context_t *ctx, const char *scope, const char *topic, const char* psaType, const char* serType) {
    pubsub_interceptors_handler_t* handler = calloc(1, sizeof(*handler));
    handler->ctx = ctx;
    handler->properties.scope = celix_utils_strdup(scope);
    handler->properties.topic = celix_utils_strdup(topic);
    handler->properties.psaType = celix_utils_strdup(psaType);
    handler->properties.serializationType = celix_utils_strdup(serType);
    handler->interceptors = celix_arrayList_create();
    celixThreadMutex_create(&handler->lock, NULL);

    // Create service tracker here, and not in the activator
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = PUBSUB_INTERCEPTOR_SERVICE_NAME;
    opts.filter.ignoreServiceLanguage = true;
    opts.callbackHandle = handler;
    opts.addWithProperties = pubsubInterceptorsHandler_addInterceptor;
    opts.removeWithProperties = pubsubInterceptorsHandler_removeInterceptor;
    handler->interceptorsTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

    return handler;
}

void pubsubInterceptorsHandler_destroy(pubsub_interceptors_handler_t *handler) {
    if (handler != NULL) {
        celix_bundleContext_stopTracker(handler->ctx, handler->interceptorsTrackerId);
        celix_arrayList_destroy(handler->interceptors);
        celixThreadMutex_destroy(&handler->lock);
        free((char*)handler->properties.scope);
        free((char*)handler->properties.topic);
        free((char*)handler->properties.psaType);
        free((char*)handler->properties.serializationType);
        free(handler);
    }
}

void pubsubInterceptorsHandler_addInterceptor(void *handle, void *svc, const celix_properties_t *props) {
    pubsub_interceptors_handler_t *handler = handle;

    celixThreadMutex_lock(&handler->lock);

    bool exists = false;
    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);
        if (entry->interceptor == svc) {
            exists = true;
        }
    }
    if (!exists) {
        entry_t *entry = calloc(1, sizeof(*entry));
        entry->properties = props;
        entry->interceptor = svc;
        celix_arrayList_add(handler->interceptors, entry);

        celix_arrayList_sort(handler->interceptors, referenceCompare);
    }

    celixThreadMutex_unlock(&handler->lock);
}

void pubsubInterceptorsHandler_removeInterceptor(void *handle, void *svc, __attribute__((unused)) const celix_properties_t *props) {
    pubsub_interceptors_handler_t *handler = handle;

    celixThreadMutex_lock(&handler->lock);

    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);
        if (entry->interceptor == svc) {
            celix_arrayList_removeAt(handler->interceptors, i);
            free(entry);
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);
}

bool pubsubInterceptorHandler_invokePreSend(pubsub_interceptors_handler_t *handler, const char *messageType, uint32_t messageId, const void *message, celix_properties_t **metadata) {
    bool cont = true;

    celixThreadMutex_lock(&handler->lock);

    if (*metadata == NULL && celix_arrayList_size(handler->interceptors) > 0) {
        *metadata = celix_properties_create();
    }

    for (uint32_t i = celix_arrayList_size(handler->interceptors); i > 0; i--) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i - 1);
        if (entry->interceptor->preSend != NULL) {
            cont = entry->interceptor->preSend(entry->interceptor->handle, &handler->properties, messageType, messageId, message, *metadata);
        }
        if (!cont) {
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);

    return cont;
}

void pubsubInterceptorHandler_invokePostSend(pubsub_interceptors_handler_t *handler, const char *messageType, uint32_t messageId, const void *message, celix_properties_t *metadata) {
    celixThreadMutex_lock(&handler->lock);

    for (uint32_t i = celix_arrayList_size(handler->interceptors); i > 0; i--) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i - 1);
        if (entry->interceptor->postSend != NULL) {
            entry->interceptor->postSend(entry->interceptor->handle, &handler->properties, messageType, messageId, message, metadata);
        }
    }

    celixThreadMutex_unlock(&handler->lock);
}

bool pubsubInterceptorHandler_invokePreReceive(pubsub_interceptors_handler_t *handler, const char *messageType, uint32_t messageId, const void *message, celix_properties_t **metadata) {
    bool cont = true;

    celixThreadMutex_lock(&handler->lock);
    if (*metadata == NULL && celix_arrayList_size(handler->interceptors) > 0) {
        *metadata = celix_properties_create();
    }
    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);
        if (entry->interceptor->preReceive != NULL) {
            cont = entry->interceptor->preReceive(entry->interceptor->handle, &handler->properties, messageType, messageId, message, *metadata);
        }
        if (!cont) {
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);

    return cont;
}

void pubsubInterceptorHandler_invokePostReceive(pubsub_interceptors_handler_t *handler, const char *messageType, uint32_t messageId, const void *message, celix_properties_t *metadata) {
    celixThreadMutex_lock(&handler->lock);

    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);
        if (entry->interceptor->postReceive != NULL) {
            entry->interceptor->postReceive(entry->interceptor->handle, &handler->properties, messageType, messageId, message, metadata);
        }
    }

    celixThreadMutex_unlock(&handler->lock);
}

int referenceCompare(const void *a, const void *b) {
    const entry_t *aEntry = a;
    const entry_t *bEntry = b;

    long servIdA = celix_properties_getAsLong(aEntry->properties, OSGI_FRAMEWORK_SERVICE_ID, 0);
    long servIdB = celix_properties_getAsLong(bEntry->properties, OSGI_FRAMEWORK_SERVICE_ID, 0);

    long servRankingA = celix_properties_getAsLong(aEntry->properties, OSGI_FRAMEWORK_SERVICE_RANKING, 0);
    long servRankingB = celix_properties_getAsLong(bEntry->properties, OSGI_FRAMEWORK_SERVICE_RANKING, 0);

    return celix_utils_compareServiceIdsAndRanking(servIdA, servRankingA, servIdB, servRankingB);
}

size_t pubsubInterceptorHandler_nrOfInterceptors(pubsub_interceptors_handler_t *handler) {
    size_t nr = 0;
    celixThreadMutex_lock(&handler->lock);
    nr = (size_t)celix_arrayList_size(handler->interceptors);
    celixThreadMutex_unlock(&handler->lock);
    return nr;
}