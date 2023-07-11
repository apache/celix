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

/**
 * @brief A scheduled event is an event which is scheduled to be processed at a given initial delay and interval.
 */
typedef struct celix_scheduled_event celix_scheduled_event_t;

/**
 * @brief Create a scheduled event for the given bundle.
 *
 * The scheduled event will be created with a use count of 1.
 *
 * @param[in] fw The framework.
 * @param[in] bndId The bundle id for the bundle which the scheduled event is created.
 * @param[in] scheduledEventId The id of the scheduled event.
 * @param[in] providedEventName The name of the event. If NULL, CELIX_SCHEDULED_EVENT_DEFAULT_NAME is used.
 * @param[in] initialDelayInSeconds The initial delay in seconds.
 * @param[in] intervalInSeconds The interval in seconds.
 * @param[in] callbackData The event data.
 * @param[in] callback The event callback.
 * @param[in] removedCallbackData The removed callback data.
 * @param[in] removedCallback The removed callback.
 * @return A new scheduled event or NULL if failed.
 */
celix_scheduled_event_t* celix_scheduledEvent_create(celix_framework_t* fw,
                                                     long bndId,
                                                     long scheduledEventId,
                                                     const char* providedEventName,
                                                     double initialDelayInSeconds,
                                                     double intervalInSeconds,
                                                     void* callbackData,
                                                     void (*callback)(void* callbackData),
                                                     void* removedCallbackData,
                                                     void (*removedCallback)(void* removedCallbackData));

/**
 * @brief Retain the scheduled event by increasing the use count.
 * Will silently ignore a NULL event.
 *
 * @param[in] event The event to retain.
 * @return The retained event or NULL if the event is NULL.
 */
celix_scheduled_event_t* celix_scheduledEvent_retain(celix_scheduled_event_t* event);

/**
 * @brief Release the scheduled event by decreasing the use count. If the use count is 0,
 * the scheduled event is destroyed. Will silently ignore a NULL event.
 */
void celix_scheduledEvent_release(celix_scheduled_event_t* event);

/**
 * @brief Call the release of provided the scheduled event pointer.
 * Meant to be pair with a __cleanup__ attribute or CELIX_SCHEDULED_EVENT_RETAIN_GUARD macro.
 */
void celix_ScheduledEvent_cleanup(celix_scheduled_event_t** event);

/*!
 * @brief Retain the scheduled event, add a cleanup attribute to release the scheduled event and
 * return a pointer to it.
 *
 * This macro can be used as a guard to automatically release the scheduled event when leaving the current scope.
 *
 * @param[in] __var_name__ The name of the variable to create.
 * @param[in] __scheduled_event__ The scheduled event to retain.
 * @return A pointer to the retained scheduled event.
 */
#define CELIX_SCHEDULED_EVENT_RETAIN_GUARD(__var_name__, __scheduled_event__)                                          \
    __attribute__((__cleanup__(celix_ScheduledEvent_cleanup))) celix_scheduled_event_t* __var_name__ =                 \
        celix_scheduledEvent_retain(__scheduled_event__)

/**
 * @brief Returns the scheduled event name.
 */
const char* celix_scheduledEvent_getName(const celix_scheduled_event_t* event);

/**
 * @brief Returns the scheduled event ID.
 */
long celix_scheduledEvent_getId(const celix_scheduled_event_t* event);

/**
 * @brief Returns the bundle id of the bundle which created the scheduled event.
 */
long celix_scheduledEvent_getBundleId(const celix_scheduled_event_t* event);

/**
 * @brief Returns whether the event deadline is reached and the event should be processed.
 * @param[in] event The event to check.
 * @param[in] scheduleTime The schedule time.
 * @return true if the event deadline is reached and the event should be processed.
 */
bool celix_scheduledEvent_deadlineReached(celix_scheduled_event_t* event,
                                          const struct timespec* scheduleTime);

/**
 * @brief Get the next deadline for the scheduled event.
 * @param[in] event The event to get the next deadline for.
 * @return The next deadline for the scheduled event.
 */
struct timespec celix_scheduledEvent_getNextDeadline(celix_scheduled_event_t* event);

/**
 * @brief Process the event by calling the event callback.
 *
 * Must be called on the Celix event thread.
 *
 * @param[in] event The event to process.
 */
void celix_scheduledEvent_process(celix_scheduled_event_t* event);

/**
 * @brief Call the removed callback of the event and set the removed flag.
 */
void celix_scheduledEvent_setRemoved(celix_scheduled_event_t* event);

/**
 * @brief Wait indefinitely until the event is removed.
 */
void celix_scheduledEvent_waitForRemoved(celix_scheduled_event_t* event);

/**
 * @brief Returns true if the event is a one-shot event.
 */
bool celix_scheduledEvent_isSingleShot(const celix_scheduled_event_t* event);

/**
 * @brief Mark the event for removal. The event will be removed on the event thread, after the next processing.
 */
void celix_scheduledEvent_markForRemoval(celix_scheduled_event_t* event);

/**
 * @brief Returns true if the event is marked for removal.
 */
bool celix_scheduledEvent_isMarkedForRemoval(celix_scheduled_event_t* event);

/**
 * @brief Configure a scheduled event for a wakeup, so celix_scheduledEvent_deadlineReached will return true until
 * the event is processed.
 *
 * @param[in] event The event to configure for wakeup.
 * @return The future call count of the event after the next processing is done.
 */
size_t celix_scheduledEvent_markForWakeup(celix_scheduled_event_t* event);

/**
 * @brief Wait for a scheduled event to be done with the next scheduled processing.
 * @param[in] event The event to wait for.
 * @param[in] timeoutInSeconds The max time to wait in seconds. Must be > 0.
 * @return CELIX_SUCCESS if the scheduled event is done with processing, ETIMEDOUT if the scheduled event is not
 *        done with processing within the timeout.
 */
celix_status_t celix_scheduledEvent_wait(celix_scheduled_event_t* event, double timeoutInSeconds);

/**
 * @brief Returns true if the event is marked for wakeup, the initial delay or interval deadline is reached or the
 * event is marked for removal for the given time.
 */
bool celix_scheduledEvent_requiresProcessing(celix_scheduled_event_t* event, const struct timespec* scheduleTime);

#ifdef __cplusplus
};
#endif

#endif // CELIX_CELIX_SCHEDULED_EVENT_H
