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

#ifndef FRAMEWORK_PRIVATE_H_
#define FRAMEWORK_PRIVATE_H_

#include <stdbool.h>

#include "celix_framework.h"
#include "framework.h"
#include "celix_bundle_manifest.h"
#include "hash_map.h"
#include "celix_array_list.h"
#include "celix_errno.h"
#include "service_factory.h"
#include "bundle_archive.h"
#include "celix_service_listener.h"
#include "bundle_listener.h"
#include "framework_listener.h"
#include "service_registration.h"
#include "bundle_context.h"
#include "celix_bundle_cache.h"
#include "celix_log.h"
#include "celix_threads.h"
#include "service_registry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE
#define CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE 1024
#endif

#define CELIX_FRAMEWORK_DEFAULT_MAX_TIMEDWAIT_EVENT_HANDLER_IN_SECONDS 1

#define CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE_DEFAULT false
#define CELIX_FRAMEWORK_CACHE_USE_TMP_DIR_DEFAULT false
#define CELIX_FRAMEWORK_CACHE_DIR_DEFAULT ".cache"

typedef struct celix_framework_bundle_entry {
    celix_bundle_t *bnd;
    celix_thread_rwlock_t fsmMutex; //protects bundle state transition
    long bndId;

    celix_thread_mutex_t useMutex; //protects useCount
    celix_thread_cond_t useCond;
    size_t useCount;
} celix_framework_bundle_entry_t;

enum celix_framework_event_type {
    CELIX_FRAMEWORK_EVENT_TYPE      = 0x01,
    CELIX_BUNDLE_EVENT_TYPE         = 0x11,
    CELIX_REGISTER_SERVICE_EVENT    = 0x21,
    CELIX_UNREGISTER_SERVICE_EVENT  = 0x22,
    CELIX_GENERIC_EVENT             = 0x30
};

typedef enum celix_framework_event_type celix_framework_event_type_e;

struct celix_framework_event {
    celix_framework_event_type_e type;
    celix_framework_bundle_entry_t* bndEntry;

    void *doneData;
    void (*doneCallback)(void*);

    //for framework event
    framework_event_type_e fwEvent;
    celix_status_t errorCode;
    const char *error;

    //for bundle event
    bundle_event_type_e bundleEvent;

    //for register event
    long registerServiceId;
    bool cancelled;
    char *serviceName;
    void *svc;
    celix_service_factory_t* factory;
    celix_properties_t* properties;
    void* registerData;
    void (*registerCallback)(void *data, long serviceId);

    //for unregister event
    long unregisterServiceId;

    //for the generic event
    long genericEventId;
    const char* genericEventName;
    void *genericProcessData;
    void (*genericProcess)(void*);


};

typedef struct celix_framework_event celix_framework_event_t;

enum celix_bundle_lifecycle_command {
    CELIX_BUNDLE_LIFECYCLE_START,
    CELIX_BUNDLE_LIFECYCLE_STOP,
    CELIX_BUNDLE_LIFECYCLE_UNINSTALL,
    CELIX_BUNDLE_LIFECYCLE_UPDATE,
    CELIX_BUNDLE_LIFECYCLE_UNLOAD
};

typedef struct celix_framework_bundle_lifecycle_handler {
    celix_framework_t* framework;
    celix_framework_bundle_entry_t* bndEntry;
    long bndId;
    char* updatedBundleUrl; //only relevant and present for update command
    enum celix_bundle_lifecycle_command command;
} celix_framework_bundle_lifecycle_handler_t;

struct celix_framework {
    celix_bundle_t *bundle;
    long bundleId; //the bundle id of the framework (normally 0)
    hash_map_pt installRequestMap;

    celix_array_list_t* frameworkListeners;
    celix_thread_mutex_t frameworkListenersLock;

    celix_array_list_t* bundleListeners;
    celix_thread_mutex_t bundleListenerLock;

    long currentBundleId; //atomic
    celix_service_registry_t *registry;
    celix_bundle_cache_t* cache;

    struct {
        celix_thread_mutex_t mutex;
        celix_thread_cond_t cond;
        bool done; //true is shutdown is done
        bool joined; //true if shutdown thread is joined
        bool initialized; //true is a shutdown is initialized
        celix_thread_t thread;
    } shutdown;

    celix_thread_mutex_t installLock; // serialize install/uninstall
    struct {
        celix_array_list_t *entries; //value = celix_framework_bundle_entry_t*. Note ordered by installed bundle time
                                     //i.e. later installed bundle are last
        celix_thread_mutex_t mutex;
    } installedBundles;


    celix_properties_t* configurationMap;


    struct {
        long nextEventId; //atomic
        long nextScheduledEventId; //atomic

        celix_thread_cond_t cond;
        celix_thread_t thread;
        celix_thread_mutex_t mutex; //protects below
        bool active;

        //normal event queue
        celix_framework_event_t* eventQueue; //ring buffer
        int eventQueueCap;
        int eventQueueSize;
        int eventQueueFirstEntry;
        celix_array_list_t *dynamicEventQueue; //entry = celix_framework_event_t*. Used when the eventQueue is full
        struct {
            int nbFramework; // number of pending framework events
            int nbBundle; // number of pending bundle events
            int nbRegister; // number of pending registration
            int nbUnregister; // number of pending async de-registration
            int nbEvent; // number of pending generic events
        } stats;
        celix_long_hash_map_t *scheduledEvents; //key = scheduled event id, entry = celix_framework_scheduled_event_t*. Used for scheduled events
    } dispatcher;

    celix_framework_logger_t* logger;

    struct {
        celix_thread_cond_t cond;
        celix_thread_mutex_t mutex; //protects below
        celix_array_list_t* bundleLifecycleHandlers; //entry = celix_framework_bundle_lifecycle_handler_t*
    } bundleLifecycleHandling;
};

/**
 * @brief Get the config property for the given key.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @param found If not NULL, the found flag is set to true if the property is found, otherwise false.
 * @return The property value or the default value if the property is not found.
 */
const char* celix_framework_getConfigProperty(celix_framework_t* framework, const char* name, const char* defaultValue, bool* found);

/**
 * @brief Get the config property for the given key converted as long value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @param found If not NULL, the found flag is set to true if the property is found and converted, otherwise false.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a long value.
 */
long celix_framework_getConfigPropertyAsLong(celix_framework_t* framework, const char* name, long defaultValue, bool* found);

/**
 * @brief Get the config property for the given key converted as double value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @param found If not NULL, the found flag is set to true if the property is found and converted, otherwise false.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a double value.
 */
double celix_framework_getConfigPropertyAsDouble(celix_framework_t* framework, const char* name, double defaultValue, bool* found);

/**
 * @brief Get the config property for the given key converted as bool value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @param found If not NULL, the found flag is set to true if the property is found and converted, otherwise false.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a bool value.
 */
bool celix_framework_getConfigPropertyAsBool(celix_framework_t* framework, const char* name, bool defaultValue, bool* found);

celix_status_t celix_framework_installBundleInternal(celix_framework_t* framework, const char* bndLoc, long* bndId);

celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, long bundleId, const char* serviceName, const void* svcObj, celix_properties_t* properties);
celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, long bundleId, const char* serviceName, service_factory_pt factory, celix_properties_t* properties);

celix_status_t fw_getServiceReferences(framework_pt framework, celix_array_list_t** references, bundle_pt bundle, const char* serviceName, const char* filter);
celix_status_t fw_getBundleRegisteredServices(framework_pt framework, bundle_pt bundle, celix_array_list_t** services);
celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, celix_array_list_t** services);

void fw_addServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener, const char* filter);
void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener);

celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);
celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);

celix_status_t fw_addFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener);
celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener);

celix_array_list_t* framework_getBundles(framework_pt framework) __attribute__((deprecated("not thread safe, use celix_framework_useBundles instead")));
long framework_getBundle(framework_pt framework, const char* location);
bundle_pt framework_getBundleById(framework_pt framework, long id);








/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

/**
 * register service or service factory. Will return a svc id directly and return a service registration in a callback.
 * callback is called on the fw event loop thread
 */
long celix_framework_registerService(framework_t *fw, celix_bundle_t *bnd, const char* serviceName, void* svc, celix_service_factory_t *factory, celix_properties_t *properties);

/**
 * register service or service factory async. Will return a svc id directly and return a service registration in a callback.
 * callback is called on the fw event loop thread
 */
long celix_framework_registerServiceAsync(
        framework_t *fw,
        celix_bundle_t *bnd,
        const char* serviceName,
        void* svc,
        celix_service_factory_t* factory,
        celix_properties_t *properties,
        void* registerDoneData,
        void(*registerDoneCallback)(void *registerDoneData, long serviceId),
        void* eventDoneData,
        void (*eventDoneCallback)(void* eventDoneData));

/**
 * Unregister service async on the event loop thread.
 */
void celix_framework_unregisterAsync(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId, void *doneData, void (*doneCallback)(void*));

/**
 * Unregister service
 */
void celix_framework_unregister(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId);

/**
 * Wait til all service registration or unregistration events for a specific bundle are no longer present in the event queue.
 */
void celix_framework_waitForAsyncRegistrations(celix_framework_t *fw, long bndId);

/**
 * Wait til the async service registration for the provided service id is no longer present in the event queue.
 */
void celix_framework_waitForAsyncRegistration(celix_framework_t *fw, long svcId);

/**
 * Wait til the async service unregistration for the provided service id is no longer present in the event queue.
 */
void celix_framework_waitForAsyncUnregistration(celix_framework_t *fw, long svcId);

/**
 * Increase the use count of a bundle and ensure that a bundle cannot be uninstalled.
 */
void celix_framework_bundleEntry_increaseUseCount(celix_framework_bundle_entry_t *entry);

/**
 * Decrease the use count of a bundle.
 */
void celix_framework_bundleEntry_decreaseUseCount(celix_framework_bundle_entry_t *entry);

/**
 * Find the bundle entry for the bnd id and increase use count
 */
celix_framework_bundle_entry_t* celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(celix_framework_t *fw, long bndId);

/**
 * @brief Check if the bundle id is already in use.
 * Note that this can happen if bundles have been created through already existing bundle archives.
 * @param fw The framework.
 * @param bndId The bundle id.
 * @return True if the bundle id is already in use, false otherwise.
 */
bool celix_framework_isBundleIdAlreadyUsed(celix_framework_t *fw, long bndId);

/**
 * @brief Check if a bundle with the provided bundle symbolic name is already installed.
 */
bool celix_framework_isBundleAlreadyInstalled(celix_framework_t* fw, const char* bundleSymbolicName);

 /**
  * Start a bundle and ensure that this is not done on the Celix event thread.
  * Will spawn a thread if needed.
  * @param fw The Celix framework
  * @param bndEntry A bnd entry
  * @param forceSpawnThread If the true, the start bundle will always be done on a spawn thread
  * @return CELIX_SUCCESS of the call went alright.
  */
celix_status_t celix_framework_startBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread);

/**
 * Stop a bundle and ensure that this is not done on the Celix event thread.
 * Will spawn a thread if needed.
 * @param fw The Celix framework
 * @param bndEntry A bnd entry
 * @param forceSpawnThread If the true, the start bundle will always be done on a spawn thread
 * @return CELIX_SUCCESS of the call went alright.
 */
celix_status_t celix_framework_stopBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread);

/**
 * Uninstall (and if needed stop) a bundle and ensure that this is not done on the Celix event thread.
 * Will spawn a thread if needed.
 * @param fw The Celix framework
 * @param bndEntry A bnd entry
 * @param forceSpawnThread If the true, the start bundle will always be done on a spawn thread
 * @param permanent If true, the bundle will be permanently uninstalled (e.g. the bundle archive will be removed).
 * @return CELIX_SUCCESS of the call went alright.
 */
celix_status_t celix_framework_uninstallBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread, bool permanent);

/**
 * Update (and if needed stop and start) a bundle and ensure that this is not done on the Celix event thread.
 * Will spawn a thread if needed.
 * @param bndEntry A bnd entry
 * @param forceSpawnThread If the true, the start bundle will always be done on a spawn thread
 * @return CELIX_SUCCESS of the call went alright.
 */
celix_status_t celix_framework_updateBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, const char* updatedBundleUrl, bool forceSpawnThread);

/**
 * Wait for all bundle lifecycle handlers finishing their jobs.
 * @param fw The Celix framework
 */
void celix_framework_waitForBundleLifecycleHandlers(celix_framework_t* fw);

/**
 * Start a bundle. Cannot be called on the Celix event thread.
 */
celix_status_t celix_framework_startBundleEntry(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry);

/**
 * Stop a bundle. Cannot be called on the Celix event thread.
 */
celix_status_t celix_framework_stopBundleEntry(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry);

/**
 * Uninstall a bundle. Cannot be called on the Celix event thread.
 */
celix_status_t celix_framework_uninstallBundleEntry(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool permanent);

/**
 * Update a bundle. Cannot be called on the Celix event thread.
 */
celix_status_t celix_framework_updateBundleEntry(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, const char* updatedBundleUrl);


/** @brief Return the next scheduled event id.
 * @param[in] fw The Celix framework
 * @return The next scheduled event id.
 */
long celix_framework_nextScheduledEventId(framework_t *fw);

/**
 * @brief Add a scheduled event to the Celix framework.
 *
 *
 * @param[in] fw The Celix framework
 * @param[in] bndId The bundle id to add the scheduled event for.
 * @param[in] eventName The event name to use for the scheduled event. If NULL, a default event name is used.
 * @param[in] initialDelayInSeconds The initial delay in seconds before the first event callback is called.
 * @param[in] intervalInSeconds The interval in seconds between event callbacks.
 * @param[in] callbackData The event data to pass to the event callback.
 * @param[in] callback The event callback to call when the scheduled event is triggered.
 * @param[in] removeCallbackData The removed callback data.
 * @param[in] removeCallback The removed callback.
 * @return The scheduled event id of the scheduled event. Can be used to cancel the event.
 * @retval <0 If the event could not be added.
 */
long celix_framework_scheduleEvent(celix_framework_t* fw,
                                    long bndId,
                                    const char* eventName,
                                    double initialDelayInSeconds,
                                    double intervalInSeconds,
                                    void* callbackData,
                                    void (*callback)(void*),
                                    void* removeCallbackData,
                                    void (*removeCallback)(void*));

/**
 * @brief Wakeup a scheduled event and returns immediately, not waiting for the scheduled event callback to be
 * called.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] fw The Celix framework
 * @param[in] scheduledEventId The scheduled event id to wakeup.
 * @return CELIX_SUCCESS if the scheduled event is woken up, CELIX_ILLEGAL_ARGUMENT if the scheduled event id is not known.
 */
celix_status_t celix_framework_wakeupScheduledEvent(celix_framework_t* fw, long scheduledEventId);

/**
 * @brief Wait for the next scheduled event to be processed.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] fw The Celix framework
 * @param[in] scheduledEventId The scheduled event id to wait for.
 * @param[in] waitTimeInSeconds The maximum time to wait for the next scheduled event. If <= 0 the function will return
 *                             immediately.
 * @return CELIX_SUCCESS if the scheduled event is done with processing, CELIX_ILLEGAL_ARGUMENT if the scheduled event id is not
 *         known and ETIMEDOUT if the waitTimeInSeconds is reached.
 */
celix_status_t celix_framework_waitForScheduledEvent(celix_framework_t* fw,
                                                     long scheduledEventId,
                                                     double waitTimeInSeconds);

/**
 * @brief Cancel a scheduled event.
 *
 * When this function returns, no more scheduled event callbacks will be called.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] fw The Celix framework
 * @param[in] async If true, the scheduled event will be cancelled asynchronously and the function will not block.
 * @param[in] errorIfNotFound If true, removal of a non existing scheduled event id will be logged.
 * @param[in] scheduledEventId The scheduled event id to cancel.
 * @return true if a scheduled event is cancelled, false if the scheduled event id is not known.
 */
bool celix_framework_removeScheduledEvent(celix_framework_t* fw, bool async, bool errorIfNotFound, long scheduledEventId);

/**
 * Remove all scheduled events for the provided bundle id and logs warning if there are still un-removed scheduled
 * events that are not a one time event.
 * @param[in] fw The Celix framework.
 * @param[in] bndId The bundle id to remove the scheduled events for.
 */
void celix_framework_cleanupScheduledEvents(celix_framework_t* fw, long bndId);


/**
 * @brief Start the celix framework shutdown sequence on a separate thread and return immediately.
 */
void celix_framework_shutdownAsync(celix_framework_t* framework);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_PRIVATE_H_ */
