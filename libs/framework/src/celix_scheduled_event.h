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

#ifndef CELIX_CELIX_SCHEDULED_EVENT_H
#define CELIX_CELIX_SCHEDULED_EVENT_H

#include "celix_bundle_context.h"
#include "framework_private.h"

#ifdef __cplusplus
extern "C" {
#endif

//Allow, 1 microsecond error in interval to ensure pthread cond wakeups result in a call.
#define CELIX_SCHEDULED_EVENT_INTERVAL_ALLOW_ERROR_IN_SECONDS 0.000001

static const char* const CELIX_SCHEDULED_EVENT_DEFAULT_NAME = "unnamed";

typedef struct celix_scheduled_event {
    long scheduledEventId;
    celix_framework_logger_t* logger;
    celix_framework_bundle_entry_t* bndEntry;

    char* eventName;
    double initialDelayInSeconds;
    double intervalInSeconds;
    void* eventCallbackData;
    void (*eventCallback)(void* eventData);

    celix_thread_mutex_t mutex; //protects below
    celix_thread_cond_t cond;
    size_t useCount; //use count, how many times the event is used to process a eventCallback or doneCallback.
    size_t callCount; //nr of times the eventCallback is called
    struct timespec lastScheduledEventTime;
} celix_scheduled_event_t;

/**
 * @brief Create a scheduled event for the given bundle.
 * @param[in] bndEntry The bundle entry for which the scheduled event is created.
 * @param[in] scheduledEventId The id of the scheduled event.
 * @param[in] eventName The name of the event. If NULL, CELIX_SCHEDULED_EVENT_DEFAULT_NAME is used.
 * @param[in] initialDelayInSeconds The initial delay in seconds.
 * @param[in] intervalInSeconds The interval in seconds.
 * @param[in] eventData The event data.
 * @param[in] eventCallback The event callback.
 * @return A new scheduled event or NULL if failed.
 */
celix_scheduled_event_t* celix_scheduledEvent_create(celix_framework_logger_t* logger,
                                                     celix_framework_bundle_entry_t* bndEntry,
                                                     long scheduledEventId,
                                                     const char* eventName,
                                                     double initialDelayInSeconds,
                                                     double intervalInSeconds,
                                                     void* eventData,
                                                     void (*eventCallback)(void* eventData));

/**
 * @brief Destroy the event.
 */
void celix_scheduledEvent_destroy(celix_scheduled_event_t* event);

/**
 * @brief Wait until the useCount is 0 and destroy the event.
 */
void celix_scheduledEvent_waitAndDestroy(celix_scheduled_event_t* event);

/**
 * @brief Returns whether the event deadline is reached and the event should be processed.
 * @param[in] event The event to check.
 * @param[in] currentTime The current time.
 * @param[out] nextProcessTimeInSeconds The time in seconds until the next event should be processed.
 *                                      if the deadline is reached, this is the next interval.
 * @return true if the event deadline is reached and the event should be processed.
 */
bool celix_scheduledEvent_deadlineReached(celix_scheduled_event_t* event,
                                          const struct timespec* currentTime,
                                          double* nextProcessTimeInSeconds);

/**
 * @brief Process the event by calling the event callback.
 *
 * Must be called on the Celix event thread.
 *
 * @param[in] event The event to process.
 * @param[in] currentTime The current time.
 * @return The time in seconds until the next event should be processed.
 */
void celix_scheduledEvent_process(celix_scheduled_event_t* event, const struct timespec* currentTime);

/**
 * @brief Returns true if the event is a one-shot event and is done.
 * @param[in] event The event to check.
 * @return  true if the event is a one-shot event and is done.
 */
bool celix_scheduleEvent_isDone(celix_scheduled_event_t* event);

/**
 * @brief Wait for a scheduled event to reach at least the provided call count.
 * @param[in] event The event to wait for.
 * @param[in] callCount The call count to wait for.
 * @param[in] timeout The max time to wait in seconds.
 * @return CELIX_SUCCESS if the scheduled event reached the call count, CELIX_TIMEOUT if the scheduled event
 *                       did not reach the call count within the timeout.
 */
celix_status_t
celix_scheduledEvent_waitForAtLeastCallCount(celix_scheduled_event_t* event, size_t useCount, double timeout);

#ifdef __cplusplus
};
#endif

#endif //CELIX_CELIX_SCHEDULED_EVENT_H
