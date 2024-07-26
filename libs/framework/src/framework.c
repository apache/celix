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
#include <celix_log_utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "celix_build_assert.h"
#include "celix_bundle_context.h"
#include "celix_compiler.h"
#include "celix_constants.h"
#include "celix_convert_utils.h"
#include "celix_dependency_manager.h"
#include "celix_file_utils.h"
#include "celix_framework_utils_private.h"
#include "celix_libloader.h"
#include "celix_log_constants.h"
#include "celix_module_private.h"
#include "celix_framework_bundle.h"

#include "bundle_archive_private.h"
#include "bundle_context_private.h"
#include "celix_bundle_private.h"
#include "celix_err.h"
#include "celix_scheduled_event.h"
#include "celix_stdlib_cleanup.h"
#include "framework_private.h"
#include "service_reference_private.h"
#include "service_registration_private.h"
#include "utils.h"

struct celix_bundle_activator {
    void * userData;

    celix_bundle_activator_create_fp create;
    celix_bundle_activator_start_fp start;
    celix_bundle_activator_stop_fp stop;
    celix_bundle_activator_destroy_fp destroy;
};

static int celix_framework_eventQueueSize(celix_framework_t* fw);
static celix_status_t celix_framework_stopBundleEntryInternal(celix_framework_t* framework,
                                                              celix_bundle_entry_t* bndEntry);

static inline celix_bundle_entry_t* fw_bundleEntry_create(celix_bundle_t *bnd) {
    celix_bundle_entry_t*entry = calloc(1, sizeof(*entry));
    entry->bnd = bnd;
    celixThreadRwlock_create(&entry->fsmMutex, NULL);
    entry->bndId = celix_bundle_getId(bnd);
    entry->useCount = 0;
    celixThreadMutex_create(&entry->useMutex, NULL);
    celixThreadCondition_init(&entry->useCond, NULL);
    return entry;
}

static inline void fw_bundleEntry_destroy(celix_bundle_entry_t*entry, bool wait) {
    celixThreadMutex_lock(&entry->useMutex);
    while (wait && entry->useCount != 0) {
        celixThreadCondition_wait(&entry->useCond, &entry->useMutex);
    }
    celixThreadMutex_unlock(&entry->useMutex);

    //destroy
    celixThreadMutex_destroy(&entry->useMutex);
    celixThreadCondition_destroy(&entry->useCond);
    celixThreadRwlock_destroy(&entry->fsmMutex);
    free(entry);
}

void celix_bundleEntry_increaseUseCount(celix_bundle_entry_t*entry) {
    assert(entry != NULL);
    celixThreadMutex_lock(&entry->useMutex);
    ++entry->useCount;
    celixThreadMutex_unlock(&entry->useMutex);
}

void celix_bundleEntry_decreaseUseCount(celix_bundle_entry_t*entry) {
    celixThreadMutex_lock(&entry->useMutex);
    assert(entry->useCount > 0);
    --entry->useCount;
    celixThreadCondition_broadcast(&entry->useCond);
    celixThreadMutex_unlock(&entry->useMutex);
}

celix_bundle_entry_t* celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(celix_framework_t *fw, long bndId) {
    celix_bundle_entry_t* found = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId == bndId) {
            celix_bundleEntry_increaseUseCount(entry);
            found = entry;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return found;
}

bool celix_framework_isBundleIdAlreadyUsed(celix_framework_t *fw, long bndId) {
    bool found = false;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId == bndId) {
            found = true;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return found;
}

static inline bool fw_bundleEntry_removeBundleEntry(celix_framework_t *fw, celix_bundle_entry_t* bndEntry) {
    bool found = false;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry == bndEntry) {
            found = true;
            celix_arrayList_removeAt(fw->installedBundles.entries, i);
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return found;
}

static celix_status_t framework_markBundleResolved(framework_pt framework, celix_module_t* module);

long framework_getNextBundleId(framework_pt framework);

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map);

void fw_fireBundleEvent(framework_pt framework, bundle_event_type_e, celix_bundle_entry_t* entry);
void fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, celix_status_t errorCode);
static void *fw_eventDispatcher(void *fw);

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle);
celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle);

static celix_status_t framework_autoStartConfiguredBundles(celix_framework_t *fw, bool* startedAllBundles);
static celix_status_t framework_autoInstallConfiguredBundles(celix_framework_t *fw, bool* installedAllBundles);
static bool framework_autoInstallConfiguredBundlesForList(celix_framework_t *fw, const char *autoStart, celix_array_list_t *installedBundles);
static bool framework_autoStartConfiguredBundlesForList(celix_framework_t* fw, const celix_array_list_t *installedBundles);
static void celix_framework_addToEventQueue(celix_framework_t *fw, const celix_framework_event_t* event);
static void celix_framework_stopAndJoinEventQueue(celix_framework_t* fw);

struct fw_bundleListener {
	bundle_pt bundle;
	bundle_listener_pt listener;

    celix_thread_mutex_t useMutex; //protects useCount
    celix_thread_cond_t useCond;
    size_t useCount;
};

typedef struct fw_bundleListener * fw_bundle_listener_pt;

static inline void fw_bundleListener_destroy(fw_bundle_listener_pt listener, bool wait) {
    if (wait) {
        celixThreadMutex_lock(&listener->useMutex);
        while (listener->useCount != 0) {
            celixThreadCondition_wait(&listener->useCond, &listener->useMutex);
        }
        celixThreadMutex_unlock(&listener->useMutex);
    }

    //destroy
    celixThreadMutex_destroy(&listener->useMutex);
    celixThreadCondition_destroy(&listener->useCond);
    free(listener);
}

static inline void fw_bundleListener_increaseUseCount(fw_bundle_listener_pt listener) {
    //pre condition mutex is taken on  fw->installedBundles.mutex
    assert(listener != NULL);
    celixThreadMutex_lock(&listener->useMutex);
    ++listener->useCount;
    celixThreadMutex_unlock(&listener->useMutex);
}

static inline void fw_bundleListener_decreaseUseCount(fw_bundle_listener_pt listener) {
    //pre condition mutex is taken on  fw->installedBundles.mutex
    celixThreadMutex_lock(&listener->useMutex);
    assert(listener->useCount > 0);
    --listener->useCount;
    celixThreadCondition_broadcast(&listener->useCond);
    celixThreadMutex_unlock(&listener->useMutex);
}

struct fw_frameworkListener {
	bundle_pt bundle;
	framework_listener_pt listener;
};

typedef struct fw_frameworkListener * fw_framework_listener_pt;

static void celix_framework_createAndStoreFrameworkUUID(celix_framework_t* fw) {
    if (celix_properties_get(fw->configurationMap, CELIX_FRAMEWORK_UUID, NULL)  == NULL) {
        char uuid[37];
        uuid_t uid;
        uuid_generate(uid);
        uuid_unparse(uid, uuid);
        celix_properties_set(fw->configurationMap, CELIX_FRAMEWORK_UUID, uuid);
    }
}

celix_status_t framework_create(framework_pt *out, celix_properties_t* config) {
    celix_framework_t* framework = calloc(1, sizeof(*framework));
    if (!framework) {
        return ENOMEM;
    }

    celixThreadCondition_init(&framework->shutdown.cond, NULL);
    celixThreadMutex_create(&framework->shutdown.mutex, NULL);
    celixThreadMutex_create(&framework->dispatcher.mutex, NULL);
    celixThreadMutex_create(&framework->frameworkListenersLock, NULL);
    celixThreadMutex_create(&framework->bundleListenerLock, NULL);
    celixThreadMutex_create(&framework->installLock, NULL);
    celixThreadMutex_create(&framework->installedBundles.mutex, NULL);
    celixThreadCondition_init(&framework->dispatcher.cond, NULL);
    framework->dispatcher.active = true;
    framework->currentBundleId = CELIX_FRAMEWORK_BUNDLE_ID;
    framework->installRequestMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
    framework->installedBundles.entries = celix_arrayList_create();
    framework->configurationMap = config; //note form now on celix_framework_getConfigProperty* can be used
    framework->bundleListeners = celix_arrayList_create();
    framework->frameworkListeners = celix_arrayList_create();
    framework->dispatcher.eventQueueCap = (int)celix_framework_getConfigPropertyAsLong(framework, CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE, CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE, NULL);
    framework->dispatcher.eventQueue = malloc(sizeof(celix_framework_event_t) * framework->dispatcher.eventQueueCap);
    framework->dispatcher.dynamicEventQueue = celix_arrayList_create();
    framework->dispatcher.scheduledEvents = celix_longHashMap_create();

    celix_framework_createAndStoreFrameworkUUID(framework);

    //setup framework logger
    const char* logStr = celix_framework_getConfigProperty(framework, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_DEFAULT_VALUE, NULL);
    framework->logger = celix_frameworkLogger_create(celix_logUtils_logLevelFromString(logStr, CELIX_LOG_LEVEL_INFO));

    celix_status_t status = celix_bundleCache_create(framework, &framework->cache);
    bundle_archive_t* systemArchive = NULL;
    status = CELIX_DO_IF(status, celix_bundleCache_createSystemArchive(framework, &systemArchive));
    status = CELIX_DO_IF(status, celix_bundle_createFromArchive(framework, systemArchive, &framework->bundle));
    status = CELIX_DO_IF(status, bundle_getBundleId(framework->bundle, &framework->bundleId));
    framework->registry = celix_serviceRegistry_create(framework);
    bundle_context_t *context = NULL;
    status = CELIX_DO_IF(status, bundleContext_create(framework, framework->logger, framework->bundle, &context));
    CELIX_DO_IF(status, celix_bundle_setContext(framework->bundle, context));

    //create framework bundle entry
    celix_bundle_entry_t*entry = fw_bundleEntry_create(framework->bundle);
    celixThreadMutex_lock(&framework->installedBundles.mutex);
    celix_arrayList_add(framework->installedBundles.entries, entry);
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create framework");
        free(framework);
        return status;
    }

    //setup framework bundle lifecycle handling
    celixThreadCondition_init(&framework->bundleLifecycleHandling.cond, NULL);
    celixThreadMutex_create(&framework->bundleLifecycleHandling.mutex, NULL);
    framework->bundleLifecycleHandling.bundleLifecycleHandlers = celix_arrayList_create();

    *out = framework;
    return status;
}

celix_status_t framework_destroy(framework_pt framework) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&framework->shutdown.mutex);
    bool shutdownInitialized = framework->shutdown.initialized;
    celixThreadMutex_unlock(&framework->shutdown.mutex);

    if (shutdownInitialized) {
        framework_waitForStop(framework);
    }

    celix_serviceRegistry_destroy(framework->registry);

    for (int i = 0; i < celix_arrayList_size(framework->installedBundles.entries); ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celixThreadMutex_lock(&entry->useMutex);
        size_t count = entry->useCount;
        celixThreadMutex_unlock(&entry->useMutex);
        bundle_t *bnd = entry->bnd;
        if (count > 0) {
            const char *bndName = celix_bundle_getSymbolicName(bnd);
            fw_log(framework->logger, CELIX_LOG_LEVEL_FATAL, "Cannot destroy framework. The use count of bundle %s (bnd id %li) is not 0, but %zu.", bndName, entry->bndId, count);
            celixThreadMutex_lock(&framework->dispatcher.mutex);
            int nrOfRequests = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
            celixThreadMutex_unlock(&framework->dispatcher.mutex);
            fw_log(framework->logger, CELIX_LOG_LEVEL_WARNING, "nr of request left: %i (should be 0).", nrOfRequests);
        }
        fw_bundleEntry_destroy(entry, true);

        bool systemBundle = false;
        bundle_isSystemBundle(bnd, &systemBundle);
        if (systemBundle) {
            bundle_context_t *context = celix_bundle_getContext(framework->bundle);
            bundleContext_destroy(context);
        }

        bundle_archive_t *archive = NULL;
        bundle_getArchive(bnd, &archive);
        celix_module_t* module = NULL;
        bundle_getCurrentModule(bnd, &module);
        if (!systemBundle && module) {
            celix_module_closeLibraries(module);
        }

        if (archive) {
            celix_bundleArchive_destroy(archive);
        }
        bundle_destroy(bnd);

    }
    celix_arrayList_destroy(framework->installedBundles.entries);
    celixThreadMutex_destroy(&framework->installedBundles.mutex);
    celixThreadMutex_destroy(&framework->installLock);

    //teardown framework bundle lifecycle handling
    assert(celix_arrayList_size(framework->bundleLifecycleHandling.bundleLifecycleHandlers) == 0);
    celix_arrayList_destroy(framework->bundleLifecycleHandling.bundleLifecycleHandlers);
    celixThreadMutex_destroy(&framework->bundleLifecycleHandling.mutex);
    celixThreadCondition_destroy(&framework->bundleLifecycleHandling.cond);

    hashMap_destroy(framework->installRequestMap, false, false);

    if (framework->bundleListeners) {
        celix_arrayList_destroy(framework->bundleListeners);
    }
    if (framework->frameworkListeners) {
        celix_arrayList_destroy(framework->frameworkListeners);
    }

    assert(celix_arrayList_size(framework->dispatcher.dynamicEventQueue) == 0);
    celix_arrayList_destroy(framework->dispatcher.dynamicEventQueue);

    assert(celix_longHashMap_size(framework->dispatcher.scheduledEvents) == 0);
    celix_longHashMap_destroy(framework->dispatcher.scheduledEvents);

    celix_bundleCache_destroy(framework->cache);

	celixThreadCondition_destroy(&framework->dispatcher.cond);
    celixThreadMutex_destroy(&framework->frameworkListenersLock);
	celixThreadMutex_destroy(&framework->bundleListenerLock);
	celixThreadMutex_destroy(&framework->dispatcher.mutex);
	celixThreadMutex_destroy(&framework->shutdown.mutex);
	celixThreadCondition_destroy(&framework->shutdown.cond);

    celix_frameworkLogger_destroy(framework->logger);

    celix_properties_destroy(framework->configurationMap);

    free(framework->dispatcher.eventQueue);
    free(framework);

	return status;
}

celix_status_t fw_init(framework_pt framework) {
    celixThreadMutex_lock(&framework->dispatcher.mutex);
    framework->dispatcher.active = true;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    celixThreadMutex_lock(&framework->shutdown.mutex);
    framework->shutdown.done = false;
    framework->shutdown.joined = false;
    framework->shutdown.initialized = false;
    celixThreadMutex_unlock(&framework->shutdown.mutex);


	celixThread_create(&framework->dispatcher.thread, NULL, fw_eventDispatcher, framework);
	celixThread_setName(&framework->dispatcher.thread, "CelixEvent");



    celix_status_t status = bundle_setState(framework->bundle, CELIX_BUNDLE_STATE_STARTING);
    if (status == CELIX_SUCCESS) {
        celix_bundle_activator_t *activator = calloc(1,(sizeof(*activator)));
        if (activator) {
            bundle_context_t *validateContext = NULL;

            activator->create = celix_frameworkBundle_create;
            activator->start = celix_frameworkBundle_start;
            activator->stop = celix_frameworkBundle_stop;
            activator->destroy = celix_frameworkBundle_destroy;
            status = CELIX_DO_IF(status, bundle_setActivator(framework->bundle, activator));
            if (status == CELIX_SUCCESS) {
                validateContext = celix_bundle_getContext(framework->bundle);
            }
            status = CELIX_DO_IF(status, activator->create(validateContext, &activator->userData));
            bool fwBundleCreated = status == CELIX_SUCCESS;
            status = CELIX_DO_IF(status, activator->start(activator->userData, validateContext));


            if (status != CELIX_SUCCESS) {
                if (fwBundleCreated) {
                    activator->destroy(activator->userData, validateContext);
                }
            	free(activator);
            }
        } else {
            status = CELIX_ENOMEM;
        }
    }

    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not init framework");
        celix_framework_stopAndJoinEventQueue(framework);
    }

	return status;
}

celix_status_t framework_start(celix_framework_t* framework) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state = celix_bundle_getState(framework->bundle);

    //framework_start should be called when state is INSTALLED or RESOLVED
    bool expectedState = state == CELIX_BUNDLE_STATE_INSTALLED || state == CELIX_BUNDLE_STATE_RESOLVED;

    if (!expectedState) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Could not start framework, unexpected state %i", state);
        return CELIX_ILLEGAL_STATE;
    }

    status = CELIX_DO_IF(status, fw_init(framework));
    status = CELIX_DO_IF(status, bundle_setState(framework->bundle, CELIX_BUNDLE_STATE_ACTIVE));

    if (status != CELIX_SUCCESS) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Could not initialize framework");
        return status;
    }

    celix_bundle_entry_t* entry =
        celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, framework->bundleId);
    fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, entry);
    celix_bundleEntry_decreaseUseCount(entry);

    bool allStarted = false;
    status = framework_autoStartConfiguredBundles(framework, &allStarted);
    bool allInstalled = false;
    status = CELIX_DO_IF(status, framework_autoInstallConfiguredBundles(framework, &allInstalled));

    if (status != CELIX_SUCCESS) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Celix framework failed to start");
        return status;
    }

    if (allStarted && allInstalled) {
        // fire started event if all bundles are started/installed and the event queue is empty
        celix_framework_waitForEmptyEventQueue(framework);
        fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_STARTED, CELIX_SUCCESS);
    } else {
        // note not returning an error, because the framework is started, but not all bundles are started/installed
        fw_logCode(
            framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not auto start or install all configured bundles");
        fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_ERROR, CELIX_BUNDLE_EXCEPTION);
    }

    fw_log(framework->logger, CELIX_LOG_LEVEL_INFO, "Celix framework started");
    fw_log(framework->logger,
           CELIX_LOG_LEVEL_TRACE,
           "Celix framework started with uuid %s",
           celix_framework_getUUID(framework));

    return CELIX_SUCCESS;
}

static celix_status_t framework_autoStartConfiguredBundles(celix_framework_t* fw, bool *startedAllBundles) {
    celix_status_t status = CELIX_SUCCESS;
    const char* const celixKeys[] = {CELIX_AUTO_START_0, CELIX_AUTO_START_1, CELIX_AUTO_START_2, CELIX_AUTO_START_3, CELIX_AUTO_START_4, CELIX_AUTO_START_5, CELIX_AUTO_START_6, NULL};
    celix_autoptr(celix_array_list_t) installedBundles = celix_arrayList_create();
    if (!installedBundles) {
        celix_framework_logTssErrors(fw->logger, CELIX_LOG_LEVEL_ERROR);
        return ENOMEM;
    }

    bool allStarted = true;
    for (int i = 0; celixKeys[i] != NULL; ++i) {
        const char *autoStart = celix_framework_getConfigProperty(fw, celixKeys[i], NULL, NULL);
        if (autoStart != NULL) {
            if (!framework_autoInstallConfiguredBundlesForList(fw, autoStart, installedBundles)) {
                allStarted = false;
            }
        }
    }
    if (!framework_autoStartConfiguredBundlesForList(fw, installedBundles)) {
        allStarted = false;
    }
    *startedAllBundles = allStarted;
    return status;
}

static celix_status_t framework_autoInstallConfiguredBundles(celix_framework_t* fw, bool* installedAllBundles) {
    *installedAllBundles = true;
    const char* autoInstall = celix_framework_getConfigProperty(fw, CELIX_AUTO_INSTALL, NULL, NULL);
    if (autoInstall != NULL) {
        *installedAllBundles = framework_autoInstallConfiguredBundlesForList(fw, autoInstall, NULL);
    }
    return CELIX_SUCCESS;
}

static bool framework_autoInstallConfiguredBundlesForList(celix_framework_t* fw,
                                                          const char* autoStartIn,
                                                          celix_array_list_t* installedBundles) {
    bool allInstalled = true;
    const char* delim = ",";
    char* save_ptr = NULL;
    char autoStartBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* autoStart = celix_utils_writeOrCreateString(autoStartBuffer, sizeof(autoStartBuffer), "%s", autoStartIn);
    celix_auto(celix_utils_string_guard_t) grd = celix_utils_stringGuard_init(autoStartBuffer, autoStart);
    if (autoStart != NULL) {
        char* location = strtok_r(autoStart, delim, &save_ptr);
        while (location != NULL) {
            // first install
            long id = -1L;
            if (celix_framework_installBundleInternal(fw, location, &id) == CELIX_SUCCESS) {
                if (installedBundles) {
                    celix_arrayList_addLong(installedBundles, id);
                }
            } else {
                fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Could not install bundle from location '%s'.", location);
                allInstalled = false;
            }
            location = strtok_r(NULL, delim, &save_ptr);
        }
    } else {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Could create auto install bundle list, out of memory.");
        allInstalled = false;
        // note out of memory, but framework can continue
    }
    return allInstalled;
}

static bool framework_autoStartConfiguredBundlesForList(celix_framework_t* fw,
                                                        const celix_array_list_t* installedBundles) {
    bool allStarted = true;
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));
    for (int i = 0; i < celix_arrayList_size(installedBundles); ++i) {
        long bndId = celix_arrayList_getLong(installedBundles, i);
        bundle_t* bnd = framework_getBundleById(fw, bndId);
        if (celix_bundle_getState(bnd) != OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            bool started = celix_framework_startBundle(fw, bndId);
            if (!started) {
                fw_log(fw->logger,
                       CELIX_LOG_LEVEL_ERROR,
                       "Could not start bundle %s (bnd id = %li)\n",
                       bnd->symbolicName,
                       bndId);
                allStarted = false;
            }
        } else {
            fw_log(fw->logger,
                   CELIX_LOG_LEVEL_WARNING,
                   "Cannot start bundle %s (bnd id = %li) again, already started\n",                   bnd->symbolicName,
                   bndId);
        }
    }
    return allStarted;
}

celix_status_t framework_stop(framework_pt framework) {
    bool stopped = celix_framework_stopBundle(framework, CELIX_FRAMEWORK_BUNDLE_ID);
    return stopped ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
}

const char* celix_framework_getConfigProperty(celix_framework_t* framework, const char* name, const char* defaultValue, bool* found) {
    const char* result = NULL;
    if (framework && name) {
        result = getenv(name); //NOTE that an env environment overrides the config.properties values
        if (result == NULL && framework->configurationMap != NULL) {
            result = celix_properties_get(framework->configurationMap, name, NULL);
        }
    }

    if (found) {
        *found = result != NULL;
    }
    result = result == NULL ? defaultValue : result;
    return result;
}

long celix_framework_getConfigPropertyAsLong(celix_framework_t* framework, const char* name, long defaultValue, bool* found) {
    bool strFound = false;
    bool strConverted = false;
    long result = defaultValue;
    const char *val = celix_framework_getConfigProperty(framework, name, NULL, &strFound);
    if (val != NULL) {
        result = celix_utils_convertStringToLong(val, defaultValue, &strConverted);
    }
    if (found) {
        *found = strFound && strConverted;
    }
    return result;
}

double celix_framework_getConfigPropertyAsDouble(celix_framework_t* framework, const char* name, double defaultValue, bool* found) {
    bool strFound = false;
    bool strConverted = false;
    double result = defaultValue;
    const char *val = celix_framework_getConfigProperty(framework, name, NULL, &strFound);
    if (val != NULL) {
        result = celix_utils_convertStringToDouble(val, defaultValue, &strConverted);
    }
    if (found) {
        *found = strFound && strConverted;
    }
    return result;
}

bool celix_framework_getConfigPropertyAsBool(celix_framework_t* framework, const char* name, bool defaultValue, bool* found) {
    bool strFound = false;
    bool strConverted = false;
    bool result = defaultValue;
    const char *val = celix_framework_getConfigProperty(framework, name, NULL, &strFound);
    if (val != NULL) {
        result = celix_utils_convertStringToBool(val, defaultValue, &strConverted);
    }
    if (found) {
        *found = strFound && strConverted;
    }
    return result;
}

static celix_status_t
celix_framework_installBundleInternalImpl(celix_framework_t* framework, const char* bndLoc, long* bndId) {
    bool valid = celix_framework_utils_isBundleUrlValid(framework, bndLoc, false);
    if (!valid) {
        return CELIX_FILE_IO_EXCEPTION;
    }

    // increase use count of framework bundle to prevent a stop.
    celix_bundle_entry_t* fwBundleEntry =
        celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, framework->bundleId);
    celix_auto(celix_bundle_entry_use_guard_t) fwEntryGuard = celix_bundleEntryUseGuard_init(fwBundleEntry);

    celix_bundle_state_e bndState = celix_bundle_getState(framework->bundle);
    if (bndState == CELIX_BUNDLE_STATE_STOPPING || bndState == CELIX_BUNDLE_STATE_UNINSTALLED) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_INFO,  "The framework is being shutdown");
        return CELIX_FRAMEWORK_SHUTDOWN;
    }

    long id;
    if (*bndId == -1L) {
        id = framework_getBundle(framework, bndLoc);
        if (id != -1L) {
            *bndId = id;
            return CELIX_SUCCESS;
        }
        long alreadyExistingBndId;
        celix_status_t status =
            celix_bundleCache_findBundleIdForLocation(framework->cache, bndLoc, &alreadyExistingBndId);
        if (status != CELIX_SUCCESS) {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not install bundle");
            return status;
        }
        id = alreadyExistingBndId == -1 ? framework_getNextBundleId(framework) : alreadyExistingBndId;
    } else {
        id = *bndId;
    }

    bundle_archive_t* archive = NULL;
    celix_bundle_t* bundle = NULL;
    celix_status_t status = celix_bundleCache_createArchive(framework->cache, id, bndLoc, &archive);
    if (status != CELIX_SUCCESS) {
        return status;
    }
    status = celix_bundle_createFromArchive(framework, archive, &bundle);
    if (status != CELIX_SUCCESS) {
        celix_bundleArchive_invalidate(archive);
        celix_bundleCache_destroyArchive(framework->cache, archive);
        return status;
    }

    celix_bundle_entry_t*bEntry = fw_bundleEntry_create(bundle);
    celix_bundleEntry_increaseUseCount(bEntry);
    celixThreadMutex_lock(&framework->installedBundles.mutex);
    celix_arrayList_add(framework->installedBundles.entries, bEntry);
    celixThreadMutex_unlock(&framework->installedBundles.mutex);
    fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED, bEntry);
    celix_bundleEntry_decreaseUseCount(bEntry);
    *bndId = id;
    return CELIX_SUCCESS;
}

celix_status_t
celix_framework_installBundleInternal(celix_framework_t* framework, const char* bndLoc, long* bndId) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&framework->installLock);
    status = celix_framework_installBundleInternalImpl(framework, bndLoc, bndId);
    celixThreadMutex_unlock(&framework->installLock);
    return status;
}

bool celix_framework_isBundleAlreadyInstalled(celix_framework_t* fw, const char* bundleSymbolicName) {
    bool alreadyExists = false;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (celix_utils_stringEquals(entry->bnd->symbolicName, bundleSymbolicName)) {
            alreadyExists = true;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return alreadyExists;
}

celix_status_t fw_registerService(framework_pt framework, service_registration_pt *registration, long bndId, const char* serviceName, const void* svcObj, celix_properties_t *properties) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
    if (serviceName == NULL || svcObj == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        error = "ServiceName and SvcObj cannot be null";
    }

    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework,
                                                                                               bndId);
    status = CELIX_DO_IF(status, serviceRegistry_registerService(framework->registry, entry->bnd, serviceName, svcObj, properties, registration));
    celix_bundleEntry_decreaseUseCount(entry);
    framework_logIfError(framework->logger, status, error, "Cannot register service: %s", serviceName);
    return status;
}

celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt *registration, long bndId, const char* serviceName, service_factory_pt factory,  celix_properties_t* properties) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
    if (serviceName == NULL || factory == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        error = "Service name and factory cannot be null";
    }

    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework,
                                                                                                          bndId);

    status = CELIX_DO_IF(status, serviceRegistry_registerServiceFactory(framework->registry, entry->bnd, serviceName, factory, properties, registration));

    celix_bundleEntry_decreaseUseCount(entry);

    framework_logIfError(framework->logger, status, error, "Cannot register service factory: %s", serviceName);

    return status;
}

celix_status_t fw_getServiceReferences(framework_pt framework, celix_array_list_t** references, bundle_pt bundle, const char * serviceName, const char * sfilter) {
    celix_status_t status = CELIX_SUCCESS;

    celix_autoptr(celix_filter_t) filter = NULL;
    int refIdx = 0;

    if (sfilter != NULL) {
        filter = celix_filter_create(sfilter);
    }

    status = CELIX_DO_IF(status, serviceRegistry_getServiceReferences(framework->registry, bundle, serviceName, filter, references));


    if (status == CELIX_SUCCESS) {
        for (refIdx = 0; (*references != NULL) && refIdx < celix_arrayList_size(*references); refIdx++) {
            service_reference_pt ref = (service_reference_pt) celix_arrayList_get(*references, refIdx);
            service_registration_pt reg = NULL;
            const char* serviceNameObjectClass;
            celix_properties_t* props = NULL;
            status = CELIX_DO_IF(status, serviceReference_getServiceRegistration(ref, &reg));
            status = CELIX_DO_IF(status, serviceRegistration_getProperties(reg, &props));
            if (status == CELIX_SUCCESS) {
                serviceNameObjectClass = celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_NAME, NULL);
                if (!serviceReference_isAssignableTo(ref, bundle, serviceNameObjectClass)) {
                    serviceReference_release(ref, NULL);
                    celix_arrayList_removeAt(*references, refIdx);
                    refIdx--;
                }
            }
        }
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to get service references");

    return status;
}

celix_status_t fw_getBundleRegisteredServices(framework_pt framework, bundle_pt bundle, celix_array_list_t** services) {
	return serviceRegistry_getRegisteredServices(framework->registry, bundle, services);
}

celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, celix_array_list_t* *services) {
	celix_status_t status = CELIX_SUCCESS;
	status = serviceRegistry_getServicesInUse(framework->registry, bundle, services);
	return status;
}

void fw_addServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener, const char* sfilter) {
    celix_serviceRegistry_addServiceListener(framework->registry, bundle, sfilter, listener);
}

void fw_removeServiceListener(framework_pt framework, bundle_pt bundle CELIX_UNUSED, celix_service_listener_t *listener) {
    celix_serviceRegistry_removeServiceListener(framework->registry, listener);
}


celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
    typedef struct {
        celix_bundle_entry_t* entry;
        celix_bundle_state_e currentState;
    } installed_bundle_entry_t;


    celix_status_t status = CELIX_SUCCESS;
    fw_bundle_listener_pt bundleListener = calloc(1, sizeof(*bundleListener));
    bundleListener->listener = listener;
    bundleListener->bundle = bundle;
    celixThreadMutex_create(&bundleListener->useMutex, NULL);
    celixThreadCondition_init(&bundleListener->useCond, NULL);
    bundleListener->useCount = 1;
    celix_array_list_t* installedBundles = celix_arrayList_create();

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    int size = celix_arrayList_size(framework->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celix_bundleEntry_increaseUseCount(entry);
        installed_bundle_entry_t* installedEntry = calloc(1, sizeof(*installedEntry));
        installedEntry->entry = entry;
        installedEntry->currentState = celix_bundle_getState(entry->bnd);
        celix_arrayList_add(installedBundles, installedEntry);
    }
    celixThreadMutex_lock(&framework->bundleListenerLock);
    celix_arrayList_add(framework->bundleListeners, bundleListener);
    celixThreadMutex_unlock(&framework->bundleListenerLock);
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    //Calling bundle events for already installed bundles.
    for (int i =0 ; i < celix_arrayList_size(installedBundles); ++i) {
        installed_bundle_entry_t* installedEntry = celix_arrayList_get(installedBundles, i);
        celix_bundle_event_t event;
        event.bnd = installedEntry->entry->bnd;
        event.bundleSymbolicName = (char*)celix_bundle_getSymbolicName(installedEntry->entry->bnd);

        celix_bundle_state_e state = installedEntry->currentState;

        //note assuming bundle state values are increasing!
        if (state >= CELIX_BUNDLE_STATE_INSTALLED) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED;
            listener->bundleChanged(listener, &event);
        }
        if (state >= CELIX_BUNDLE_STATE_RESOLVED) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED;
            listener->bundleChanged(listener, &event);
        }
        if (state >= CELIX_BUNDLE_STATE_ACTIVE) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED;
            listener->bundleChanged(listener, &event);
        }
        celix_bundleEntry_decreaseUseCount(installedEntry->entry);
        free(installedEntry);
    }
    fw_bundleListener_decreaseUseCount(bundleListener);
    celix_arrayList_destroy(installedBundles);

    return status;
}

celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    fw_bundle_listener_pt bundleListener = NULL;

    celixThreadMutex_lock(&framework->bundleListenerLock);
    for (int i = 0; i < celix_arrayList_size(framework->bundleListeners); i++) {
        bundleListener = celix_arrayList_get(framework->bundleListeners, i);
        if (bundleListener->listener == listener && bundleListener->bundle == bundle) {
            celix_arrayList_removeAt(framework->bundleListeners, i);
            break;
        }
    }
    celixThreadMutex_unlock(&framework->bundleListenerLock);

    if (bundleListener != NULL) {
        fw_bundleListener_destroy(bundleListener, true);
    } else {
        framework_logIfError(framework->logger, status, NULL, "Failed to remove bundle listener");
    }


    return status;
}

celix_status_t fw_addFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;
    fw_framework_listener_pt frameworkListener = NULL;

    frameworkListener = (fw_framework_listener_pt) malloc(sizeof(*frameworkListener));
    if (!frameworkListener) {
        status = CELIX_ENOMEM;
    } else {
        frameworkListener->listener = listener;
        frameworkListener->bundle = bundle;

        celixThreadMutex_lock(&framework->frameworkListenersLock);
        celix_arrayList_add(framework->frameworkListeners, frameworkListener);
        celixThreadMutex_unlock(&framework->frameworkListenersLock);
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to add framework listener");

    return status;
}

celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
    int i;
    fw_framework_listener_pt frameworkListener;

    celixThreadMutex_lock(&framework->frameworkListenersLock);
    for (i = 0; i < celix_arrayList_size(framework->frameworkListeners); i++) {
        frameworkListener = (fw_framework_listener_pt) celix_arrayList_get(framework->frameworkListeners, i);
        if (frameworkListener->listener == listener && frameworkListener->bundle == bundle) {
            celix_arrayList_removeAt(framework->frameworkListeners, i);


            free(frameworkListener);
        }
    }
    celixThreadMutex_unlock(&framework->frameworkListenersLock);
    return CELIX_SUCCESS;
}

long framework_getNextBundleId(framework_pt framework) {
    long nextId = __atomic_fetch_add(&framework->currentBundleId, 1, __ATOMIC_SEQ_CST);
    while ( celix_bundleCache_isBundleIdAlreadyUsed(framework->cache, nextId) ||
            celix_framework_isBundleIdAlreadyUsed(framework, nextId)) {
        nextId = __atomic_fetch_add(&framework->currentBundleId, 1, __ATOMIC_SEQ_CST);
    }
    return nextId;
}

static celix_status_t framework_markBundleResolved(framework_pt framework, celix_module_t* module) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_pt bundle = module_getBundle(module);
    bundle_state_e state;

    if (bundle != NULL) {
        long bndId = celix_bundle_getId(bundle);
        celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework,bndId);

        bundle_getState(bundle, &state);
        if (state != CELIX_BUNDLE_STATE_INSTALLED) {
            fw_log(framework->logger, CELIX_LOG_LEVEL_WARNING, "Trying to resolve a resolved bundle");
            status = CELIX_ILLEGAL_STATE;
        } else {
            // Load libraries of this module
            bool isSystemBundle = false;
            bundle_isSystemBundle(bundle, &isSystemBundle);
            if (!isSystemBundle) {
                status = CELIX_DO_IF(status, celix_module_loadLibraries(module));
            }

            status = CELIX_DO_IF(status, bundle_setState(bundle, CELIX_BUNDLE_STATE_RESOLVED));
            CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED, entry));
        }

        if (status != CELIX_SUCCESS) {
            const char *symbolicName = NULL;
            long id = 0;
            module_getSymbolicName(module, &symbolicName);
            bundle_getBundleId(bundle, &id);
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not resolve bundle: %s [%ld]", symbolicName, id);
        }

        celix_bundleEntry_decreaseUseCount(entry);
    }

    return status;
}

celix_array_list_t* framework_getBundles(framework_pt framework) {
    //FIXME Note that this does not increase the use count of the bundle, which can lead to race conditions.
    //promote to use the celix_bundleContext_useBundle(s) functions and deprecated this one
    celix_array_list_t* bundles = celix_arrayList_create();

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    int size = celix_arrayList_size(framework->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celix_arrayList_add(bundles, entry->bnd);
    }
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    return bundles;
}

long framework_getBundle(framework_pt framework, const char* location) {
    long id = -1L;

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    int size = celix_arrayList_size(framework->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(framework->installedBundles.entries, i);
        const char *loc = NULL;
        bundle_getBundleLocation(entry->bnd, &loc);
        if (loc != NULL && location != NULL && strncmp(loc, location, strlen(loc)) == 0) {
            id = entry->bndId;
            break;
        }
    }
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    return id;
}

celix_status_t framework_waitForStop(framework_pt framework) {
    celix_framework_waitForStop(framework);
    return CELIX_SUCCESS;
}

static void celix_framework_stopAndJoinEventQueue(celix_framework_t* fw) {
    fw_log(fw->logger,
           CELIX_LOG_LEVEL_TRACE,
           "Stop and joining event loop thread for framework %s",
           celix_framework_getUUID(fw));
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    fw->dispatcher.active = false;
    celixThreadCondition_broadcast(&fw->dispatcher.cond);
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    celixThread_join(fw->dispatcher.thread, NULL);
    fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG, "Joined event loop thread for framework %s", celix_framework_getUUID(fw));
}

static void* framework_shutdown(void *framework) {
    framework_pt fw = (framework_pt) framework;

    fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Celix framework shutting down");

    celix_framework_waitForBundleLifecycleHandlers(fw);

    celix_array_list_t *stopEntries = celix_arrayList_create();
    celix_bundle_entry_t*fwEntry = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    int size = celix_arrayList_size(fw->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        celix_bundleEntry_increaseUseCount(entry);
        if (entry->bndId != 0) { //i.e. not framework bundle
            celix_arrayList_add(stopEntries, entry);
        } else {
            fwEntry = entry;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);

    size = celix_arrayList_size(stopEntries);
    for (int i = size-1; i >= 0; --i) { //note loop in reverse order -> uninstall later installed bundle first
        celix_bundle_entry_t*entry = celix_arrayList_get(stopEntries, i);
        celix_framework_uninstallBundleEntry(fw, entry, false);
    }
    celix_arrayList_destroy(stopEntries);


    // make sure the framework has been stopped
    if (fwEntry != NULL) {
        // Lock the mutex to make sure that `celix_framework_stopBundleEntryInternal` on the framework has finished.
        celixThreadRwlock_readLock(&fwEntry->fsmMutex);
        celixThreadRwlock_unlock(&fwEntry->fsmMutex);
        celix_bundleEntry_decreaseUseCount(fwEntry);
    }
    //Now that all bundled has been stopped, no more events will be sent, we can safely stop the event dispatcher.
    celix_framework_stopAndJoinEventQueue(fw);

    celixThreadMutex_lock(&fw->shutdown.mutex);
    fw->shutdown.done = true;
    celixThreadCondition_broadcast(&fw->shutdown.cond);
    celixThreadMutex_unlock(&fw->shutdown.mutex);

    celixThread_exit(NULL);
    return NULL;
}

void celix_framework_shutdownAsync(celix_framework_t* framework) {
    fw_log(framework->logger,
           CELIX_LOG_LEVEL_TRACE,
           "Start shutdown thread for framework %s",
           celix_framework_getUUID(framework));
    celixThreadMutex_lock(&framework->shutdown.mutex);
    bool alreadyInitialized = framework->shutdown.initialized;
    framework->shutdown.initialized = true;
    celixThreadMutex_unlock(&framework->shutdown.mutex);

    if (!alreadyInitialized) {
        celixThread_create(&framework->shutdown.thread, NULL, framework_shutdown, framework);
    }
}

celix_status_t framework_getFrameworkBundle(const_framework_pt framework, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;

    if (framework != NULL && *bundle == NULL) {
        *bundle = framework->bundle;
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

bundle_context_t* framework_getContext(const_framework_pt framework) {
    bundle_context_t *result = NULL;
    if (framework != NULL && framework->bundle != NULL) {
        result = framework->bundle->context;
    }
    return result;
}

void fw_fireBundleEvent(framework_pt framework, bundle_event_type_e eventType, celix_bundle_entry_t* entry) {
    if (eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING || eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED || eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED) {
        if (entry->bndId == framework->bundleId) {
            //NOTE for framework bundle not triggering events while framework is stopped (and as result in use)
            return;
        }
    }

    celix_bundleEntry_increaseUseCount(entry);

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_BUNDLE_EVENT_TYPE;
    event.bndEntry = entry;
    event.bundleEvent = eventType;
    __atomic_add_fetch(&framework->dispatcher.stats.nbBundle, 1, __ATOMIC_RELAXED);
    celix_framework_addToEventQueue(framework, &event);
}

void fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, celix_status_t errorCode) {
    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_FRAMEWORK_EVENT_TYPE;
    event.fwEvent = eventType;
    event.errorCode = errorCode;
    event.error = "";
    if (errorCode != CELIX_SUCCESS) {
        event.error = celix_strerror(errorCode);
    }

    __atomic_add_fetch(&framework->dispatcher.stats.nbFramework, 1, __ATOMIC_RELAXED);
    celix_framework_addToEventQueue(framework, &event);
}

static void celix_framework_addToEventQueue(celix_framework_t *fw, const celix_framework_event_t* event) {
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    //try to add to static queue
    if (celix_arrayList_size(fw->dispatcher.dynamicEventQueue) > 0) { //always to dynamic queue if not empty (to ensure order)
        celix_framework_event_t *e = malloc(sizeof(*e));
        *e = *event; //shallow copy
        celix_arrayList_add(fw->dispatcher.dynamicEventQueue, e);
        if (celix_arrayList_size(fw->dispatcher.dynamicEventQueue) % 100 == 0) {
            fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING, "dynamic event queue size is %i. Is there a bundle blocking on the event loop thread?", celix_arrayList_size(fw->dispatcher.dynamicEventQueue));
        }
    } else if (fw->dispatcher.eventQueueSize < fw->dispatcher.eventQueueCap) {
        size_t index = (fw->dispatcher.eventQueueFirstEntry + fw->dispatcher.eventQueueSize) %
                       fw->dispatcher.eventQueueCap;
        fw->dispatcher.eventQueue[index] = *event; //shallow copy
        fw->dispatcher.eventQueueSize += 1;
    } else {
        //static queue is full, dynamics queue is empty. Add first entry to dynamic queue
        fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING,
               "Static event queue for celix framework is full, falling back to dynamic allocated events. Increase static event queue size, current size is %i", fw->dispatcher.eventQueueCap);
        celix_framework_event_t *e = malloc(sizeof(*e));
        *e = *event; //shallow copy
        celix_arrayList_add(fw->dispatcher.dynamicEventQueue, e);
    }
    celixThreadCondition_broadcast(&fw->dispatcher.cond);
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

static void fw_handleEventRequest(celix_framework_t *framework, celix_framework_event_t* event) {
    if (event->type == CELIX_BUNDLE_EVENT_TYPE) {
        celix_array_list_t *localListeners = celix_arrayList_create();
        celixThreadMutex_lock(&framework->bundleListenerLock);
        for (int i = 0; i < celix_arrayList_size(framework->bundleListeners); ++i) {
            fw_bundle_listener_pt listener = celix_arrayList_get(framework->bundleListeners, i);
            fw_bundleListener_increaseUseCount(listener);
            celix_arrayList_add(localListeners, listener);
        }
        celixThreadMutex_unlock(&framework->bundleListenerLock);
        for (int i = 0; i < celix_arrayList_size(localListeners); ++i) {
            fw_bundle_listener_pt listener = celix_arrayList_get(localListeners, i);

            bundle_event_t bEvent;
            memset(&bEvent, 0, sizeof(bEvent));
            bEvent.bnd = event->bndEntry->bnd;
            bEvent.type = event->bundleEvent;
            fw_invokeBundleListener(framework, listener->listener, &bEvent, listener->bundle);

            fw_bundleListener_decreaseUseCount(listener);
        }
        celix_arrayList_destroy(localListeners);
        __atomic_sub_fetch(&framework->dispatcher.stats.nbBundle, 1, __ATOMIC_RELAXED);
    } else if (event->type == CELIX_FRAMEWORK_EVENT_TYPE) {
        celixThreadMutex_lock(&framework->frameworkListenersLock);
        for (int i = 0; i < celix_arrayList_size(framework->frameworkListeners); ++i) {
            fw_framework_listener_pt listener = celix_arrayList_get(framework->frameworkListeners, i);
            framework_event_t fEvent;
            memset(&fEvent, 0, sizeof(fEvent));
            fEvent.type = event->fwEvent;
            fEvent.error = event->error;
            fEvent.errorCode = event->errorCode;

            fw_invokeFrameworkListener(framework, listener->listener, &fEvent, listener->bundle);
        }
        celixThreadMutex_unlock(&framework->frameworkListenersLock);
        __atomic_sub_fetch(&framework->dispatcher.stats.nbFramework, 1, __ATOMIC_RELAXED);
    } else if (event->type == CELIX_REGISTER_SERVICE_EVENT) {
        service_registration_t* reg = NULL;
        celix_status_t status = CELIX_SUCCESS;
        if (event->cancelled) {
            fw_log(framework->logger, CELIX_LOG_LEVEL_DEBUG, "CELIX_REGISTER_SERVICE_EVENT for svcId %li (service name = %s) was cancelled. Skipping registration", event->registerServiceId, event->serviceName);
            celix_properties_destroy(event->properties);
        } else if (event->factory != NULL) {
            status = celix_serviceRegistry_registerServiceFactory(framework->registry, event->bndEntry->bnd, event->serviceName, event->factory, event->properties, event->registerServiceId, &reg);
        } else {
            status = celix_serviceRegistry_registerService(framework->registry, event->bndEntry->bnd, event->serviceName, event->svc, event->properties, event->registerServiceId, &reg);
        }
        if (status != CELIX_SUCCESS) {
            fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Could not register service async. svc name is %s, error is %s", event->serviceName, celix_strerror(status));
        } else if (!event->cancelled && event->registerCallback != NULL) {
            event->registerCallback(event->registerData, serviceRegistration_getServiceId(reg));
        }
        __atomic_sub_fetch(&framework->dispatcher.stats.nbRegister, 1, __ATOMIC_RELAXED);
    } else if (event->type == CELIX_UNREGISTER_SERVICE_EVENT) {
        celix_serviceRegistry_unregisterService(framework->registry, event->bndEntry->bnd, event->unregisterServiceId);
        __atomic_sub_fetch(&framework->dispatcher.stats.nbUnregister, 1, __ATOMIC_RELAXED);
    } else if (event->type == CELIX_GENERIC_EVENT) {
        if (event->genericProcess != NULL) {
            event->genericProcess(event->genericProcessData);
        }
        __atomic_sub_fetch(&framework->dispatcher.stats.nbEvent, 1, __ATOMIC_RELAXED);
    }

    if (event->doneCallback != NULL && !event->cancelled) {
        event->doneCallback(event->doneData);
    }
}

static inline celix_framework_event_t* fw_topEventFromQueue(celix_framework_t* fw) {
    celix_framework_event_t* e = NULL;
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    if (fw->dispatcher.eventQueueSize > 0) {
        e = &fw->dispatcher.eventQueue[fw->dispatcher.eventQueueFirstEntry];
    } else if (celix_arrayList_size(fw->dispatcher.dynamicEventQueue) > 0) {
        e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, 0);
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return e;
}

static inline bool fw_removeTopEventFromQueue(celix_framework_t* fw) {
    bool dynamicallyAllocated = false;
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    if (fw->dispatcher.eventQueueSize > 0) {
        fw->dispatcher.eventQueueFirstEntry = (fw->dispatcher.eventQueueFirstEntry+1) % fw->dispatcher.eventQueueCap;
        fw->dispatcher.eventQueueSize -= 1;
    } else if (celix_arrayList_size(fw->dispatcher.dynamicEventQueue) > 0) {
        celix_arrayList_removeAt(fw->dispatcher.dynamicEventQueue, 0);
        dynamicallyAllocated = true;
    }
    celixThreadCondition_broadcast(&fw->dispatcher.cond); //notify that the queue size is changed
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return dynamicallyAllocated;
}


static inline void fw_handleEvents(celix_framework_t* framework) {
    celixThreadMutex_lock(&framework->dispatcher.mutex);
    int size = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    while (size > 0) {
        celix_framework_event_t* topEvent = fw_topEventFromQueue(framework);
        fw_handleEventRequest(framework, topEvent);
        bool dynamicallyAllocatedEvent = fw_removeTopEventFromQueue(framework);

        if (topEvent->bndEntry != NULL) {
            celix_bundleEntry_decreaseUseCount(topEvent->bndEntry);
        }
        free(topEvent->serviceName);
        if (dynamicallyAllocatedEvent) {
            free(topEvent);
        }

        celixThreadMutex_lock(&framework->dispatcher.mutex);
        size = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }
}

/**
 * @brief Process all scheduled events.
 */
static void celix_framework_processScheduledEvents(celix_framework_t* fw) {
    struct timespec scheduleTime = celixThreadCondition_getTime();
    celix_scheduled_event_t* callEvent;
    celix_scheduled_event_t* removeEvent;
    do {
        callEvent = NULL;
        removeEvent = NULL;
        celixThreadMutex_lock(&fw->dispatcher.mutex);
        CELIX_LONG_HASH_MAP_ITERATE(fw->dispatcher.scheduledEvents, entry) {
            celix_scheduled_event_t* visit = entry.value.ptrValue;
            if (!fw->dispatcher.active || celix_scheduledEvent_isMarkedForRemoval(visit)) {
                removeEvent = visit;
                celix_longHashMap_remove(fw->dispatcher.scheduledEvents, celix_scheduledEvent_getId(visit));
                break;
            }

            bool call = celix_scheduledEvent_deadlineReached(visit, &scheduleTime);
            if (call) {
                callEvent = visit;
                if (celix_scheduledEvent_isSingleShot(visit)) {
                    removeEvent = visit;
                    celix_longHashMap_remove(fw->dispatcher.scheduledEvents, celix_scheduledEvent_getId(visit));
                }
                break;
            }
        }
        celixThreadMutex_unlock(&fw->dispatcher.mutex);

        if (callEvent != NULL) {
            celix_scheduledEvent_process(callEvent);
        }
        if (removeEvent != NULL) {
            fw_log(fw->logger,
                   CELIX_LOG_LEVEL_DEBUG,
                   "Removing processed %s""scheduled event '%s' (id=%li) for bundle if %li.",
                   celix_scheduledEvent_isSingleShot(removeEvent) ? "one-shot " : "",
                   celix_scheduledEvent_getName(removeEvent),
                   celix_scheduledEvent_getId(removeEvent),
                   celix_scheduledEvent_getBundleId(removeEvent));
            celix_scheduledEvent_setRemoved(removeEvent);
            celix_scheduledEvent_release(removeEvent);
        }
    } while (callEvent || removeEvent);
}

/**
 * @brief Calculate the next deadline for scheduled events.
 * @return The next deadline or 1 second delayed timespec if no events are scheduled.
 */
static struct timespec celix_framework_nextDeadlineForEventsWait(celix_framework_t* framework) {
    bool closestDeadlineSet = false;
    struct timespec closestDeadline = {0,0};

    celixThreadMutex_lock(&framework->dispatcher.mutex);
    CELIX_LONG_HASH_MAP_ITERATE(framework->dispatcher.scheduledEvents, entry) {
        celix_scheduled_event_t *visit = entry.value.ptrValue;
        struct timespec eventDeadline = celix_scheduledEvent_getNextDeadline(visit);
        if (!closestDeadlineSet) {
            closestDeadline = eventDeadline;
            closestDeadlineSet = true;
        } else if (celix_compareTime(&eventDeadline, &closestDeadline) < 0) {
            closestDeadline = eventDeadline;
        }
    }
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    struct timespec fallbackDeadline = celixThreadCondition_getDelayedTime(1); //max 1 second wait
    return closestDeadlineSet ? closestDeadline : fallbackDeadline;
}

void celix_framework_cleanupScheduledEvents(celix_framework_t* fw, long bndId) {
    celix_scheduled_event_t* removeEvent;
    do {
        removeEvent = NULL;
        celixThreadMutex_lock(&fw->dispatcher.mutex);
        CELIX_LONG_HASH_MAP_ITERATE(fw->dispatcher.scheduledEvents, entry) {
            celix_scheduled_event_t* visit = entry.value.ptrValue;
            if (bndId == celix_scheduledEvent_getBundleId(visit)) {
                removeEvent = visit;
                celix_scheduledEvent_retain(removeEvent);
                if (!celix_scheduledEvent_isSingleShot(removeEvent)) {
                    fw_log(fw->logger,
                           CELIX_LOG_LEVEL_WARNING,
                           "Removing dangling scheduled event '%s' (id=%li) for bundle id %li. This scheduled event should "
                           "have been removed up by the bundle.",
                           celix_scheduledEvent_getName(removeEvent),
                           celix_scheduledEvent_getId(removeEvent),
                           celix_scheduledEvent_getBundleId(removeEvent));
                }
                celix_scheduledEvent_markForRemoval(removeEvent);
                celixThreadCondition_broadcast(&fw->dispatcher.cond); //notify that scheduled event is marked for removal
                break;
            }
        }
        celixThreadMutex_unlock(&fw->dispatcher.mutex);

        if (removeEvent) {
            celix_scheduledEvent_waitForRemoved(removeEvent);
            celix_scheduledEvent_release(removeEvent);
        }
    } while (removeEvent != NULL);
}

static int celix_framework_eventQueueSize(celix_framework_t* fw) {
    //precondition fw->dispatcher.mutex locked);
    return fw->dispatcher.eventQueueSize + celix_arrayList_size(fw->dispatcher.dynamicEventQueue);
}

bool celix_framework_isEventQueueEmpty(celix_framework_t* fw) {
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool empty = celix_framework_eventQueueSize(fw) == 0;
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return empty;
}

static bool requiresScheduledEventsProcessing(celix_framework_t* framework) {
    // precondition framework->dispatcher.mutex locked
    struct timespec currentTime = celixThreadCondition_getTime();
    bool eventProcessingRequired = false;
    CELIX_LONG_HASH_MAP_ITERATE(framework->dispatcher.scheduledEvents, mapEntry) {
        celix_scheduled_event_t* visit = mapEntry.value.ptrValue;
        if (celix_scheduledEvent_requiresProcessing(visit, &currentTime)) {
            eventProcessingRequired = true;
            break;
        }
    }
    return eventProcessingRequired;
}

static void celix_framework_waitForNextEvent(celix_framework_t* fw, struct timespec nextDeadline) {
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    if (celix_framework_eventQueueSize(fw) == 0 && !requiresScheduledEventsProcessing(fw) && fw->dispatcher.active) {
        celixThreadCondition_waitUntil(&fw->dispatcher.cond, &fw->dispatcher.mutex, &nextDeadline);
        // note failing through to fw_eventDispatcher even if timeout is not reached, the fw_eventDispatcher
        // will call this again after processing the events and scheduled events.
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

static void *fw_eventDispatcher(void *fw) {
    framework_pt framework = (framework_pt) fw;

    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool active = framework->dispatcher.active;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    while (active) {
        fw_handleEvents(framework);
        celix_framework_processScheduledEvents(framework);
        struct timespec nextDeadline = celix_framework_nextDeadlineForEventsWait(framework);
        celix_framework_waitForNextEvent(framework, nextDeadline);

        celixThreadMutex_lock(&framework->dispatcher.mutex);
        active = framework->dispatcher.active;
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }

    //not active anymore, extra runs for possible request leftovers
    celix_framework_processScheduledEvents(framework);
    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool needExtraRun = celix_framework_eventQueueSize(fw) > 0;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);
    while (needExtraRun) {
        fw_handleEvents(framework);
        celixThreadMutex_lock(&framework->dispatcher.mutex);
        needExtraRun = celix_framework_eventQueueSize(fw) > 0;
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }

    celixThread_exit(NULL);
    return NULL;

}

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle) {
    // We only support async bundle listeners for now
    bundle_state_e state;
    celix_status_t ret = bundle_getState(bundle, &state);
    if (state == CELIX_BUNDLE_STATE_STARTING || state == CELIX_BUNDLE_STATE_ACTIVE) {

        listener->bundleChanged(listener, event);
    }

    return ret;
}

celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle) {
    bundle_state_e state;
    celix_status_t ret = bundle_getState(bundle, &state);
    if (state == CELIX_BUNDLE_STATE_STARTING || state == CELIX_BUNDLE_STATE_ACTIVE) {
        listener->frameworkEvent(listener, event);
    }

    return ret;
}

/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

static size_t celix_framework_useBundlesInternal(framework_t *fw, bool includeFrameworkBundle, bool onlyActive, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    size_t count = 0;
    celix_array_list_t *bundleIds = celix_arrayList_create();

    celixThreadMutex_lock(&fw->installedBundles.mutex);
    int size = celix_arrayList_size(fw->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_bundle_entry_t*entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId > 0 || includeFrameworkBundle) {
            celix_arrayList_addLong(bundleIds, entry->bndId);
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);

    // note that stored bundle ids can now already be invalid, but the celix_framework_useBundle function should be
    // able to handle this safely.

    size = celix_arrayList_size(bundleIds);
    for (int i = 0; i < size; ++i) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        bool called = celix_framework_useBundle(fw, onlyActive, bndId, callbackHandle, use);
        if (called) {
            ++count;
        }
    }

    celix_arrayList_destroy(bundleIds);
    return count;

}

size_t celix_framework_useBundles(framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    return celix_framework_useBundlesInternal(fw, includeFrameworkBundle, false, callbackHandle, use);
}

size_t celix_framework_useActiveBundles(framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    return celix_framework_useBundlesInternal(fw, includeFrameworkBundle, true, callbackHandle, use);
}

bool celix_framework_useBundle(framework_t *fw, bool onlyActive, long bundleId, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    bool called = false;
    if (bundleId >= 0) {
        celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw,
                                                                                                              bundleId);
        if (entry != NULL) {
            if (onlyActive) {
                celixThreadRwlock_readLock(&entry->fsmMutex);
            }
            celix_bundle_state_e bndState = celix_bundle_getState(entry->bnd);
            if (onlyActive && (bndState == CELIX_BUNDLE_STATE_ACTIVE || bndState == CELIX_BUNDLE_STATE_STARTING)) {
                use(callbackHandle, entry->bnd);
                called = true;
            } else if (!onlyActive) {
                use(callbackHandle, entry->bnd);
                called = true;
            }
            if (onlyActive) {
                celixThreadRwlock_unlock(&entry->fsmMutex);
            }
            celix_bundleEntry_decreaseUseCount(entry);
        } else {
            framework_logIfError(fw->logger, CELIX_FRAMEWORK_EXCEPTION, NULL, "Bundle with id %li is not installed", bundleId);
        }
    }
    return called;
}

long celix_framework_registerService(framework_t *fw, celix_bundle_t *bnd, const char* serviceName, void* svc, celix_service_factory_t *factory, celix_properties_t *properties) {
    const char *error = NULL;
    celix_status_t status;
    service_registration_t *reg = NULL;

    if (serviceName == NULL) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Null serviceName");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    long bndId = celix_bundle_getId(bnd);
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);


    if (factory != NULL) {
        status = celix_serviceRegistry_registerServiceFactory(fw->registry, bnd, serviceName, factory, properties, 0, &reg);
    } else {
        status = celix_serviceRegistry_registerService(fw->registry, bnd, serviceName, svc, properties, 0, &reg);
    }

    celix_bundleEntry_decreaseUseCount(entry);

    framework_logIfError(fw->logger, status, error, "Cannot register %s '%s'", factory == NULL ? "service" : "service factory", serviceName);

    return serviceRegistration_getServiceId(reg);
}

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
        void (*eventDoneCallback)(void* eventDoneData)) {
    long bndId = celix_bundle_getId(bnd);
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);

    long svcId = celix_serviceRegistry_nextSvcId(fw->registry);

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_REGISTER_SERVICE_EVENT;
    event.bndEntry = entry;
    event.registerServiceId = svcId;
    event.serviceName = celix_utils_strdup(serviceName);
    event.properties = properties;
    event.svc = svc;
    event.factory = factory;
    event.registerData = registerDoneData;
    event.registerCallback = registerDoneCallback;
    event.doneData = eventDoneData;
    event.doneCallback = eventDoneCallback;
    __atomic_add_fetch(&fw->dispatcher.stats.nbRegister, 1, __ATOMIC_RELAXED);
    celix_framework_addToEventQueue(fw, &event);

    return svcId;
}

void celix_framework_unregisterAsync(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId, void *doneData, void (*doneCallback)(void*)) {
    long bndId = celix_bundle_getId(bnd);
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_UNREGISTER_SERVICE_EVENT;
    event.bndEntry = entry;
    event.unregisterServiceId = serviceId;
    event.doneData = doneData;
    event.doneCallback = doneCallback;

    __atomic_add_fetch(&fw->dispatcher.stats.nbUnregister, 1, __ATOMIC_RELAXED);
    celix_framework_addToEventQueue(fw, &event);
}

/**
 * Checks if there is a pending service registration in the event queue and canels this.
 *
 * This can be needed when a service is regsitered async and still on the event queue when an sync unregistration
 * is made.
 * @returns true if a service registration is cancelled.
 */
static bool celix_framework_cancelServiceRegistrationIfPending(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId) {
    bool cancelled = false;
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
        celix_framework_event_t *event = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
        if (event->type == CELIX_REGISTER_SERVICE_EVENT && event->registerServiceId == serviceId) {
            event->cancelled = true;
            cancelled = true;
            break;
        }
    }
    for (size_t i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
        size_t index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
        celix_framework_event_t *event = &fw->dispatcher.eventQueue[index];
        if (event->type == CELIX_REGISTER_SERVICE_EVENT && event->registerServiceId == serviceId) {
            event->cancelled = true;
            cancelled = true;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return cancelled;
}

void celix_framework_unregister(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId) {
    if (!celix_framework_cancelServiceRegistrationIfPending(fw, bnd, serviceId)) {
        celix_serviceRegistry_unregisterService(fw->registry, bnd, serviceId);
    }
}

void celix_framework_waitForAsyncRegistration(framework_t *fw, long svcId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool registrationsInProgress = true;
    while (registrationsInProgress) {
        registrationsInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if (e->type == CELIX_REGISTER_SERVICE_EVENT && e->registerServiceId == svcId) {
                registrationsInProgress = true;
                break;
            }
        }
        for (int i = 0; !registrationsInProgress && i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if (e->type == CELIX_REGISTER_SERVICE_EVENT && e->registerServiceId == svcId) {
                registrationsInProgress = true;
                break;
            }
        }
        if (registrationsInProgress) {
            celixThreadCondition_timedwaitRelative(&fw->dispatcher.cond, &fw->dispatcher.mutex, 5, 0);
        }
    }

    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

void celix_framework_waitForAsyncUnregistration(framework_t *fw, long svcId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool registrationsInProgress = true;
    while (registrationsInProgress) {
        registrationsInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if (e->type == CELIX_UNREGISTER_SERVICE_EVENT && e->unregisterServiceId == svcId) {
                registrationsInProgress = true;
                break;
            }
        }
        for (int i = 0; !registrationsInProgress && i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if (e->type == CELIX_UNREGISTER_SERVICE_EVENT && e->unregisterServiceId == svcId) {
                registrationsInProgress = true;
                break;
            }
        }
        if (registrationsInProgress) {
            celixThreadCondition_timedwaitRelative(&fw->dispatcher.cond, &fw->dispatcher.mutex, 5, 0);
        }
    }

    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

void celix_framework_waitForAsyncRegistrations(framework_t *fw, long bndId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool registrationsInProgress = true;
    while (registrationsInProgress) {
        registrationsInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if ((e->type == CELIX_REGISTER_SERVICE_EVENT || e->type == CELIX_UNREGISTER_SERVICE_EVENT) && e->bndEntry->bndId == bndId) {
                registrationsInProgress = true;
                break;
            }
        }
        for (int i = 0; i < !registrationsInProgress && celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if ((e->type == CELIX_REGISTER_SERVICE_EVENT || e->type == CELIX_UNREGISTER_SERVICE_EVENT) && e->bndEntry->bndId == bndId) {
                registrationsInProgress = true;
                break;
            }
        }
        if (registrationsInProgress) {
            celixThreadCondition_timedwaitRelative(&fw->dispatcher.cond, &fw->dispatcher.mutex, 5, 0);
        }
    }

    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

bool celix_framework_isCurrentThreadTheEventLoop(framework_t* fw) {
    return celixThread_equals(celixThread_self(), fw->dispatcher.thread);
}

const char* celix_framework_getUUID(const celix_framework_t *fw) {
    if (fw != NULL) {
        return celix_properties_get(fw->configurationMap, CELIX_FRAMEWORK_UUID, NULL);
    }
    return NULL;
}


celix_bundle_context_t* celix_framework_getFrameworkContext(const celix_framework_t *fw) {
    celix_bundle_context_t* ctx = NULL;
    if (fw != NULL) {
        if (fw->bundle != NULL) {
            ctx = fw->bundle->context;
        }
    }
    return ctx;
}

celix_bundle_t* celix_framework_getFrameworkBundle(const celix_framework_t *fw) {
    celix_bundle_t* bnd = NULL;
    if (fw != NULL) {
        bnd = fw->bundle;
    }
    return bnd;
}

bundle_pt framework_getBundleById(framework_pt framework, long id) {
    celix_bundle_t *bnd = NULL;
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, id);
    if (entry != NULL) {
        bnd = entry->bnd;
        celix_bundleEntry_decreaseUseCount(entry); //NOTE returning bundle without increased use count -> FIXME make all getBundle api private (use bundle id instead)
    }
    return bnd;
}

bool celix_framework_isBundleInstalled(celix_framework_t *fw, long bndId) {
    bool isInstalled = false;
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        isInstalled = true;
        celix_bundleEntry_decreaseUseCount(entry);
    }
    return isInstalled;
}

bool celix_framework_isBundleActive(celix_framework_t *fw, long bndId) {
    bool isActive = false;
    celix_bundle_entry_t*entry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        isActive = celix_bundle_getState(entry->bnd) == CELIX_BUNDLE_STATE_ACTIVE;
        celix_bundleEntry_decreaseUseCount(entry);
    }
    return isActive;
}

static void celix_framework_waitForBundleEvents(celix_framework_t *fw, long bndId) {
    if (bndId >= 0 && !celix_framework_isCurrentThreadTheEventLoop(fw)) {
        celix_framework_waitUntilNoEventsForBnd(fw, bndId);
    }
}

static long celix_framework_installAndStartBundleInternal(celix_framework_t *fw, const char *bundleLoc, bool autoStart, bool forcedAsync) {
    long bundleId = -1;
    celix_status_t status = celix_framework_installBundleInternal(fw, bundleLoc, &bundleId) ;
    if (status == CELIX_SUCCESS) {
        if (autoStart) {
            celix_bundle_entry_t* bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw,
                                                                                                                     bundleId);
            if (bndEntry != NULL) {
                status = celix_framework_startBundleOnANonCelixEventThread(fw, bndEntry, forcedAsync);
                celix_bundleEntry_decreaseUseCount(bndEntry);
            } else {
                status = CELIX_ILLEGAL_STATE;
            }
        }
    }
    if (!forcedAsync) {
        celix_framework_waitForBundleEvents(fw, bundleId);
    }
    const char* action = autoStart ? "install and start" : "install";
    framework_logIfError(fw->logger, status, NULL, "Failed to %s bundle '%s'", action, bundleLoc);

    return bundleId;
}

long celix_framework_installBundle(celix_framework_t *fw, const char *bundleLoc, bool autoStart) {
    return celix_framework_installAndStartBundleInternal(fw, bundleLoc, autoStart, false);
}

long celix_framework_installBundleAsync(celix_framework_t *fw, const char *bundleLoc, bool autoStart) {
    return celix_framework_installAndStartBundleInternal(fw, bundleLoc, autoStart, true);
}

static bool celix_framework_uninstallBundleInternal(celix_framework_t *fw, long bndId, bool forcedAsync, bool permanent) {
    bool uninstalled = false;
    celix_bundle_entry_t*bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (bndEntry != NULL) {
        celix_status_t status = celix_framework_uninstallBundleOnANonCelixEventThread(fw, bndEntry, forcedAsync, permanent);
        if (!forcedAsync) {
            celix_framework_waitForBundleEvents(fw, bndId);
        }
        //note not decreasing bndEntry, because this entry should now be deleted (uninstalled)
        uninstalled = status == CELIX_SUCCESS;
    }
    return uninstalled;
}

bool celix_framework_uninstallBundle(celix_framework_t *fw, long bndId) {
    return celix_framework_uninstallBundleInternal(fw, bndId, false, true);
}

void celix_framework_uninstallBundleAsync(celix_framework_t *fw, long bndId) {
    celix_framework_uninstallBundleInternal(fw, bndId, true, true);
}

bool celix_framework_unloadBundle(celix_framework_t *fw, long bndId) {
    return celix_framework_uninstallBundleInternal(fw, bndId, false, false);
}

void celix_framework_unloadBundleAsync(celix_framework_t *fw, long bndId) {
    celix_framework_uninstallBundleInternal(fw, bndId, true, false);
}

static celix_status_t celix_framework_uninstallBundleEntryImpl(celix_framework_t* framework, celix_bundle_entry_t* bndEntry,
                                                               bool permanent) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(framework));

    celixThreadRwlock_writeLock(&bndEntry->fsmMutex);
    celix_bundle_state_e bndState = celix_bundle_getState(bndEntry->bnd);
    if (bndState == CELIX_BUNDLE_STATE_ACTIVE) {
        celix_framework_stopBundleEntryInternal(framework, bndEntry);
    }

    if (!fw_bundleEntry_removeBundleEntry(framework, bndEntry)) {
        celixThreadRwlock_unlock(&bndEntry->fsmMutex);
        celix_bundleEntry_decreaseUseCount(bndEntry);
        return CELIX_ILLEGAL_STATE;
    }

    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_t *bnd = bndEntry->bnd;
    bundle_archive_t *archive = NULL;
    celix_bundle_revision_t*revision = NULL;
    celix_module_t* module = NULL;
    status = CELIX_DO_IF(status, bundle_getArchive(bnd, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundle_getCurrentModule(bnd, &module));

    if (module) {
        celix_module_closeLibraries(module);
    }

    CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bndEntry));

    status = CELIX_DO_IF(status, bundle_setState(bnd, CELIX_BUNDLE_STATE_UNINSTALLED));

    CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED, bndEntry));

    celixThreadRwlock_unlock(&bndEntry->fsmMutex);

    //NOTE wait outside installedBundles.mutex
    celix_bundleEntry_decreaseUseCount(bndEntry);
    fw_bundleEntry_destroy(bndEntry, true); //wait till use count is 0 -> e.g. not used

    if (status == CELIX_SUCCESS) {
        celix_framework_waitForEmptyEventQueue(framework); //to ensure that the uninstalled event is triggered and handled
        (void)bundle_destroy(bnd);
        if(permanent) {
            celix_bundleArchive_invalidate(archive);
        }
        (void)celix_bundleCache_destroyArchive(framework->cache, archive);
    }
    framework_logIfError(framework->logger, status, "", "Cannot uninstall bundle");
    return status;

}

celix_status_t celix_framework_uninstallBundleEntry(celix_framework_t* framework, celix_bundle_entry_t* bndEntry, bool permanent) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&framework->installLock);
    status = celix_framework_uninstallBundleEntryImpl(framework, bndEntry, permanent);
    celixThreadMutex_unlock(&framework->installLock);
    return status;
}

static bool celix_framework_stopBundleInternal(celix_framework_t* fw, long bndId, bool forcedAsync) {
    bool stopped = false;
    celix_bundle_entry_t*bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (bndEntry != NULL) {
        celix_bundle_state_e state = celix_bundle_getState(bndEntry->bnd);
        if (state == CELIX_BUNDLE_STATE_ACTIVE) {
            celix_status_t rc = celix_framework_stopBundleOnANonCelixEventThread(fw, bndEntry, forcedAsync);
            stopped = rc == CELIX_SUCCESS;
        } else if (state == CELIX_BUNDLE_STATE_RESOLVED) {
            //already stopped, silently ignore.
        } else {
            fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING, "Cannot stop bundle, bundle state is %s", celix_bundleState_getName(state));
        }
        celix_bundleEntry_decreaseUseCount(bndEntry);
        if (!forcedAsync) {
            celix_framework_waitForBundleEvents(fw, bndId);
        }
    }
    return stopped;
}

bool celix_framework_stopBundle(celix_framework_t *fw, long bndId) {
    return celix_framework_stopBundleInternal(fw, bndId, false);
}

void celix_framework_stopBundleAsync(celix_framework_t* fw, long bndId) {
    celix_framework_stopBundleInternal(fw, bndId, true);
}

static celix_status_t celix_framework_stopBundleEntryInternal(celix_framework_t* framework,
                                                              celix_bundle_entry_t* bndEntry) {
    celix_bundle_activator_t *activator = NULL;
    bundle_context_t *context = NULL;
    bool wasActive = false;
    char *error = NULL;

    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_state_e state = celix_bundle_getState(bndEntry->bnd);
    switch (state) {
    case CELIX_BUNDLE_STATE_UNKNOWN:
        status = CELIX_ILLEGAL_STATE;
        error = "state is unknown";
        break;
    case CELIX_BUNDLE_STATE_UNINSTALLED:
        status = CELIX_ILLEGAL_STATE;
        error = "bundle is uninstalled";
        break;
    case CELIX_BUNDLE_STATE_STARTING:
        status = CELIX_BUNDLE_EXCEPTION;
        error = "bundle is starting";
        break;
    case CELIX_BUNDLE_STATE_STOPPING:
        status = CELIX_BUNDLE_EXCEPTION;
        error = "bundle is stopping";
        break;
    case CELIX_BUNDLE_STATE_INSTALLED:
    case CELIX_BUNDLE_STATE_RESOLVED:
        break;
    case CELIX_BUNDLE_STATE_ACTIVE:
        wasActive = true;
        break;
    }


    if (status == CELIX_SUCCESS && wasActive) {
        CELIX_DO_IF(status, bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_STOPPING));
        CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING, bndEntry));
        activator = bundle_getActivator(bndEntry->bnd);

        if (status == CELIX_SUCCESS) {
            context = celix_bundle_getContext(bndEntry->bnd);
            if (activator->stop != NULL) {
                status = CELIX_DO_IF(status, activator->stop(activator->userData, context));
                if (status == CELIX_SUCCESS) {
                    celix_dependency_manager_t *mng = celix_bundleContext_getDependencyManager(context);
                    celix_dependencyManager_removeAllComponents(mng);
                }
            }
        }
        if (status == CELIX_SUCCESS) {
            if (activator->destroy != NULL) {
                status = CELIX_DO_IF(status, activator->destroy(activator->userData, context));
            }
        }

        if (bndEntry->bndId >= CELIX_FRAMEWORK_BUNDLE_ID) {
            //framework and "normal" bundle
            celix_framework_waitUntilNoEventsForBnd(framework, bndEntry->bndId);
            celix_bundleContext_cleanup(bndEntry->bnd->context);
        }
        if (bndEntry->bndId > CELIX_FRAMEWORK_BUNDLE_ID) {
            //"normal" bundle
            if (status == CELIX_SUCCESS) {
                celix_module_t* module = NULL;
                const char *symbolicName = NULL;
                long id = 0;
                bundle_getCurrentModule(bndEntry->bnd, &module);
                module_getSymbolicName(module, &symbolicName);
                bundle_getBundleId(bndEntry->bnd, &id);
            }

            if (context != NULL) {
                status = CELIX_DO_IF(status, bundleContext_destroy(context));
                CELIX_DO_IF(status, celix_bundle_setContext(bndEntry->bnd, NULL));
            }

            status = CELIX_DO_IF(status, bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_RESOLVED));
            CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, bndEntry));
        } else if (bndEntry->bndId == CELIX_FRAMEWORK_BUNDLE_ID) {
            //framework bundle
            status = CELIX_DO_IF(status, bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_RESOLVED));
            CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, bndEntry));
        }

        if (activator != NULL) {
            bundle_setActivator(bndEntry->bnd, NULL);
            free(activator);
        }
    }

    if (status != CELIX_SUCCESS) {
        celix_module_t* module = NULL;
        const char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(bndEntry->bnd, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(bndEntry->bnd, &id);
        if (error != NULL) {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot stop bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot stop bundle: %s [%ld]", symbolicName, id);
        }
    } else {
        fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, bndEntry);
    }
    return status;
}

celix_status_t celix_framework_stopBundleEntry(celix_framework_t* framework, celix_bundle_entry_t* bndEntry) {
    celix_status_t status = CELIX_SUCCESS;
    assert(!celix_framework_isCurrentThreadTheEventLoop(framework));
    celixThreadRwlock_writeLock(&bndEntry->fsmMutex);
    status = celix_framework_stopBundleEntryInternal(framework, bndEntry);
    celixThreadRwlock_unlock(&bndEntry->fsmMutex);
    return status;
}

bool celix_framework_startBundleInternal(celix_framework_t *fw, long bndId, bool forcedAsync) {
    bool started = false;
    celix_bundle_entry_t*bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (bndEntry != NULL) {
        celix_bundle_state_e state = celix_bundle_getState(bndEntry->bnd);
        if (state == CELIX_BUNDLE_STATE_INSTALLED || state == CELIX_BUNDLE_STATE_RESOLVED) {
            celix_status_t rc = celix_framework_startBundleOnANonCelixEventThread(fw, bndEntry, forcedAsync);
            started = rc == CELIX_SUCCESS;
        }
        celix_bundleEntry_decreaseUseCount(bndEntry);
        if (!forcedAsync) {
            celix_framework_waitForBundleEvents(fw, bndId);
        }
    }
    return started;
}

bool celix_framework_startBundle(celix_framework_t *fw, long bndId) {
    return celix_framework_startBundleInternal(fw, bndId, false);
}

void celix_framework_startBundleAsync(celix_framework_t *fw, long bndId) {
    celix_framework_startBundleInternal(fw, bndId, true);
}

static void celix_framework_printCelixErrForBundleEntry(celix_framework_t* framework, celix_bundle_entry_t* bndEntry) {
    if (celix_err_getErrorCount() > 0) {
        celix_framework_log(framework->logger, CELIX_LOG_LEVEL_WARNING, NULL, NULL, 0,
               "Found unprocessed celix err messages for bundle %s [bndId=%li]. Unprocessed celix err messages:",
               celix_bundle_getSymbolicName(bndEntry->bnd),
               bndEntry->bndId);
        int count = 1;
        const char* msg = NULL;
        while ((msg = celix_err_popLastError())) {
            celix_framework_log(framework->logger, CELIX_LOG_LEVEL_ERROR, NULL, NULL, 0,
                                "Message nr %i: %s", count++, msg);
        }
    }
}

celix_status_t celix_framework_startBundleEntry(celix_framework_t* framework, celix_bundle_entry_t* bndEntry) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(framework));
    celix_status_t status = CELIX_SUCCESS;
    const char* error = NULL;
    const char* name = "";
    celix_module_t* module = NULL;
    celix_bundle_context_t* context = NULL;
    celix_bundle_activator_t* activator = NULL;

    celixThreadRwlock_writeLock(&bndEntry->fsmMutex);
    celix_bundle_state_e state = celix_bundle_getState(bndEntry->bnd);

    switch (state) {
        case CELIX_BUNDLE_STATE_UNKNOWN:
            error = "state is unknown";
            status = CELIX_ILLEGAL_STATE;
            break;
        case CELIX_BUNDLE_STATE_UNINSTALLED:
            error = "bundle is uninstalled";
            status = CELIX_ILLEGAL_STATE;
            break;
        case CELIX_BUNDLE_STATE_STARTING:
            error = "bundle is starting";
            status = CELIX_BUNDLE_EXCEPTION;
            break;
        case CELIX_BUNDLE_STATE_STOPPING:
            error = "bundle is stopping";
            status = CELIX_BUNDLE_EXCEPTION;
            break;
        case CELIX_BUNDLE_STATE_ACTIVE:
            break;
        case CELIX_BUNDLE_STATE_INSTALLED:
            bundle_getCurrentModule(bndEntry->bnd, &module);
            module_getSymbolicName(module, &name);
            if (!module_isResolved(module)) {
                status = framework_markBundleResolved(framework, module);
                if (status == CELIX_SUCCESS) {
                    module_setResolved(module);
                } else {
                    break;
                }
            }
            /* no break */
        case CELIX_BUNDLE_STATE_RESOLVED:
            module = NULL;
            name = NULL;
            bundle_getCurrentModule(bndEntry->bnd, &module);
            module_getSymbolicName(module, &name);
            status = CELIX_DO_IF(status, bundleContext_create(framework, framework->logger, bndEntry->bnd, &context));
            CELIX_DO_IF(status, celix_bundle_setContext(bndEntry->bnd, context));

            if (status == CELIX_SUCCESS) {
                activator = calloc(1,(sizeof(*activator)));
                if (activator == NULL) {
                    status = CELIX_ENOMEM;
                } else {
                    void* userData = NULL;
                    void* bundleHandle = bundle_getHandle(bndEntry->bnd);

                    activator->create = celix_libloader_findBundleActivatorCreate(bundleHandle);
                    activator->start = celix_libloader_findBundleActivatorStart(bundleHandle);
                    activator->stop = celix_libloader_findBundleActivatorStop(bundleHandle);
                    activator->destroy = celix_libloader_findBundleActivatorDestroy(bundleHandle);

                    status = CELIX_DO_IF(status, bundle_setActivator(bndEntry->bnd, activator));

                    status = CELIX_DO_IF(status, bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_STARTING));
                    CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING, bndEntry));

                    if (status == CELIX_SUCCESS) {
                        context = celix_bundle_getContext(bndEntry->bnd);
                        if (activator->create != NULL) {
                            status = CELIX_DO_IF(status, activator->create(context, &userData));
                            if (status == CELIX_SUCCESS) {
                                activator->userData = userData;
                            }
                        }
                    }
                    if (status == CELIX_SUCCESS) {
                        if (activator->start != NULL) {
                            status = CELIX_DO_IF(status, activator->start(userData, context));
                        }
                        celix_framework_printCelixErrForBundleEntry(framework, bndEntry);
                    }

                    status = CELIX_DO_IF(status, bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_ACTIVE));
                    CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, bndEntry));

                    if (status != CELIX_SUCCESS) {
                        //state is still STARTING, back to resolved
                        bool createCalled = activator != NULL && activator->userData != NULL;
                        if (createCalled && activator->destroy) {
                            activator->destroy(activator->userData, context);
                        }
                        celix_bundle_setContext(bndEntry->bnd, NULL);
                        bundle_setActivator(bndEntry->bnd, NULL);
                        bundleContext_destroy(context);
                        free(activator);
                        (void)bundle_setState(bndEntry->bnd, CELIX_BUNDLE_STATE_RESOLVED);
                    }
                }
            }

            break;
    }

    if (status != CELIX_SUCCESS) {
        const char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(bndEntry->bnd, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(bndEntry->bnd, &id);
        if (error != NULL) {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
        }
    }
    celixThreadRwlock_unlock(&bndEntry->fsmMutex);
    return status;
}

static bool celix_framework_updateBundleInternal(celix_framework_t *fw, long bndId, const char* updatedBundleUrl, bool forcedAsync) {
    if (bndId == CELIX_FRAMEWORK_BUNDLE_ID) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_INFO, "Cannot update framework bundle. Ignore update cmd.");
        return true;
    }
    bool updated = false;
    celix_bundle_entry_t*bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (bndEntry != NULL) {
        celix_status_t rc = celix_framework_updateBundleOnANonCelixEventThread(fw, bndEntry, updatedBundleUrl, forcedAsync);
        //note not decreasing bndEntry, because this entry should now be deleted (uninstalled)
        updated = rc == CELIX_SUCCESS;
        if (!forcedAsync) {
            celix_framework_waitForBundleEvents(fw, bndId);
        }
    }
    return updated;
}

celix_status_t celix_framework_updateBundleEntry(celix_framework_t* framework,
                                                 celix_bundle_entry_t* bndEntry,
                                                 const char* updatedBundleUrl) {
    celix_status_t status = CELIX_SUCCESS;
    long bundleId = bndEntry->bndId;
    fw_log(framework->logger, CELIX_LOG_LEVEL_DEBUG, "Updating bundle %s", celix_bundle_getSymbolicName(bndEntry->bnd));
    celix_bundle_state_e bndState = celix_bundle_getState(bndEntry->bnd);
    celix_autofree char* location = celix_bundle_getLocation(bndEntry->bnd);

    // lock to reuse the bundle id
    celix_auto(celix_mutex_lock_guard_t) lck = celixMutexLockGuard_init(&framework->installLock);
    if (updatedBundleUrl == NULL) {
        updatedBundleUrl = location;
    } else if (strcmp(location, updatedBundleUrl) != 0) {
        long existingBndId = -1L;
        // celix_bundleCache_findBundleIdForLocation can never fail since there is at lease bndEntry is installed
        (void)celix_bundleCache_findBundleIdForLocation(framework->cache, updatedBundleUrl, &existingBndId);
        if (existingBndId != -1 && existingBndId != bundleId) {
            fw_log(framework->logger,
                   CELIX_LOG_LEVEL_ERROR,
                   "Specified bundle location %s exists in cache with a different id. Update failed.",
                   updatedBundleUrl);
            celix_bundleEntry_decreaseUseCount(bndEntry);
            return CELIX_ILLEGAL_STATE;
        }
        celix_bundleArchive_invalidateCache(celix_bundle_getArchive(bndEntry->bnd));
    }
    status = celix_framework_uninstallBundleEntryImpl(framework, bndEntry, false);
    if (status != CELIX_SUCCESS) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Failed to uninstall bundle id %li", bundleId);
        return status;
    }

    // bndEntry is now invalid
    status = celix_framework_installBundleInternalImpl(framework, updatedBundleUrl, &bundleId);
    if (status != CELIX_SUCCESS) {
        fw_log(framework->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Failed to install updated bundle %s",
               updatedBundleUrl);
        return status;
    }

    if (bndState != CELIX_BUNDLE_STATE_STARTING && bndState != CELIX_BUNDLE_STATE_ACTIVE) {
        // no need to restart the updated bundle
        fw_log(framework->logger, CELIX_LOG_LEVEL_DEBUG, "Bundle %li is not active, no need to restart", bundleId);
        return CELIX_SUCCESS;
    }

    celix_bundle_entry_t* entry =
        celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bundleId);
    celix_auto(celix_bundle_entry_use_guard_t) entryGuard = celix_bundleEntryUseGuard_init(entry);
    celixMutexLockGuard_deinit(&lck);
    status = celix_framework_startBundleEntry(framework, entry);
    if (status != CELIX_SUCCESS) {
        fw_log(framework->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Failed to start updated bundle %s",
               celix_bundle_getSymbolicName(entry->bnd));
        return CELIX_BUNDLE_EXCEPTION;
    }

    return CELIX_SUCCESS;
}

bool celix_framework_updateBundle(celix_framework_t *fw, long bndId, const char* updatedBundleUrl) {
    return celix_framework_updateBundleInternal(fw, bndId, updatedBundleUrl, false);
}

void celix_framework_updateBundleAsync(celix_framework_t *fw, long bndId, const char* updatedBundleUrl) {
    celix_framework_updateBundleInternal(fw, bndId, updatedBundleUrl, true);
}

static celix_array_list_t* celix_framework_listBundlesInternal(celix_framework_t* framework, bool activeOnly) {
    celix_array_list_t* result = celix_arrayList_create();
    celix_auto(celix_mutex_lock_guard_t) lock = celixMutexLockGuard_init(&framework->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(framework->installedBundles.entries); ++i) {
        celix_bundle_entry_t* entry = celix_arrayList_get(framework->installedBundles.entries, i);
        if (entry->bndId == CELIX_FRAMEWORK_BUNDLE_ID) {
            continue;
        }
        if (!activeOnly) {
            celix_arrayList_addLong(result, entry->bndId);
        } else if (celix_bundle_getState(entry->bnd) == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            celix_arrayList_addLong(result, entry->bndId);
        }
    }
    return result;
}

celix_array_list_t* celix_framework_listBundles(celix_framework_t* framework) {
    return celix_framework_listBundlesInternal(framework, true);
}

celix_array_list_t* celix_framework_listInstalledBundles(celix_framework_t* framework) {
    return celix_framework_listBundlesInternal(framework, false);
}

celix_status_t celix_framework_waitForEmptyEventQueueFor(celix_framework_t *fw, double periodInSeconds) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));
    celix_status_t status = CELIX_SUCCESS;

    struct timespec absTimeout = {0, 0};
    absTimeout = (periodInSeconds == 0) ? absTimeout : celixThreadCondition_getDelayedTime(periodInSeconds);
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    while (celix_framework_eventQueueSize(fw) > 0) {
        if (periodInSeconds == 0) {
            celixThreadCondition_wait(&fw->dispatcher.cond, &fw->dispatcher.mutex);
        } else {
            status = celixThreadCondition_waitUntil(&fw->dispatcher.cond, &fw->dispatcher.mutex, &absTimeout);
            if (status == ETIMEDOUT) {
                break;
            }
        }

    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return status;
}

void celix_framework_waitForEmptyEventQueue(celix_framework_t *fw) {
    celix_framework_waitForEmptyEventQueueFor(fw, 0.0);
}

void celix_framework_waitUntilNoEventsForBnd(celix_framework_t* fw, long bndId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool eventInProgress = true;
    while (eventInProgress) {
        eventInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if (e->bndEntry != NULL && (bndId < 0 || e->bndEntry->bndId == bndId)) {
                eventInProgress = true;
                break;
            }
        }
        for (int i = 0; !eventInProgress && i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if (e->bndEntry != NULL && (bndId < 0 || e->bndEntry->bndId == bndId)) {
                eventInProgress = true;
                break;
            }
        }
        if (eventInProgress) {
            celixThreadCondition_timedwaitRelative(&fw->dispatcher.cond, &fw->dispatcher.mutex, 5, 0);
        }
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

void celix_framework_waitUntilNoPendingRegistration(celix_framework_t* fw)
{
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    while (__atomic_load_n(&fw->dispatcher.stats.nbRegister, __ATOMIC_RELAXED) > 0) {
        celixThreadCondition_wait(&fw->dispatcher.cond, &fw->dispatcher.mutex);
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

long celix_framework_scheduleEvent(celix_framework_t* fw,
                                    long bndId,
                                    const char* eventName,
                                    double initialDelayInSeconds,
                                    double intervalInSeconds,
                                    void* callbackData,
                                    void (*callback)(void*),
                                    void* removeCallbackData,
                                    void (*removeCallback)(void*)) {
    if (callback == NULL) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Cannot add scheduled event for bundle id %li. Invalid NULL event callback.",
               bndId);
        return -1;
    }
    if (initialDelayInSeconds < 0 || intervalInSeconds < 0) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_ERROR,
               "Cannot add scheduled event for bundle id %li. Invalid intervals: (%f,%f).",
               bndId, initialDelayInSeconds, intervalInSeconds);
        return -1;
    }

    celix_bundle_entry_t* bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (bndEntry == NULL) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Cannot add scheduled event for non existing bundle id %li.", bndId);
        return -1;
    }
    celix_scheduled_event_t* event = celix_scheduledEvent_create(fw,
                                                                 bndEntry->bndId,
                                                                 celix_framework_nextScheduledEventId(fw),
                                                                 eventName,
                                                                 initialDelayInSeconds,
                                                                 intervalInSeconds,
                                                                 callbackData,
                                                                 callback,
                                                                 removeCallbackData,
                                                                 removeCallback);

    if (event == NULL) {
        celix_bundleEntry_decreaseUseCount(bndEntry);
        return -1L; //error logged by celix_scheduledEvent_create
    }

    long id = celix_scheduledEvent_getId(event);
    fw_log(fw->logger,
           CELIX_LOG_LEVEL_DEBUG,
           "Added scheduled event '%s' (id=%li) for bundle '%s' (id=%li).",
           celix_scheduledEvent_getName(event),
           id,
           celix_bundle_getSymbolicName(bndEntry->bnd),
           bndId);
    celix_bundleEntry_decreaseUseCount(bndEntry);

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    if (fw->dispatcher.active) {
        celix_longHashMap_put(fw->dispatcher.scheduledEvents, id, event);
        celixThreadCondition_broadcast(&fw->dispatcher.cond); //notify dispatcher thread for newly added scheduled event
    } else {
        celix_scheduledEvent_release(event);
        id = -1L;
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);

    return id;
}

celix_status_t celix_framework_wakeupScheduledEvent(celix_framework_t* fw, long scheduledEventId) {
    if (scheduledEventId < 0) {
        return CELIX_SUCCESS; // silently ignore
    }
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    celix_scheduled_event_t* event = celix_longHashMap_get(fw->dispatcher.scheduledEvents, scheduledEventId);
    if (event != NULL) {
        celix_scheduledEvent_markForWakeup(event);
        celixThreadCondition_broadcast(&fw->dispatcher.cond); //notify dispatcher thread for configured wakeup
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);

    if (event == NULL) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_WARNING,
               "celix_framework_wakeupScheduledEvent called with unknown scheduled event id %li.",
               scheduledEventId);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    return CELIX_SUCCESS;
}

celix_status_t
celix_framework_waitForScheduledEvent(celix_framework_t* fw, long scheduledEventId, double waitTimeInSeconds) {
    if (scheduledEventId < 0) {
        return CELIX_SUCCESS; // silently ignore
    }

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    celix_autoptr(celix_scheduled_event_t) event = celix_scheduledEvent_retain(
        celix_longHashMap_get(fw->dispatcher.scheduledEvents, scheduledEventId));
    celixThreadMutex_unlock(&fw->dispatcher.mutex);

    if (event == NULL) {
        fw_log(fw->logger,
               CELIX_LOG_LEVEL_WARNING,
               "Cannot wait for scheduled event. Unknown scheduled event id %li.",
               scheduledEventId);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t status = celix_scheduledEvent_wait(event, waitTimeInSeconds);
    return status;
}

bool celix_framework_removeScheduledEvent(celix_framework_t* fw,
                                          bool async,
                                          bool errorIfNotFound,
                                          long scheduledEventId) {
    if (scheduledEventId < 0) {
        return false; // silently ignore
    }

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    celix_autoptr(celix_scheduled_event_t) event = celix_scheduledEvent_retain(
        celix_longHashMap_get(fw->dispatcher.scheduledEvents, scheduledEventId));
    if (event) {
        celix_scheduledEvent_markForRemoval(event);
        celixThreadCondition_broadcast(&fw->dispatcher.cond); //notify dispatcher thread for removed scheduled event
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);

    if (!event) {
        celix_log_level_e level = errorIfNotFound ? CELIX_LOG_LEVEL_ERROR : CELIX_LOG_LEVEL_TRACE;
        fw_log(fw->logger, level, "Cannot remove scheduled event with id %li. Not found.", scheduledEventId);
        return false;
    }

    if (!async) {
        celix_scheduledEvent_waitForRemoved(event);
    }
    return true;
}

void celix_framework_setLogCallback(celix_framework_t* fw, void* logHandle, void (*logFunction)(void* handle, celix_log_level_e level, const char* file, const char *function, int line, const char *format, va_list formatArgs)) {
    celix_frameworkLogger_setLogCallback(fw->logger, logHandle, logFunction);
}

long celix_framework_fireGenericEvent(framework_t* fw, long eventId, long bndId, const char *eventName, void* processData, void (*processCallback)(void *data), void* doneData, void (*doneCallback)(void* doneData)) {
    celix_bundle_entry_t* bndEntry = NULL;
    if (bndId >=0) {
        bndEntry = celix_framework_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
        if (bndEntry == NULL) {
            fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Cannot find bundle for id %li.", bndId);
            return -1L;
        }
    }

    if (eventId < 0) {
        eventId = celix_framework_nextEventId(fw);
    }

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_GENERIC_EVENT;
    event.bndEntry = bndEntry;
    event.genericEventId = eventId;
    event.genericEventName = eventName;
    event.genericProcessData = processData;
    event.genericProcess = processCallback;
    event.doneData = doneData;
    event.doneCallback = doneCallback;
    __atomic_add_fetch(&fw->dispatcher.stats.nbEvent, 1, __ATOMIC_RELAXED);
    celix_framework_addToEventQueue(fw, &event);

    return eventId;
}

long celix_framework_nextEventId(framework_t *fw) {
    return __atomic_fetch_add(&fw->dispatcher.nextEventId, 1, __ATOMIC_RELAXED);
}

long celix_framework_nextScheduledEventId(framework_t *fw) {
    return __atomic_fetch_add(&fw->dispatcher.nextScheduledEventId, 1, __ATOMIC_RELAXED);
}

/**
 * @brief Checks if a generic event with the provided eventId is in progress.
 */
static bool celix_framework_isGenericEventInProgress(celix_framework_t* fw, long eventId) {
    // precondition fw->dispatcher.mutex locked)
    for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
        int index = (fw->dispatcher.eventQueueFirstEntry + i) % fw->dispatcher.eventQueueCap;
        celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
        if (e->type == CELIX_GENERIC_EVENT && e->genericEventId == eventId) {
            return true;;
        }
    }
    for (int i = 0; i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
        celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
        if (e->type == CELIX_GENERIC_EVENT && e->genericEventId == eventId) {
            return true;
        }
    }
    return false;
}

void celix_framework_waitForGenericEvent(celix_framework_t* fw, long eventId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));
    struct timespec logAbsTime = celixThreadCondition_getDelayedTime(5);
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    while (celix_framework_isGenericEventInProgress(fw, eventId)) {
        celix_status_t waitStatus =
            celixThreadCondition_waitUntil(&fw->dispatcher.cond, &fw->dispatcher.mutex, &logAbsTime);
        if (waitStatus == ETIMEDOUT) {
            fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING, "Generic event with id %li not finished.", eventId);
            logAbsTime = celixThreadCondition_getDelayedTime(5);
        }
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

void celix_framework_waitForStop(celix_framework_t *framework) {
    celixThreadMutex_lock(&framework->shutdown.mutex);
    while (!framework->shutdown.done) {
        celixThreadCondition_wait(&framework->shutdown.cond, &framework->shutdown.mutex);
    }
    if (!framework->shutdown.joined) {
        celixThread_join(framework->shutdown.thread, NULL);
        framework->shutdown.joined = true;
    }

    celixThreadMutex_unlock(&framework->shutdown.mutex);
}

