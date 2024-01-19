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

#include "rsa_request_sender_tracker.h"
#include "rsa_request_sender_service.h"
#include "celix_stdlib_cleanup.h"
#include "celix_log_helper.h"
#include "celix_long_hash_map.h"
#include "celix_threads.h"
#include "celix_constants.h"
#include <stdlib.h>
#include <assert.h>

struct rsa_request_sender_tracker {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long reqSenderTrkId;
    celix_thread_rwlock_t lock;//projects below
    celix_long_hash_map_t *requestSenderSvcs;//Key:service id, Value:service
};

static void rsaRequestSenderTracker_addServiceWithProperties(void *handle, void *svc,
        const celix_properties_t *props);
static void rsaRequestSenderTracker_removeServiceWithProperties(void *handle, void *svc,
        const celix_properties_t *props);

celix_status_t rsaRequestSenderTracker_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper,
        rsa_request_sender_tracker_t **trackerOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (ctx == NULL || logHelper == NULL || trackerOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree rsa_request_sender_tracker_t *tracker = calloc(1, sizeof(*tracker));
    if (tracker == NULL) {
        return CELIX_ENOMEM;
    }
    tracker->ctx = ctx;
    tracker->logHelper = logHelper;
    status = celixThreadRwlock_create(&tracker->lock, NULL);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    celix_autoptr(celix_thread_rwlock_t) lock = &tracker->lock;
    celix_autoptr(celix_long_hash_map_t) requestSenderSvcs = tracker->requestSenderSvcs = celix_longHashMap_create();
    assert(tracker->requestSenderSvcs != NULL);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = CELIX_RSA_REQUEST_SENDER_SERVICE_NAME;
    opts.filter.versionRange = CELIX_RSA_REQUEST_SENDER_SERVICE_USE_RANGE;
    opts.callbackHandle = tracker;
    opts.addWithProperties = rsaRequestSenderTracker_addServiceWithProperties;
    opts.removeWithProperties = rsaRequestSenderTracker_removeServiceWithProperties;
    tracker->reqSenderTrkId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    if (tracker->reqSenderTrkId < 0) {
        celix_logHelper_error(tracker->logHelper, "Error tracking request sender service.");
        return CELIX_SERVICE_EXCEPTION;
    }
    celix_steal_ptr(requestSenderSvcs);
    celix_steal_ptr(lock);
    *trackerOut = celix_steal_ptr(tracker);
    return CELIX_SUCCESS;
}

static void rsaRequestSenderTracker_addServiceWithProperties(void *handle, void *svc,
        const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    rsa_request_sender_tracker_t *tracker = (rsa_request_sender_tracker_t *)handle;
    const char *serviceName = celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_NAME, "unknown-service");
    long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (svcId < 0) {
        celix_logHelper_error(tracker->logHelper, "Error getting rsa request sender service id for %s.", serviceName);
        return;
    }
    celixThreadRwlock_writeLock(&tracker->lock);
    (void)celix_longHashMap_put(tracker->requestSenderSvcs, svcId, svc);
    celixThreadRwlock_unlock(&tracker->lock);
    return;
}

static void rsaRequestSenderTracker_removeServiceWithProperties(void *handle, void *svc,
        const celix_properties_t *props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    rsa_request_sender_tracker_t *tracker = (rsa_request_sender_tracker_t *)handle;
    const char *serviceName = celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_NAME, "unknown-service");
    long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (svcId < 0) {
        celix_logHelper_error(tracker->logHelper, "Error getting rsa request sender service id for %s.", serviceName);
        return;
    }
    celixThreadRwlock_writeLock(&tracker->lock);
    (void)celix_longHashMap_remove(tracker->requestSenderSvcs, svcId);
    celixThreadRwlock_unlock(&tracker->lock);
    return;
}

static void rsaRequestSenderTracker_stopDone(void *data) {
    assert(data != NULL);
    rsa_request_sender_tracker_t *tracker = (rsa_request_sender_tracker_t *)data;
    assert(celix_longHashMap_size(tracker->requestSenderSvcs) == 0);
    celix_longHashMap_destroy(tracker->requestSenderSvcs);
    (void)celixThreadRwlock_destroy(&tracker->lock);
    free(tracker);
    return;
}

void rsaRequestSenderTracker_destroy(rsa_request_sender_tracker_t *tracker) {
    if (tracker != NULL) {
        celix_bundleContext_stopTrackerAsync(tracker->ctx, tracker->reqSenderTrkId, tracker, rsaRequestSenderTracker_stopDone);
    }
    return;
}

celix_status_t rsaRequestSenderTracker_useService(rsa_request_sender_tracker_t *tracker,
        long reqSenderSvcId, void *handle, celix_status_t (*use)(void *handle, rsa_request_sender_service_t *svc)) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&tracker->lock);
    rsa_request_sender_service_t *svc = celix_longHashMap_get(tracker->requestSenderSvcs, reqSenderSvcId);
    if (svc != NULL) {
        status = use(handle, svc);
    } else {
        status = CELIX_ILLEGAL_STATE;
    }
    celixThreadRwlock_unlock(&tracker->lock);
    return status;
}


