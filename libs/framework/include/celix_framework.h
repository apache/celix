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

#ifndef CELIX_FRAMEWORK_H_
#define CELIX_FRAMEWORK_H_


#include "celix_types.h"
#include "celix_properties.h"
#include "celix_log_level.h"
#include "celix_array_list.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the framework UUID. This is unique for every created framework and will not be the same if the process is
 * restarted.
 */
const char* celix_framework_getUUID(const celix_framework_t *fw);

/**
 * @brief Returns the framework bundle context. This is the same as a 'normal' bundle context and can be used to register, use
 * and track services. The only difference is that the framework is the bundle.
 * @param fw The framework
 * @return A pointer to the bundle context of the framework or NULL if something went wrong.
 */
celix_bundle_context_t* celix_framework_getFrameworkContext(const celix_framework_t *fw);

/**
 * @brief Returns the framework bundle. This is the same as a 'normal' bundle, expect that this bundle cannot be uninstalled
 * and the `celix_bundle_getEntry` return a entries relative from the working directory.
  * @param fw The framework
 * @return A pointer to the bundle of the framework or NULL if something went wrong.
 */
celix_bundle_t* celix_framework_getFrameworkBundle(const celix_framework_t *fw);

/**
 * @brief Use the currently active (started) bundles.
 * The provided callback will be called for all the currently started bundles.
 *
 * @param ctx                       The bundle context.
 * @param includeFrameworkBundle    If true the callback will also be triggered for the framework bundle.
 * @param callbackHandle            The data pointer, which will be used in the callbacks
 * @param use                       The callback which will be called for the currently started bundles.
 *                                  The bundle pointers are only guaranteed to be valid during the callback.
 */
void celix_framework_useBundles(celix_framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const celix_bundle_t *bnd));

/**
 * @brief Use the bundle with the provided bundle id
 * The provided callback will be called if the bundle is found.
 *
 * @param fw                The framework.
 * @param onlyActive        If true only starting and active bundles will trigger the callback.
 * @param bundleId          The bundle id.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 * @return                  Returns true if the bundle is found and the callback is called.
 */
bool celix_framework_useBundle(celix_framework_t *fw, bool onlyActive, long bndId, void *callbackHandle, void(*use)(void *handle, const celix_bundle_t *bnd));

/**
 * @brief Check whether a bundle is installed.
 * @param fw        The Celix framework
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed.
 */
bool celix_framework_isBundleInstalled(celix_framework_t *fw, long bndId);

/**
 * @brief Check whether the bundle is active.
 * @param fw        The Celix framework
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed and active.
 */
bool celix_framework_isBundleActive(celix_framework_t *fw, long bndId);


/**
 * @brief Install and optional start a bundle.
 * Will silently ignore bundle ids < 0.
 *
 * @param fw The Celix framework
 * @param bundleLoc The bundle location to the bundle zip file.
 * @param autoStart If the bundle should also be started.
 * @return the bundleId (>= 0) or < 0 if the bundle could not be installed and possibly started.
 */
long celix_framework_installBundle(celix_framework_t *fw, const char *bundleLoc, bool autoStart);

/**
 * @brief Uninstall the bundle with the provided bundle id. If needed the bundle will be stopped first.
 * Will silently ignore bundle ids < 0.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to uninstall.
 * @return true if the bundle is correctly uninstalled. False if not.
 */
bool celix_framework_uninstallBundle(celix_framework_t *fw, long bndId);

/**
 * @brief Stop the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to stop.
 * @return true if the bundle is found & correctly stop. False if not.
 */
bool celix_framework_stopBundle(celix_framework_t *fw, long bndId);

/**
 * @brief Start the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to start.
 * @return true if the bundle is found & correctly started. False if not.
 */
bool celix_framework_startBundle(celix_framework_t *fw, long bndId);

/**
 * @brief Install and optional start a bundle async.
 * Will silently ignore bundle ids < 0.
 *
 * If the bundle needs to be started this will be done a separate spawned thread.
 *
 * @param fw The Celix framework
 * @param bundleLoc The bundle location to the bundle zip file.
 * @param autoStart If the bundle should also be started.
 * @return The bundle id of the installed bundle or -1 if the bundle could not be installed
 */
long celix_framework_installBundleAsync(celix_framework_t *fw, const char *bundleLoc, bool autoStart);

/**
 * @brief Uninstall the bundle with the provided bundle id async. If needed the bundle will be stopped first.
 * Will silently ignore bundle ids < 0.
 *
 * The bundle will be uninstalled on a separate spawned thread.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to uninstall.
 */
void celix_framework_uninstallBundleAsync(celix_framework_t *fw, long bndId);

/**
 * @brief Stop the bundle with the provided bundle id async.
 * Will silently ignore bundle ids < 0.
 *
 * The bundle will be stopped on a separate spawned thread.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to stop.
 */
void celix_framework_stopBundleAsync(celix_framework_t *fw, long bndId);

/**
 * @brief Start the bundle with the provided bundle id async.
 * Will silently ignore bundle ids < 0.
 *
 * The bundle will be started on a separate spawned thread.
 *
 * @param fw The Celix framework
 * @param bndId The bundle id to start.
 */
void celix_framework_startBundleAsync(celix_framework_t *fw, long bndId);

/**
 * @brief List the installed and started bundle ids.
 * The bundle ids does not include the framework bundle (bundle id CELIX_FRAMEWORK_BUNDLE_ID).
 *
 * @param framework The Celix framework.
 * @return A array with bundle ids (long). The caller is responsible for destroying the array.
 */
celix_array_list_t* celix_framework_listBundles(celix_framework_t* framework);

/**
 * @brief List the installed bundle ids.
 * The bundle ids does not include the framework bundle (bundle id CELIX_FRAMEWORK_BUNDLE_ID).
 *
 * @param framework The Celix framework.
 * @return A array with bundle ids (long). The caller is responsible for destroying the array.
 */
celix_array_list_t* celix_framework_listInstalledBundles(celix_framework_t* framework);

/**
 * @brief Wait until the framework event queue is empty.
 *
 * The Celix framework has an event queue which (among others) handles bundle events.
 * This function can be used to ensure that all queue event are handled, mainly useful
 * for testing.
 *
 * @param fw The Celix Framework
 */
void celix_framework_waitForEmptyEventQueue(celix_framework_t *fw);

/**
 * @brief Sets the log function for this framework.
 * Default the celix framework will log to stdout/stderr.
 *
 * A log function can be injected to change how the Celix framework logs.
 * Can be reset by setting the log function to NULL.
 */
void celix_framework_setLogCallback(celix_framework_t* fw, void* logHandle, void (*logFunction)(void* handle, celix_log_level_e level, const char* file, const char *function, int line, const char *format, va_list formatArgs));


/**
 * @brief wait until all events for the bundle identified by the bndId are processed.
 */
void celix_framework_waitUntilNoEventsForBnd(celix_framework_t* fw, long bndId);

/**
 * @brief wait until all pending service registration  are processed.
 */
void celix_framework_waitUntilNoPendingRegistration(celix_framework_t* fw);

/**
 * @brief Returns whether the current thread is the Celix framework event loop thread.
 */
bool celix_framework_isCurrentThreadTheEventLoop(celix_framework_t* fw);


/**
 * @brief Fire a generic event. The event will be added to the event loop and handled on the event loop thread.
 *
 * if bndId >=0 the bundle usage count will be increased while the event is not yet processed or finished processing.
 * The eventName is expected to be const char* valid during til the event is finished processing.
 *
 * if eventId >=0 this will be used, otherwise a new event id will be generated
 * return eventId
 */
long celix_framework_fireGenericEvent(celix_framework_t* fw, long eventId, long bndId, const char *eventName, void* processData, void (*processCallback)(void *data), void* doneData, void (*doneCallback)(void* doneData));

/**
 * @brief Get the next event id.
 *
 * This can be used to ensure celix_framework_waitForGenericEvent can be used to wait for an event.
 * The returned event id will not be used by the framework itself unless followed up with a
 * celix_framework_fireGenericEvent call using the returned event id.
 */
long celix_framework_nextEventId(celix_framework_t *fw);

/**
 * @brief Wait until a event with the provided event id is completely handled.
 * This function will directly return if the provided event id is not in the event loop (already done or never issued).
 */
void celix_framework_waitForGenericEvent(celix_framework_t *fw, long eventId);

/**
 * @brief Wait until the framework is stopped.
 */
void celix_framework_waitForStop(celix_framework_t *framework);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_H_ */
