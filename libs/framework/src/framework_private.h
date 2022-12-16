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

#include "celix_framework.h"
#include "framework.h"

#include "manifest.h"
#include "wire.h"
#include "hash_map.h"
#include "array_list.h"
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

#ifndef CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE
#define CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE 1024
#endif

typedef struct celix_framework_bundle_entry {
    celix_bundle_t *bnd;
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
    CELIX_BUNDLE_LIFECYCLE_UNINSTALL
};

typedef struct celix_framework_bundle_lifecycle_handler {
    celix_thread_t thread;
    celix_framework_t* framework;
    celix_framework_bundle_entry_t* bndEntry;
    long bndId;
    enum celix_bundle_lifecycle_command command;
    int done; //NOTE atomic -> 0 not done, 1 done (thread can be joined)
} celix_framework_bundle_lifecycle_handler_t;

struct celix_framework {
    celix_bundle_t *bundle;
    long bundleId; //the bundle id of the framework (normally 0)
    hash_map_pt installRequestMap;

    array_list_pt frameworkListeners;
    celix_thread_mutex_t frameworkListenersLock;

    array_list_pt bundleListeners;
    celix_thread_mutex_t bundleListenerLock;

    long nextBundleId;
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

    struct {
        celix_array_list_t *entries; //value = celix_framework_bundle_entry_t*. Note ordered by installed bundle time
                                     //i.e. later installed bundle are last
        celix_thread_mutex_t mutex;
    } installedBundles;


    properties_pt configurationMap;


    struct {
        celix_thread_cond_t cond;
        celix_thread_t thread;
        celix_thread_mutex_t mutex; //protects below
        bool active;
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
    } dispatcher;

    celix_framework_logger_t* logger;

    long nextGenericEventId;

    struct {
        celix_thread_mutex_t mutex; //protects below
        celix_array_list_t* bundleLifecycleHandlers; //entry = celix_framework_bundle_lifecycle_handler_t*
    } bundleLifecycleHandling;
};

FRAMEWORK_EXPORT celix_status_t fw_getProperty(framework_pt framework, const char* name, const char* defaultValue, const char** value);

FRAMEWORK_EXPORT celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, const char * location, const char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t framework_getBundleEntry(framework_pt framework, const_bundle_pt bundle, const char* name, char** entry);
FRAMEWORK_EXPORT celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, const char* inputFile);

FRAMEWORK_EXPORT celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, long bundleId, const char* serviceName, const void* svcObj, properties_pt properties);
FRAMEWORK_EXPORT celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, long bundleId, const char* serviceName, service_factory_pt factory, properties_pt properties);
FRAMEWORK_EXPORT void fw_unregisterService(service_registration_pt registration);

FRAMEWORK_EXPORT celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char* serviceName, const char* filter);
FRAMEWORK_EXPORT celix_status_t fw_getBundleRegisteredServices(framework_pt framework, bundle_pt bundle, array_list_pt *services);
FRAMEWORK_EXPORT celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services);

FRAMEWORK_EXPORT void fw_addServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener, const char* filter);
FRAMEWORK_EXPORT void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener);

FRAMEWORK_EXPORT celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);
FRAMEWORK_EXPORT celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t fw_addFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener);
FRAMEWORK_EXPORT celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener);

FRAMEWORK_EXPORT void fw_serviceChanged(framework_pt framework, celix_service_event_type_t eventType, service_registration_pt registration, properties_pt oldprops);

FRAMEWORK_EXPORT celix_status_t fw_isServiceAssignable(framework_pt fw, bundle_pt requester, service_reference_pt reference, bool* assignable);

//bundle_archive_t fw_createArchive(long id, char * location);
//void revise(bundle_archive_t archive, char * location);
FRAMEWORK_EXPORT celix_status_t getManifest(bundle_archive_pt archive, manifest_pt *manifest);

FRAMEWORK_EXPORT bundle_pt findBundle(bundle_context_pt context);
FRAMEWORK_EXPORT service_registration_pt findRegistration(service_reference_pt reference);

FRAMEWORK_EXPORT service_reference_pt listToArray(array_list_pt list);
FRAMEWORK_EXPORT celix_status_t framework_markResolvedModules(framework_pt framework, linked_list_pt wires);

FRAMEWORK_EXPORT array_list_pt framework_getBundles(framework_pt framework) __attribute__((deprecated("not thread safe, use celix_framework_useBundles instead")));
FRAMEWORK_EXPORT bundle_pt framework_getBundle(framework_pt framework, const char* location);
FRAMEWORK_EXPORT bundle_pt framework_getBundleById(framework_pt framework, long id);








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
 * Returns whether the current thread is the Celix framework event loop thread.
 */
bool celix_framework_isCurrentThreadTheEventLoop(celix_framework_t* fw);

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
 * @return CELIX_SUCCESS of the call went alright.
 */
celix_status_t celix_framework_uninstallBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread);

/**
 * cleanup finished bundle lifecyles threads.
 * @param fw                The framework.
 * @param waitTillEmpty     Whether to wait for all threads to be finished.
 */
void celix_framework_cleanupBundleLifecycleHandlers(celix_framework_t* fw, bool waitTillEmpty);

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
celix_status_t celix_framework_uninstallBundleEntry(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry);

#endif /* FRAMEWORK_PRIVATE_H_ */
