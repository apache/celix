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

/*!
 * @brief Allow error in seconds for the interval. This ensure pthread cond wakeups result in a call even if
 * the exact wakeupt time is a bit off.
 */
#define CELIX_SCHEDULED_EVENT_INTERVAL_ALLOW_ERROR_IN_SECONDS 0.000001

/**
 * @brief The timeout in seconds, before a log message is printed while waiting for a scheduled event to be processed.
 */
#define CELIX_SCHEDULED_EVENT_ERROR_LOG_TIMEOUT_IN_SECONDS 10.0


/**
 * @brief Default name for a scheduled event.
 */
static const char* const CELIX_SCHEDULED_EVENT_DEFAULT_NAME = "unnamed";

/**
 * @brief Struct representing a scheduled event.
 *
 * A scheduled event is an event that is scheduled to be executed at a certain ititial delay and/or interval.
 * It is created using the `celix_bundleContext_scheduleEvent` function and can be woken up
 * using the `celix_bundleContext_wakeupScheduledEvent` function.
 *
 * The struct contains information about the scheduled event, such as the event name, initial delay,
 * interval, and callback function. It also contains synchronization primitives to protect the use
 * count and call count of the event.
 *
 * @see celix_bundleContext_scheduleEvent
 * @see celix_bundleContext_wakeupScheduledEvent
 */
struct celix_scheduled_event {
    long scheduledEventId;            /**< The ID of the scheduled event. */
    celix_framework_logger_t* logger; /**< The framework logger used to log information */
    long bndId;                       /**< The bundle id for the bundle that owns the scheduled event. */
    char* eventName; /**< The name of the scheduled event. Will be CELIX_SCHEDULED_EVENT_DEFAULT_NAME if no name is
                        provided during creation. */
    double initialDelayInSeconds;                       /**< The initial delay of the scheduled event in seconds. */
    double intervalInSeconds;                           /**< The interval of the scheduled event in seconds. */
    void* callbackData;                                 /**< The data for the scheduled event callback. */
    void (*callback)(void* callbackData);               /**< The callback function for the scheduled event. */
    void* removedCallbackData;                          /**< The data for the scheduled event removed callback. */
    void (*removedCallback)(void* removedCallbackData); /**< The callback function for the scheduled event removed
                                                            callback. */

    celix_thread_mutex_t mutex; /**< The mutex to protect the data below. */
    celix_thread_cond_t cond;   /**< The condition variable to signal the scheduled event for a changed callCount and
                                   isRemoved. */
    size_t useCount;            /**< The use count of the scheduled event. */
    size_t callCount;           /**< The call count of the scheduled event. */
    bool isMarkedForRemoval;    /**< Whether the scheduled event is marked for removal. */
    bool isRemoved;             /**< Whether the scheduled event is removed. */
    struct timespec lastScheduledEventTime; /**< The last scheduled event time of the scheduled event. */
    bool processForWakeup; /**< Whether the scheduled event should be processed directly due to a wakeupScheduledEvent
                              call. */
};

celix_scheduled_event_t* celix_scheduledEvent_create(celix_framework_logger_t* logger,
                                                     long bndId,
                                                     long scheduledEventId,
                                                     const char* providedEventName,
                                                     double initialDelayInSeconds,
                                                     double intervalInSeconds,
                                                     void* callbackData,
                                                     void (*callback)(void* callbackData),
                                                     void* removedCallbackData,
                                                     void (*removedCallback)(void* removedCallbackData)) {
    celix_scheduled_event_t* event = malloc(sizeof(*event));
    char* eventName =
        providedEventName == NULL ? (char*)CELIX_SCHEDULED_EVENT_DEFAULT_NAME : celix_utils_strdup(providedEventName);
    if (event == NULL || eventName == NULL) {
        fw_log(logger,
               CELIX_LOG_LEVEL_ERROR,
               "Cannot add scheduled event for bundle id %li. Out of memory",
               bndId);
        free(event);
        if (eventName != CELIX_SCHEDULED_EVENT_DEFAULT_NAME) {
            free(eventName);
        }
        return NULL;
    }

    event->scheduledEventId = scheduledEventId;
    event->logger = logger;
    event->bndId = bndId;

    event->eventName = eventName;
    event->initialDelayInSeconds = initialDelayInSeconds;
    event->intervalInSeconds = intervalInSeconds;
    event->callbackData = callbackData;
    event->callback = callback;
    event->removedCallbackData = removedCallbackData;
    event->removedCallback = removedCallback;
    event->isMarkedForRemoval = false;
    event->useCount = 1;
    event->callCount = 0;
    event->isRemoved = false;
    clock_gettime(CLOCK_MONOTONIC, &event->lastScheduledEventTime);
    event->processForWakeup = false;

    celixThreadMutex_create(&event->mutex, NULL);
    celixThreadCondition_init(&event->cond, NULL);

    return event;
}

static void celix_scheduledEvent_destroy(celix_scheduled_event_t* event) {
    celixThreadMutex_destroy(&event->mutex);
    celixThreadCondition_destroy(&event->cond);
    if (event->eventName != CELIX_SCHEDULED_EVENT_DEFAULT_NAME) {
        free(event->eventName);
    }
    free(event);
}

void celix_scheduledEvent_retain(celix_scheduled_event_t* event) {
    if (event == NULL) {
        return;
    }

    celixThreadMutex_lock(&event->mutex);
    event->useCount += 1;
    celixThreadMutex_unlock(&event->mutex);
}

void celix_scheduledEvent_release(celix_scheduled_event_t* event) {
    if (event == NULL) {
        return;
    }

    celixThreadMutex_lock(&event->mutex);
    event->useCount -= 1;
    bool unused = event->useCount == 0;
    celixThreadMutex_unlock(&event->mutex);

    if (unused) {
        celix_scheduledEvent_destroy(event);
    }
}

const char* celix_scheduledEvent_getName(const celix_scheduled_event_t* event) { return event->eventName; }

long celix_scheduledEvent_getId(const celix_scheduled_event_t* event) { return event->scheduledEventId; }

long celix_scheduledEvent_getBundleId(const celix_scheduled_event_t* event) {
    return event->bndId;
}

bool celix_scheduledEvent_deadlineReached(celix_scheduled_event_t* event,
                                          const struct timespec* currentTime,
                                          double* nextProcessTimeInSeconds) {
    celixThreadMutex_lock(&event->mutex);
    double elapsed = celix_difftime(&event->lastScheduledEventTime, currentTime);
    double deadline = event->callCount == 0 ? event->initialDelayInSeconds : event->intervalInSeconds;
    deadline -= CELIX_SCHEDULED_EVENT_INTERVAL_ALLOW_ERROR_IN_SECONDS;
    bool deadlineReached = elapsed >= deadline;
    if (event->processForWakeup) {
        deadlineReached = true;
    }

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
    fw_log(event->logger,
           CELIX_LOG_LEVEL_TRACE,
           "Processing scheduled event %s for bundle id %li",
           event->eventName,
           event->bndId);
    assert(event->callback != NULL);

    event->callback(event->callbackData); // note called outside of lock

    celixThreadMutex_lock(&event->mutex);
    event->lastScheduledEventTime = *currentTime;
    event->callCount += 1;
    event->processForWakeup = false;
    celixThreadCondition_broadcast(&event->cond); //for changed callCount
    celixThreadMutex_unlock(&event->mutex);
}

bool celix_scheduledEvent_isSingleShot(celix_scheduled_event_t* event) {
    bool isDone = false;
    celixThreadMutex_lock(&event->mutex);
    isDone = event->intervalInSeconds == 0;
    celixThreadMutex_unlock(&event->mutex);
    return isDone;
}

size_t celix_scheduledEvent_markForWakeup(celix_scheduled_event_t* event) {
    celixThreadMutex_lock(&event->mutex);
    event->processForWakeup = true;
    size_t currentCallCount = event->callCount;
    celixThreadMutex_unlock(&event->mutex);

    fw_log(event->logger,
           CELIX_LOG_LEVEL_DEBUG,
           "Wakeup scheduled event '%s' (id=%li) for bundle id %li",
           event->eventName,
           event->scheduledEventId,
           event->bndId);

    return currentCallCount + 1;
}

celix_status_t celix_scheduledEvent_waitForAtLeastCallCount(celix_scheduled_event_t* event,
                                                            size_t targetCallCount,
                                                            double waitTimeInSeconds) {
    celix_status_t status = CELIX_SUCCESS;
    if (waitTimeInSeconds > 0) {
        struct timespec start = celix_gettime(CLOCK_MONOTONIC);
        struct timespec absTimeoutTime = celix_addDelayInSecondsToTime(&start, waitTimeInSeconds);
        celixThreadMutex_lock(&event->mutex);
        while (event->callCount < targetCallCount) {
            celixThreadCondition_waitUntil(&event->cond, &event->mutex, &absTimeoutTime);
            struct timespec now = celix_gettime(CLOCK_MONOTONIC);
            if (celix_difftime(&start, &now) > waitTimeInSeconds) {
                status = CELIX_TIMEOUT;
                break;
            }
        }
        celixThreadMutex_unlock(&event->mutex);
    }
    return status;
}

void celix_scheduledEvent_waitForRemoved(celix_scheduled_event_t* event) {
    struct timespec start = celix_gettime(CLOCK_MONOTONIC);
    struct timespec absLogTimeout = celix_addDelayInSecondsToTime(&start, CELIX_SCHEDULED_EVENT_ERROR_LOG_TIMEOUT_IN_SECONDS);
    celixThreadMutex_lock(&event->mutex);
    while (!event->isRemoved) {
        celix_status_t waitStatus = celixThreadCondition_waitUntil(&event->cond, &event->mutex, &absLogTimeout);
        if (waitStatus == ETIMEDOUT) {
            fw_log(event->logger,
                   CELIX_LOG_LEVEL_WARNING,
                   "Timeout while waiting for removal of scheduled event '%s' (id=%li) for bundle id %li.",
                   event->eventName,
                   event->scheduledEventId,
                   event->bndId);
            start = celix_gettime(CLOCK_MONOTONIC);
            absLogTimeout = celix_addDelayInSecondsToTime(&start, CELIX_SCHEDULED_EVENT_ERROR_LOG_TIMEOUT_IN_SECONDS);
        }
    }
    celixThreadMutex_unlock(&event->mutex);
}

celix_status_t celix_scheduledEvent_wait(celix_scheduled_event_t* event, double timeoutInSeconds) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&event->mutex);
    size_t targetCallCount = event->callCount + 1;
    struct timespec start = celix_gettime(CLOCK_MONOTONIC);
    struct timespec absTimeoutTime = celix_addDelayInSecondsToTime(&start, timeoutInSeconds);
    while (event->callCount < targetCallCount) {
        celix_status_t waitStatus = celixThreadCondition_waitUntil(&event->cond, &event->mutex, &absTimeoutTime);
        if (waitStatus == ETIMEDOUT) {
            status = CELIX_TIMEOUT;
            break;
        }
    }
    celixThreadMutex_unlock(&event->mutex);
    return status;
}

void celix_scheduledEvent_setRemoved(celix_scheduled_event_t* event) {
    if (event->removedCallback) {
        event->removedCallback(event->removedCallbackData);
    }
    celixThreadMutex_lock(&event->mutex);
    event->isRemoved = true;
    celixThreadCondition_broadcast(&event->cond); //for changed isRemoved
    celixThreadMutex_unlock(&event->mutex);
}

void celix_scheduledEvent_markForRemoval(celix_scheduled_event_t* event) {
    celixThreadMutex_lock(&event->mutex);
    event->isMarkedForRemoval = true;
    celixThreadMutex_unlock(&event->mutex);
}

bool celix_scheduledEvent_isMarkedForRemoval(celix_scheduled_event_t* event) {
    celixThreadMutex_lock(&event->mutex);
    bool isMarkedForRemoval = event->isMarkedForRemoval;
    celixThreadMutex_unlock(&event->mutex);
    return isMarkedForRemoval;
}
