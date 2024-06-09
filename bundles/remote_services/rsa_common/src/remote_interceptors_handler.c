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

#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_compiler.h"
#include "celix_array_list.h"
#include "celix_threads.h"
#include "celix_utils.h"

#include "remote_interceptors_handler.h"

typedef struct entry {
    const celix_properties_t *properties;
    remote_interceptor_t *interceptor;
} entry_t;

struct remote_interceptors_handler {
    celix_array_list_t *interceptors;

    long interceptorsTrackerId;

    celix_bundle_context_t *ctx;

    celix_thread_mutex_t lock;
};

static int referenceCompare(celix_array_list_entry_t a, celix_array_list_entry_t b);

static void remoteInterceptorsHandler_addInterceptor(void *handle, void *svc, const celix_properties_t *props);
static void remoteInterceptorsHandler_removeInterceptor(void *handle, void *svc, const celix_properties_t *props);

celix_status_t remoteInterceptorsHandler_create(celix_bundle_context_t *ctx, remote_interceptors_handler_t **handler) {
    celix_status_t status = CELIX_SUCCESS;

    *handler = calloc(1, sizeof(**handler));
    if (!*handler) {
        status = CELIX_ENOMEM;
    } else {
        (*handler)->ctx = ctx;

        (*handler)->interceptors = celix_arrayList_create();

        status = celixThreadMutex_create(&(*handler)->lock, NULL);

        if (status == CELIX_SUCCESS) {
            // Create service tracker here, and not in the activator
            celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
            opts.filter.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
            opts.callbackHandle = *handler;
            opts.addWithProperties = remoteInterceptorsHandler_addInterceptor;
            opts.removeWithProperties = remoteInterceptorsHandler_removeInterceptor;
            (*handler)->interceptorsTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
        }
    }

    return status;
}

static void remoteInterceptorsHandler_destroyCallback(void* data) {
    remote_interceptors_handler_t *handler = data;
    celix_arrayList_destroy(handler->interceptors);
    celixThreadMutex_destroy(&handler->lock);
    free(handler);
}

celix_status_t remoteInterceptorsHandler_destroy(remote_interceptors_handler_t *handler) {
    celix_bundleContext_stopTrackerAsync(handler->ctx, handler->interceptorsTrackerId, handler, remoteInterceptorsHandler_destroyCallback);
    return CELIX_SUCCESS;
}

void remoteInterceptorsHandler_addInterceptor(void *handle, void *svc, const celix_properties_t *props) {
    remote_interceptors_handler_t *handler = handle;

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

        celix_arrayList_sortEntries(handler->interceptors, referenceCompare);
    }

    celixThreadMutex_unlock(&handler->lock);
}

void remoteInterceptorsHandler_removeInterceptor(void *handle, void *svc, const celix_properties_t *props CELIX_UNUSED) {
    remote_interceptors_handler_t *handler = handle;

    celixThreadMutex_lock(&handler->lock);

    for (int i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);
        if (entry->interceptor == svc) {
            celix_arrayList_removeAt(handler->interceptors, i);
            free(entry);
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);
}

bool remoteInterceptorHandler_invokePreExportCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t **metadata) {
    bool cont = true;

    celixThreadMutex_lock(&handler->lock);

    if (*metadata == NULL && celix_arrayList_size(handler->interceptors) > 0) {
        *metadata = celix_properties_create();
    }

    for (uint32_t i = celix_arrayList_size(handler->interceptors); i > 0; i--) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i - 1);

        cont = entry->interceptor->preExportCall(entry->interceptor->handle, svcProperties, functionName, *metadata);
        if (!cont) {
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);

    return cont;
}

void remoteInterceptorHandler_invokePostExportCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    celixThreadMutex_lock(&handler->lock);

    for (uint32_t i = celix_arrayList_size(handler->interceptors); i > 0; i--) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i - 1);

        entry->interceptor->postExportCall(entry->interceptor->handle, svcProperties, functionName, metadata);
    }

    celixThreadMutex_unlock(&handler->lock);
}

bool remoteInterceptorHandler_invokePreProxyCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t **metadata) {
    bool cont = true;

    celixThreadMutex_lock(&handler->lock);

    if (*metadata == NULL && celix_arrayList_size(handler->interceptors) > 0) {
        *metadata = celix_properties_create();
    }

    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);

        cont = entry->interceptor->preProxyCall(entry->interceptor->handle, svcProperties, functionName, *metadata);
        if (!cont) {
            break;
        }
    }

    celixThreadMutex_unlock(&handler->lock);

    return cont;
}

void remoteInterceptorHandler_invokePostProxyCall(remote_interceptors_handler_t *handler, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
    celixThreadMutex_lock(&handler->lock);

    for (uint32_t i = 0; i < celix_arrayList_size(handler->interceptors); i++) {
        entry_t *entry = celix_arrayList_get(handler->interceptors, i);

        entry->interceptor->postProxyCall(entry->interceptor->handle, svcProperties, functionName, metadata);
    }

    celixThreadMutex_unlock(&handler->lock);
}

int referenceCompare(celix_array_list_entry_t a, celix_array_list_entry_t b) {
    const entry_t *aEntry = a.voidPtrVal;
    const entry_t *bEntry = b.voidPtrVal;

    long servIdA = celix_properties_getAsLong(aEntry->properties, CELIX_FRAMEWORK_SERVICE_ID, 0);
    long servIdB = celix_properties_getAsLong(bEntry->properties, CELIX_FRAMEWORK_SERVICE_ID, 0);

    long servRankingA = celix_properties_getAsLong(aEntry->properties, CELIX_FRAMEWORK_SERVICE_RANKING, 0);
    long servRankingB = celix_properties_getAsLong(bEntry->properties, CELIX_FRAMEWORK_SERVICE_RANKING, 0);

    return celix_utils_compareServiceIdsAndRanking(servIdA, servRankingA, servIdB, servRankingB);
}
