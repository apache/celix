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

#include <assert.h>

#include "celix_scheduled_event.h"
#include "celix_utils.h"

celix_scheduled_event_t* celix_scheduledEvent_create(celix_framework_logger_t* logger,
                                                     celix_framework_bundle_entry_t* bndEntry,
                                                     long scheduledEventId,
                                                     const char* providedEventName,
                                                     double initialDelayInSeconds,
                                                     double intervalInSeconds,
                                                     void* eventData,
                                                     void (*eventCallback)(void* eventData)) {
    celix_scheduled_event_t* event = malloc(sizeof(*event));
    char* eventName = providedEventName == NULL ? (char*)CELIX_SCHEDULED_EVENT_DEFAULT_NAME
                                                 : celix_utils_strdup(providedEventName);
    if (event == NULL || eventName == NULL) {
        fw_log(logger, CELIX_LOG_LEVEL_ERROR, "Cannot add scheduled event for bundle id %li. Out of memory", bndEntry->bndId);
        free(event);
        if (eventName != CELIX_SCHEDULED_EVENT_DEFAULT_NAME) {
            free(eventName);
        }
        celix_framework_bundleEntry_decreaseUseCount(bndEntry);
        return NULL;
    }

    event->scheduledEventId = scheduledEventId;
    event->logger = logger;
    event->bndEntry = bndEntry;

    event->eventName = eventName;
    event->initialDelayInSeconds = initialDelayInSeconds;
    event->intervalInSeconds = intervalInSeconds;
    event->eventCallbackData = eventData;
    event->eventCallback = eventCallback;
    event->useCount = 0;
    event->callCount = 0;
    clock_gettime(CLOCK_MONOTONIC, &event->lastScheduledEventTime);
    celixThreadMutex_create(&event->mutex, NULL);
    celixThreadCondition_init(&event->cond, NULL);

    return event;
}

void celix_scheduledEvent_destroy(celix_scheduled_event_t* event) {
    if (event != NULL) {
        celix_framework_bundleEntry_decreaseUseCount(event->bndEntry);
        celixThreadMutex_destroy(&event->mutex);
        celixThreadCondition_destroy(&event->cond);
        if (event->eventName != CELIX_SCHEDULED_EVENT_DEFAULT_NAME) {
            free(event->eventName);
        }
        free(event);
    }
}

void celix_scheduledEvent_waitAndDestroy(celix_scheduled_event_t* event) {
    if (event == NULL) {
        return;
    }

    //wait for use count 0;
    celixThreadMutex_lock(&event->mutex);
    while (event->useCount) {
        celixThreadCondition_wait(&event->cond, &event->mutex);
    }
    celixThreadMutex_unlock(&event->mutex);

    celix_scheduledEvent_destroy(event);
}

bool celix_scheduledEvent_deadlineReached(celix_scheduled_event_t* event,
                                          const struct timespec* currentTime,
                                          double* nextProcessTimeInSeconds) {
    celixThreadMutex_lock(&event->mutex);
    double elapsed = celix_difftime(&event->lastScheduledEventTime, currentTime);
    double deadline = event->callCount == 0 ? event->initialDelayInSeconds : event->intervalInSeconds;
    deadline -= CELIX_SCHEDULED_EVENT_INTERVAL_ALLOW_ERROR_IN_SECONDS;
    bool deadlineReached = elapsed >= deadline;
    if (deadlineReached) {
        *nextProcessTimeInSeconds =
            event->intervalInSeconds == 0 /*one shot*/ ? CELIX_FRAMEWORK_DEFAULT_MAX_TIMEDWAIT_EVENT_HANDLER_IN_SECONDS
                                                       : event->intervalInSeconds;
    } else {
        *nextProcessTimeInSeconds = event->callCount == 0 ? event->initialDelayInSeconds : event->intervalInSeconds;
    }
    celixThreadMutex_unlock(&event->mutex);
    return deadlineReached;
}

void celix_scheduledEvent_process(celix_scheduled_event_t* event, const struct timespec* currentTime) {
    void (*callback)(void*) = NULL;
    void* callbackData = NULL;

    celixThreadMutex_lock(&event->mutex);
    callback = event->eventCallback;
    callbackData = event->eventCallbackData;
    event->useCount += 1;
    celixThreadCondition_broadcast(&event->cond); //broadcast for changed useCount
    celixThreadMutex_unlock(&event->mutex);
    assert(callback != NULL);

    callback(callbackData); //note called outside of lock

    celixThreadMutex_lock(&event->mutex);
    event->lastScheduledEventTime = *currentTime;
    event->useCount -= 1;
    event->callCount += 1;
    celixThreadCondition_broadcast(&event->cond); //broadcast for changed useCount
    celixThreadMutex_unlock(&event->mutex);
}


bool celix_scheduleEvent_isDone(celix_scheduled_event_t* event) {
    bool isDone = false;
    celixThreadMutex_lock(&event->mutex);
    isDone = event->intervalInSeconds == 0 && event->callCount > 0;
    celixThreadMutex_unlock(&event->mutex);
    return isDone;
}
