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
}celix_earpm_deliverer_event_entry_t;


struct celix_earpm_event_deliverer {
    celix_bundle_context_t* ctx;
    celix_log_helper_t *logHelper;

    celix_thread_rwlock_t eaLock;
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
        return NULL;
    }
    long syncEventQueueCap = celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_MSG_QUEUE_CAPACITY, CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT);
    if (syncEventQueueCap <= 0 || syncEventQueueCap > CELIX_EARPM_MSG_QUEUE_MAX_SIZE) {
        return NULL;
    }
    celix_autofree celix_earpm_event_deliverer_t* earpmd = calloc(1, sizeof(*earpmd));
    if (earpmd == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for event deliverer.");
        return NULL;
    }
    earpmd->ctx = ctx;
    earpmd->logHelper = logHelper;
    earpmd->eventAdminSvc = NULL;
    earpmd->syncEventDeliveryThreadsNr = syncEventDeliveryThreadsNr;
    earpmd->syncEventQueueSizeMax = syncEventQueueCap;
    celix_status_t status = celixThreadRwlock_create(&earpmd->eaLock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event admin rwlock for event deliverer. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_rwlock_t) eaLock = &earpmd->eaLock;
    status = celixThreadMutex_create(&earpmd->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer mutex. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &earpmd->mutex;
    status = celixThreadCondition_init(&earpmd->hasSyncEventsOrExiting, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer condition. %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) cond = &earpmd->hasSyncEventsOrExiting;
    celix_autoptr(celix_array_list_t) syncEventQueue = earpmd->syncEventQueue = celix_arrayList_createPointerArray();
    if (syncEventQueue == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create sync event queue.");
        return NULL;
    }

    earpmd->syncEventDeliveryThreadRunning = true;
    for (int i = 0; i < syncEventDeliveryThreadsNr; ++i) {
        status = celixThread_create(&earpmd->syncEventDeliveryThreads[i], NULL,
                                    celix_earpmDeliverer_syncEventDeliveryThread, earpmd);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(logHelper, "Failed to create sync event delivery thread %d. %d.", i, status);
            for (int j = 0; j < i; ++j) {
                celixThreadMutex_lock(mutex);
                earpmd->syncEventDeliveryThreadRunning = false;
                celixThreadMutex_unlock(mutex);
                celixThreadCondition_broadcast(&earpmd->hasSyncEventsOrExiting);
                celixThread_join(earpmd->syncEventDeliveryThreads[j], NULL);
            }
            return NULL;
        }
        char threadName[32];
        snprintf(threadName, 32, "earpm_evt_th%d", i);
        celixThread_setName(&earpmd->syncEventDeliveryThreads[i], threadName);
    }

    celix_steal_ptr(syncEventQueue);
    celix_steal_ptr(cond);
    celix_steal_ptr(mutex);
    celix_steal_ptr(eaLock);

    return celix_steal_ptr(earpmd);
}

void celix_earpmDeliverer_destroy(celix_earpm_event_deliverer_t* earpmd) {
    assert(earpmd != NULL);
    celixThreadMutex_lock(&earpmd->mutex);
    earpmd->syncEventDeliveryThreadRunning = false;
    celixThreadMutex_unlock(&earpmd->mutex);
    celixThreadCondition_broadcast(&earpmd->hasSyncEventsOrExiting);
    for (int i = 0; i < earpmd->syncEventDeliveryThreadsNr; ++i) {
        celixThread_join(earpmd->syncEventDeliveryThreads[i], NULL);
    }
    int size = celix_arrayList_size(earpmd->syncEventQueue);
    for (int i = 0; i < size; ++i) {
        celix_earpm_deliverer_event_entry_t* entry = (celix_earpm_deliverer_event_entry_t*)celix_arrayList_get(earpmd->syncEventQueue, i);
        if (entry->done != NULL) {
            entry->done(entry->doneHandle, entry->topic, CELIX_ILLEGAL_STATE);
        }
        celix_properties_destroy(entry->properties);
        free(entry->topic);
        free(entry);
    }
    celix_arrayList_destroy(earpmd->syncEventQueue);
    celixThreadCondition_destroy(&earpmd->hasSyncEventsOrExiting);
    celixThreadMutex_destroy(&earpmd->mutex);
    celixThreadRwlock_destroy(&earpmd->eaLock);
    free(earpmd);
    return;
}

celix_status_t celix_earpmDeliverer_setEventAdminSvc(celix_earpm_event_deliverer_t* earpmd, celix_event_admin_service_t *eventAdminSvc) {
    celix_auto(celix_rwlock_wlock_guard_t) wLockGuard = celixRwlockWlockGuard_init(&earpmd->eaLock);
    earpmd->eventAdminSvc = eventAdminSvc;
    return CELIX_SUCCESS;
}

celix_status_t celix_earpmDeliverer_postEvent(celix_earpm_event_deliverer_t* earpmd, const char* topic, celix_properties_t* properties) {
    assert(earpmd != NULL);
    assert(topic != NULL);
    celix_autoptr(celix_properties_t) _properties = properties;
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&earpmd->eaLock);
    celix_event_admin_service_t* eventAdminSvc = earpmd->eventAdminSvc;
    if (eventAdminSvc == NULL) {
        celix_logHelper_warning(earpmd->logHelper, "No event admin service available, drop event %s.", topic);
        return CELIX_ILLEGAL_STATE;
    }
    celix_status_t status = eventAdminSvc->postEvent(eventAdminSvc->handle, topic, _properties);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpmd->logHelper, "Failed to post event %s. %d.", topic, status);
    }
    return status;
}

celix_status_t celix_earpmDeliverer_sendEvent(celix_earpm_event_deliverer_t* earpmd, const char* topic, celix_properties_t* properties, celix_earpm_deliver_done_callback done, void* callbackData) {
    assert(earpmd != NULL);
    assert(topic != NULL);
    celix_autoptr(celix_properties_t) _properties = properties;
    celix_autofree celix_earpm_deliverer_event_entry_t* entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_logHelper_error(earpmd->logHelper, "Failed to allocate memory for event entry for %s.", topic);
        return CELIX_ENOMEM;
    }
    entry->topic = celix_utils_strdup(topic);
    if (entry->topic == NULL) {
        celix_logHelper_error(earpmd->logHelper, "Failed to duplicate topic %s.", topic);
        return CELIX_ENOMEM;
    }
    entry->properties = properties;
    entry->done = done;
    entry->doneHandle = callbackData;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpmd->mutex);
        if (celix_arrayList_size(earpmd->syncEventQueue) >= earpmd->syncEventQueueSizeMax) {
            celix_logHelper_error(earpmd->logHelper, "Sync event queue full, drop event %s.", topic);
            return CELIX_ENOMEM;
        }
        celix_status_t status = celix_arrayList_add(earpmd->syncEventQueue, entry);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(earpmd->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpmd->logHelper, "Failed to add event to sync event queue for %s. %d.", topic, status);
            return CELIX_ENOMEM;
        }
    }
    celix_steal_ptr(_properties);
    celix_steal_ptr(entry);
    celixThreadCondition_signal(&earpmd->hasSyncEventsOrExiting);

    return CELIX_SUCCESS;
}

static celix_status_t celix_earpmDeliverer_sendEventToEventAdmin(celix_earpm_event_deliverer_t* earpmd, celix_earpm_deliverer_event_entry_t* eventEntry) {
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&earpmd->eaLock);
    celix_event_admin_service_t* eventAdminSvc = earpmd->eventAdminSvc;
    if (eventAdminSvc == NULL) {
        celix_logHelper_warning(earpmd->logHelper, "No event admin service available, drop event %s.", eventEntry->topic);
        return CELIX_ILLEGAL_STATE;
    }
    celix_status_t status = eventAdminSvc->sendEvent(eventAdminSvc->handle, eventEntry->topic, eventEntry->properties);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpmd->logHelper, "Failed to send event %s. %d.", eventEntry->topic, status);
    }
    return status;
}

static void* celix_earpmDeliverer_syncEventDeliveryThread(void* data) {
    assert(data != NULL);
    celix_earpm_event_deliverer_t* earpmd = data;
    celixThreadMutex_lock(&earpmd->mutex);
    bool running = earpmd->syncEventDeliveryThreadRunning;
    celixThreadMutex_unlock(&earpmd->mutex);
    while (running) {
        celix_earpm_deliverer_event_entry_t* entry = NULL;
        celixThreadMutex_lock(&earpmd->mutex);
        while (earpmd->syncEventDeliveryThreadRunning && celix_arrayList_size(earpmd->syncEventQueue) == 0) {
            celixThreadCondition_wait(&earpmd->hasSyncEventsOrExiting, &earpmd->mutex);
        }
        running = earpmd->syncEventDeliveryThreadRunning;
        if (running) {
            entry = celix_arrayList_get(earpmd->syncEventQueue, 0);
            celix_arrayList_removeAt(earpmd->syncEventQueue, 0);
        }
        celixThreadMutex_unlock(&earpmd->mutex);
        if (entry != NULL) {
            celix_status_t status = celix_earpmDeliverer_sendEventToEventAdmin(earpmd, entry);
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