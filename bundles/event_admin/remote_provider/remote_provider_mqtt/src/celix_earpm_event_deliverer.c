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
#include "celix_earpm_event_deliverer.h"

#include <stdlib.h>
#include <assert.h>

#include "celix_threads.h"
#include "celix_array_list.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_earpm_constants.h"

#define CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_MAX 20

typedef struct celix_earpm_deliverer_event_entry {
    celix_properties_t* properties;
    char* topic;
    celix_earpm_deliver_done_callback done;
    void* doneHandle;
} celix_earpm_deliverer_event_entry_t;


struct celix_earpm_event_deliverer {
    celix_bundle_context_t* ctx;
    celix_log_helper_t *logHelper;

    celix_thread_rwlock_t eaLock;//protects eventAdminSvc
    celix_event_admin_service_t* eventAdminSvc;
    long syncEventDeliveryThreadsNr;
    long syncEventQueueSizeMax;
    celix_thread_mutex_t mutex;//protects belows
    celix_thread_t syncEventDeliveryThreads[CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_MAX];
    celix_thread_cond_t hasSyncEventsOrExiting;
    celix_array_list_t* syncEventQueue;//element = celix_earpmd_event_entry_t*
    bool syncEventDeliveryThreadRunning;
};

static void* celix_earpmDeliverer_syncEventDeliveryThread(void* data);

celix_earpm_event_deliverer_t* celix_earpmDeliverer_create(celix_bundle_context_t* ctx, celix_log_helper_t* logHelper) {
    assert(ctx != NULL);
    assert(logHelper != NULL);
    long syncEventDeliveryThreadsNr = celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS,
                                                                            CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_DEFAULT);
    if (syncEventDeliveryThreadsNr <= 0 || syncEventDeliveryThreadsNr > CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_MAX) {
        celix_logHelper_error(logHelper, "Invalid number of sync event delivery threads %ld, must be in range 1-%d.", syncEventDeliveryThreadsNr, CELIX_EARPM_SYNC_EVENT_DELIVERY_THREADS_MAX);
        return NULL;
    }
    long syncEventQueueCap = celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_MSG_QUEUE_CAPACITY, CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT);
    if (syncEventQueueCap <= 0 || syncEventQueueCap > CELIX_EARPM_MSG_QUEUE_MAX_SIZE) {
        celix_logHelper_error(logHelper, "Invalid sync event queue capacity %ld, must be in range 1-%d.", syncEventQueueCap, CELIX_EARPM_MSG_QUEUE_MAX_SIZE);
        return NULL;
    }
    celix_autofree celix_earpm_event_deliverer_t* deliverer = calloc(1, sizeof(*deliverer));
    if (deliverer == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for event deliverer.");
        return NULL;
    }
    deliverer->ctx = ctx;
    deliverer->logHelper = logHelper;
    deliverer->eventAdminSvc = NULL;
    deliverer->syncEventDeliveryThreadsNr = syncEventDeliveryThreadsNr;
    deliverer->syncEventQueueSizeMax = syncEventQueueCap;
    celix_status_t status = celixThreadRwlock_create(&deliverer->eaLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event admin rwlock for event deliverer. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_rwlock_t) eaLock = &deliverer->eaLock;
    status = celixThreadMutex_create(&deliverer->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer mutex. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &deliverer->mutex;
    status = celixThreadCondition_init(&deliverer->hasSyncEventsOrExiting, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer condition. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) cond = &deliverer->hasSyncEventsOrExiting;
    celix_autoptr(celix_array_list_t) syncEventQueue = deliverer->syncEventQueue = celix_arrayList_createPointerArray();
    if (syncEventQueue == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create sync event queue.");
        return NULL;
    }

    deliverer->syncEventDeliveryThreadRunning = true;
    for (int i = 0; i < syncEventDeliveryThreadsNr; ++i) {
        status = celixThread_create(&deliverer->syncEventDeliveryThreads[i], NULL,
                                    celix_earpmDeliverer_syncEventDeliveryThread, deliverer);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(logHelper, "Failed to create sync event delivery thread %d. %d.", i, status);
            for (int j = 0; j < i; ++j) {
                celixThreadMutex_lock(mutex);
                deliverer->syncEventDeliveryThreadRunning = false;
                celixThreadMutex_unlock(mutex);
                celixThreadCondition_broadcast(&deliverer->hasSyncEventsOrExiting);
                celixThread_join(deliverer->syncEventDeliveryThreads[j], NULL);
            }
            return NULL;
        }
        char threadName[32];
        snprintf(threadName, 32, "CelixEarpmED%d", i);
        celixThread_setName(&deliverer->syncEventDeliveryThreads[i], threadName);
    }

    celix_steal_ptr(syncEventQueue);
    celix_steal_ptr(cond);
    celix_steal_ptr(mutex);
    celix_steal_ptr(eaLock);

    return celix_steal_ptr(deliverer);
}

void celix_earpmDeliverer_destroy(celix_earpm_event_deliverer_t* deliverer) {
    assert(deliverer != NULL);
    celixThreadMutex_lock(&deliverer->mutex);
    deliverer->syncEventDeliveryThreadRunning = false;
    celixThreadMutex_unlock(&deliverer->mutex);
    celixThreadCondition_broadcast(&deliverer->hasSyncEventsOrExiting);
    for (int i = 0; i < deliverer->syncEventDeliveryThreadsNr; ++i) {
        celixThread_join(deliverer->syncEventDeliveryThreads[i], NULL);
    }
    int size = celix_arrayList_size(deliverer->syncEventQueue);
    for (int i = 0; i < size; ++i) {
        celix_earpm_deliverer_event_entry_t* entry = (celix_earpm_deliverer_event_entry_t*)celix_arrayList_get(deliverer->syncEventQueue, i);
        if (entry->done != NULL) {
            entry->done(entry->doneHandle, entry->topic, CELIX_ILLEGAL_STATE);
        }
        celix_properties_destroy(entry->properties);
        free(entry->topic);
        free(entry);
    }
    celix_arrayList_destroy(deliverer->syncEventQueue);
    celixThreadCondition_destroy(&deliverer->hasSyncEventsOrExiting);
    celixThreadMutex_destroy(&deliverer->mutex);
    celixThreadRwlock_destroy(&deliverer->eaLock);
    free(deliverer);
    return;
}

celix_status_t celix_earpmDeliverer_setEventAdminSvc(celix_earpm_event_deliverer_t* deliverer, celix_event_admin_service_t *eventAdminSvc) {
    celix_auto(celix_rwlock_wlock_guard_t) wLockGuard = celixRwlockWlockGuard_init(&deliverer->eaLock);
    deliverer->eventAdminSvc = eventAdminSvc;
    return CELIX_SUCCESS;
}

celix_status_t celix_earpmDeliverer_postEvent(celix_earpm_event_deliverer_t* deliverer, const char* topic, celix_properties_t* properties) {
    assert(deliverer != NULL);
    assert(topic != NULL);
    celix_autoptr(celix_properties_t) _properties = properties;
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&deliverer->eaLock);
    celix_event_admin_service_t* eventAdminSvc = deliverer->eventAdminSvc;
    if (eventAdminSvc == NULL) {
        celix_logHelper_warning(deliverer->logHelper, "No event admin service available, drop event %s.", topic);
        return CELIX_ILLEGAL_STATE;
    }
    celix_status_t status = eventAdminSvc->postEvent(eventAdminSvc->handle, topic, _properties);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(deliverer->logHelper, "Failed to post event %s. %d.", topic, status);
    }
    return status;
}

celix_status_t celix_earpmDeliverer_sendEvent(celix_earpm_event_deliverer_t* deliverer, const char* topic, celix_properties_t* properties, celix_earpm_deliver_done_callback done, void* callbackData) {
    assert(deliverer != NULL);
    assert(topic != NULL);
    celix_autoptr(celix_properties_t) _properties = properties;
    celix_autofree celix_earpm_deliverer_event_entry_t* entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_logHelper_error(deliverer->logHelper, "Failed to allocate memory for event entry for %s.", topic);
        return ENOMEM;
    }
    celix_autofree char* _topic = entry->topic = celix_utils_strdup(topic);
    if (entry->topic == NULL) {
        celix_logHelper_error(deliverer->logHelper, "Failed to duplicate topic %s.", topic);
        return ENOMEM;
    }
    entry->properties = properties;
    entry->done = done;
    entry->doneHandle = callbackData;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&deliverer->mutex);
        if (celix_arrayList_size(deliverer->syncEventQueue) >= deliverer->syncEventQueueSizeMax) {
            celix_logHelper_error(deliverer->logHelper, "Sync event queue full, drop event %s.", topic);
            return ENOMEM;
        }
        celix_status_t status = celix_arrayList_add(deliverer->syncEventQueue, entry);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(deliverer->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(deliverer->logHelper, "Failed to add event to sync event queue for %s. %d.", topic, status);
            return ENOMEM;
        }
    }
    celix_steal_ptr(_properties);
    celix_steal_ptr(_topic);
    celix_steal_ptr(entry);
    celixThreadCondition_signal(&deliverer->hasSyncEventsOrExiting);

    return CELIX_SUCCESS;
}

static celix_status_t celix_earpmDeliverer_sendEventToEventAdmin(celix_earpm_event_deliverer_t* deliverer, celix_earpm_deliverer_event_entry_t* eventEntry) {
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&deliverer->eaLock);
    celix_event_admin_service_t* eventAdminSvc = deliverer->eventAdminSvc;
    if (eventAdminSvc == NULL) {
        celix_logHelper_warning(deliverer->logHelper, "No event admin service available, drop event %s.", eventEntry->topic);
        return CELIX_ILLEGAL_STATE;
    }
    celix_status_t status = eventAdminSvc->sendEvent(eventAdminSvc->handle, eventEntry->topic, eventEntry->properties);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(deliverer->logHelper, "Failed to send event %s. %d.", eventEntry->topic, status);
    }
    return status;
}

static void* celix_earpmDeliverer_syncEventDeliveryThread(void* data) {
    assert(data != NULL);
    celix_earpm_event_deliverer_t* deliverer = (celix_earpm_event_deliverer_t*)data;
    celixThreadMutex_lock(&deliverer->mutex);
    bool running = deliverer->syncEventDeliveryThreadRunning;
    celixThreadMutex_unlock(&deliverer->mutex);
    while (running) {
        celix_earpm_deliverer_event_entry_t* entry = NULL;
        celixThreadMutex_lock(&deliverer->mutex);
        while (deliverer->syncEventDeliveryThreadRunning && celix_arrayList_size(deliverer->syncEventQueue) == 0) {
            celixThreadCondition_wait(&deliverer->hasSyncEventsOrExiting, &deliverer->mutex);
        }
        running = deliverer->syncEventDeliveryThreadRunning;
        if (running) {
            entry = celix_arrayList_get(deliverer->syncEventQueue, 0);
            celix_arrayList_removeAt(deliverer->syncEventQueue, 0);
        }
        celixThreadMutex_unlock(&deliverer->mutex);
        if (entry != NULL) {
            celix_status_t status = celix_earpmDeliverer_sendEventToEventAdmin(deliverer, entry);
            if (entry->done != NULL) {
                entry->done(entry->doneHandle, entry->topic, status);
            }
            celix_properties_destroy(entry->properties);
            free(entry->topic);
            free(entry);
        }
    }
    return NULL;
}