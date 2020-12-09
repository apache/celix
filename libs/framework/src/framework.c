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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "celixbool.h"
#include <uuid/uuid.h>
#include <assert.h>
#include <celix_log_utils.h>

#include "celix_dependency_manager.h"
#include "framework_private.h"
#include "celix_constants.h"
#include "resolver.h"
#include "utils.h"
#include "linked_list_iterator.h"
#include "service_reference_private.h"
#include "service_registration_private.h"
#include "bundle_private.h"
#include "celix_bundle_context.h"
#include "bundle_context_private.h"
#include "service_tracker.h"
#include "celix_library_loader.h"
#include "celix_log_constants.h"

typedef celix_status_t (*create_function_fp)(bundle_context_t *context, void **userData);
typedef celix_status_t (*start_function_fp)(void *userData, bundle_context_t *context);
typedef celix_status_t (*stop_function_fp)(void *userData, bundle_context_t *context);
typedef celix_status_t (*destroy_function_fp)(void *userData, bundle_context_t *context);

struct celix_bundle_activator {
    void * userData;

    create_function_fp create;
    start_function_fp start;
    stop_function_fp stop;
    destroy_function_fp destroy;
};

static inline celix_framework_bundle_entry_t* fw_bundleEntry_create(celix_bundle_t *bnd) {
    celix_framework_bundle_entry_t *entry = calloc(1, sizeof(*entry));
    entry->bnd = bnd;

    entry->bndId = celix_bundle_getId(bnd);
    entry->useCount = 0;
    celixThreadMutex_create(&entry->useMutex, NULL);
    celixThreadCondition_init(&entry->useCond, NULL);
    return entry;
}


static inline void fw_bundleEntry_waitTillUseCountIs(celix_framework_bundle_entry_t *entry, size_t desiredUseCount) {
    celixThreadMutex_lock(&entry->useMutex);
    time_t start = time(NULL);
    bool warningPrinted = false;
    while (entry->useCount != desiredUseCount) {
        celixThreadCondition_timedwaitRelative(&entry->useCond, &entry->useMutex, 5, 0);
        if (entry->useCount != desiredUseCount) {
            time_t now = time(NULL);
            if (!warningPrinted && (now-start) > 5) {
                fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_WARNING, "Bundle %s (%li) still in use. Use count is %u, desired is %li", celix_bundle_getSymbolicName(entry->bnd), entry->bndId, entry->useCount, desiredUseCount);
                warningPrinted = true;
            }
        }
    }
    celixThreadMutex_unlock(&entry->useMutex);
}

static inline void fw_bundleEntry_destroy(celix_framework_bundle_entry_t *entry, bool wait) {
    celixThreadMutex_lock(&entry->useMutex);
    while (wait && entry->useCount != 0) {
        celixThreadCondition_wait(&entry->useCond, &entry->useMutex);
    }
    celixThreadMutex_unlock(&entry->useMutex);

    //destroy
    celixThreadMutex_destroy(&entry->useMutex);
    celixThreadCondition_destroy(&entry->useCond);
    free(entry);
}

static inline void fw_bundleEntry_increaseUseCount(celix_framework_bundle_entry_t *entry) {
    assert(entry != NULL);
    celixThreadMutex_lock(&entry->useMutex);
    ++entry->useCount;
    celixThreadMutex_unlock(&entry->useMutex);
}

static inline void fw_bundleEntry_decreaseUseCount(celix_framework_bundle_entry_t *entry) {
    celixThreadMutex_lock(&entry->useMutex);
    assert(entry->useCount > 0);
    --entry->useCount;
    celixThreadCondition_broadcast(&entry->useCond);
    celixThreadMutex_unlock(&entry->useMutex);
}

static inline celix_framework_bundle_entry_t* fw_bundleEntry_getBundleEntryAndIncreaseUseCount(celix_framework_t *fw, long bndId) {
    celix_framework_bundle_entry_t* found = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId == bndId) {
            fw_bundleEntry_increaseUseCount(entry);
            found = entry;
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return found;
}

static inline celix_framework_bundle_entry_t* fw_bundleEntry_removeBundleEntryAndIncreaseUseCount(celix_framework_t *fw, long bndId) {
    celix_framework_bundle_entry_t* found = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(fw->installedBundles.entries); ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId == bndId) {
            found = entry;
            fw_bundleEntry_increaseUseCount(entry);
            celix_arrayList_removeAt(fw->installedBundles.entries, i);
            break;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);
    return found;
}

celix_status_t framework_markBundleResolved(framework_pt framework, module_pt module);

long framework_getNextBundleId(framework_pt framework);

celix_status_t fw_installBundle2(framework_pt framework, bundle_pt * bundle, long id, const char * location, const char *inputFile, bundle_archive_pt archive);

celix_status_t fw_refreshBundles(framework_pt framework, long bundleIds[], int size);
celix_status_t fw_refreshBundle(framework_pt framework, long bndId);

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map);

void fw_fireBundleEvent(framework_pt framework, bundle_event_type_e, celix_framework_bundle_entry_t* entry);
void fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, celix_status_t errorCode);
static void *fw_eventDispatcher(void *fw);

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle);
celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle);

static celix_status_t framework_loadBundleLibraries(framework_pt framework, bundle_pt bundle);
static celix_status_t framework_loadLibraries(framework_pt framework, const char* libraries, const char* activator, bundle_archive_pt archive, void **activatorHandle);
static celix_status_t framework_loadLibrary(framework_pt framework, const char* library, bundle_archive_pt archive, void **handle);

static celix_status_t frameworkActivator_start(void * userData, bundle_context_t *context);
static celix_status_t frameworkActivator_stop(void * userData, bundle_context_t *context);
static celix_status_t frameworkActivator_destroy(void * userData, bundle_context_t *context);

static void framework_autoStartConfiguredBundles(bundle_context_t *fwCtx);
static void framework_autoInstallConfiguredBundlesForList(bundle_context_t *fwCtx, const char *autoStart, celix_array_list_t *installedBundles);
static void framework_autoStartConfiguredBundlesForList(bundle_context_t *fwCtx, const celix_array_list_t *installedBundles);
static void celix_framework_addToEventQueue(celix_framework_t *fw, const celix_framework_event_t* event);

struct fw_refreshHelper {
    framework_pt framework;
    bundle_pt bundle;
    bundle_state_e oldState;
};

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper);

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


celix_status_t framework_create(framework_pt *out, properties_pt config) {
    celix_framework_t* framework = calloc(1, sizeof(*framework));
//    celix_thread_mutexattr_t attr;
//    celixThreadMutexAttr_create(&attr);
//    celixThreadMutexAttr_settype(&attr, CELIX_THREAD_MUTEX_RECURSIVE);

    celixThreadCondition_init(&framework->shutdown.cond, NULL);
    celixThreadMutex_create(&framework->shutdown.mutex, NULL);
    celixThreadMutex_create(&framework->dispatcher.mutex, NULL);
    celixThreadMutex_create(&framework->frameworkListenersLock, NULL);
    celixThreadMutex_create(&framework->bundleListenerLock, NULL);
    celixThreadMutex_create(&framework->installedBundles.mutex, NULL);
    celixThreadCondition_init(&framework->dispatcher.cond, NULL);
    framework->dispatcher.active = true;
    framework->nextBundleId = 1L; //system bundle is 0
    framework->installRequestMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
    framework->installedBundles.entries = celix_arrayList_create();
    framework->configurationMap = config;
    framework->bundleListeners = celix_arrayList_create();
    framework->frameworkListeners = celix_arrayList_create();
    framework->dispatcher.dynamicEventQueue = celix_arrayList_create();

    //create and store framework uuid
    char uuid[37];
    uuid_t uid;
    uuid_generate(uid);
    uuid_unparse(uid, uuid);
    properties_set(framework->configurationMap, (char*) OSGI_FRAMEWORK_FRAMEWORK_UUID, uuid);

    //setup framework logger
    const char* logStr = getenv(CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME);
    if (logStr == NULL) {
        logStr = celix_properties_get(config, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_DEFAULT_VALUE);
    }
    framework->logger = celix_frameworkLogger_create(celix_logUtils_logLevelFromString(logStr, CELIX_LOG_LEVEL_INFO));

    celix_status_t status =bundle_create(&framework->bundle);
    status = CELIX_DO_IF(status, bundle_getBundleId(framework->bundle, &framework->bundleId));
    status = CELIX_DO_IF(status, bundle_setFramework(framework->bundle, framework));
    status = CELIX_DO_IF(status, bundleCache_create(uuid, framework->configurationMap, &framework->cache));
    status = CELIX_DO_IF(status, serviceRegistry_create(framework, &framework->registry));
    bundle_context_t *context = NULL;
    status = CELIX_DO_IF(status, bundleContext_create(framework, framework->logger, framework->bundle, &context));
    status = CELIX_DO_IF(status, bundle_setContext(framework->bundle, context));

    //create framework bundle entry
    long bndId = -1L;
    bundle_getBundleId(framework->bundle, &bndId);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_create(framework->bundle);
    celixThreadMutex_lock(&framework->installedBundles.mutex);
    celix_arrayList_add(framework->installedBundles.entries, entry);
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not create framework");
        free(framework);
        framework = NULL;
    }

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
    } else {
        fw_log(framework->logger, CELIX_LOG_LEVEL_FATAL, "Cannot destroy framework. framework is not stopped or stopping!");
        return CELIX_ILLEGAL_STATE;
    }

    //Note the shutdown thread can not be joined on the framework_shutdown (which is normally more logical),
    //because a shutdown can be initiated from a bundle.
    //A bundle cannot be stopped when it is waiting for a framework shutdown -> hence a shutdown thread which
    //has not been joined yet.
    if (!framework->shutdown.joined) {
        celixThread_join(framework->shutdown.thread, NULL);
        framework->shutdown.joined = true;
    }


    serviceRegistry_destroy(framework->registry);

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(framework->installedBundles.entries); ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celixThreadMutex_lock(&entry->useMutex);
        size_t count = entry->useCount;
        celixThreadMutex_unlock(&entry->useMutex);
        bundle_t *bnd = entry->bnd;
        if (count > 0) {
            const char *bndName = celix_bundle_getSymbolicName(bnd);
            fw_log(framework->logger, CELIX_LOG_LEVEL_FATAL, "Cannot destroy framework. The use count of bundle %s (bnd id %li) is not 0, but %u.", bndName, entry->bndId, count);
            celixThreadMutex_lock(&framework->dispatcher.mutex);
            int nrOfRequests = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
            celixThreadMutex_unlock(&framework->dispatcher.mutex);
            fw_log(framework->logger, CELIX_LOG_LEVEL_WARNING, "nr of request left: %i (should be 0).", nrOfRequests);
        }
        fw_bundleEntry_destroy(entry, true);

        bool systemBundle = false;
        bundle_isSystemBundle(bnd, &systemBundle);
        if (systemBundle) {
            bundle_context_t *context = NULL;
            bundle_getContext(framework->bundle, &context);
            bundleContext_destroy(context);
        }

        bundle_archive_t *archive = NULL;
        if (bundle_getArchive(bnd, &archive) == CELIX_SUCCESS) {
            if (!systemBundle) {
                bundle_revision_pt revision = NULL;
                array_list_pt handles = NULL;
                status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
                status = CELIX_DO_IF(status, bundleRevision_getHandles(revision, &handles));
                if (handles != NULL) {
                    for (int h = arrayList_size(handles) - 1; h >= 0; h--) {
                        celix_library_handle_t *handle = arrayList_get(handles, h);
                        celix_libloader_close(handle);
                    }
                }
            }

            bundleArchive_destroy(archive);
        }
        bundle_destroy(bnd);

    }
    celixThreadMutex_unlock(&framework->installedBundles.mutex);
    celix_arrayList_destroy(framework->installedBundles.entries);
    celixThreadMutex_destroy(&framework->installedBundles.mutex);

	hashMap_destroy(framework->installRequestMap, false, false);

    if (framework->bundleListeners) {
        arrayList_destroy(framework->bundleListeners);
    }
    if (framework->frameworkListeners) {
        arrayList_destroy(framework->frameworkListeners);
    }

    assert(celix_arrayList_size(framework->dispatcher.dynamicEventQueue) == 0);
    celix_arrayList_destroy(framework->dispatcher.dynamicEventQueue);

	bundleCache_destroy(&framework->cache);

	celixThreadCondition_destroy(&framework->dispatcher.cond);
    celixThreadMutex_destroy(&framework->frameworkListenersLock);
	celixThreadMutex_destroy(&framework->bundleListenerLock);
	celixThreadMutex_destroy(&framework->dispatcher.mutex);
	celixThreadMutex_destroy(&framework->shutdown.mutex);
	celixThreadCondition_destroy(&framework->shutdown.cond);

    celix_frameworkLogger_destroy(framework->logger);

    properties_destroy(framework->configurationMap);

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

    bool cleanCache = celix_properties_getAsBool(framework->configurationMap, OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME, OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_DEFAULT);
    if (cleanCache) {
        bundleCache_delete(framework->cache);
    }

    celix_array_list_t* archives = NULL;
    celix_status_t status = bundleCache_getArchives(framework->cache, &archives);
    if (status == CELIX_SUCCESS) {
        unsigned int arcIdx;
        for (arcIdx = 0; arcIdx < arrayList_size(archives); arcIdx++) {
            bundle_archive_pt archive1 = (bundle_archive_pt) arrayList_get(archives, arcIdx);
            long id;
            bundle_state_e bundleState;
            bundleArchive_getId(archive1, &id);
            framework->nextBundleId = framework->nextBundleId > id + 1 ? framework->nextBundleId : id + 1;

            bundleArchive_getPersistentState(archive1, &bundleState);
            if (bundleState == OSGI_FRAMEWORK_BUNDLE_UNINSTALLED) {
                bundleArchive_closeAndDelete(archive1);
            } else {
                bundle_pt bundle = NULL;
                const char *location1 = NULL;
                status = bundleArchive_getLocation(archive1, &location1);
                fw_installBundle2(framework, &bundle, id, location1, NULL, archive1);
            }
        }
        arrayList_destroy(archives);
    }


    status = CELIX_DO_IF(status, bundle_setState(framework->bundle, OSGI_FRAMEWORK_BUNDLE_STARTING));

    if (status == CELIX_SUCCESS) {
        celix_bundle_activator_t *activator = calloc(1,(sizeof(*activator)));
        if (activator != NULL) {
            bundle_context_t *validateContext = NULL;
            void * userData = NULL;

			//create_function_pt create = NULL;
			start_function_fp start = (start_function_fp) frameworkActivator_start;
			stop_function_fp stop = (stop_function_fp) frameworkActivator_stop;
			destroy_function_fp destroy = (destroy_function_fp) frameworkActivator_destroy;

            activator->start = start;
            activator->stop = stop;
            activator->destroy = destroy;
            status = CELIX_DO_IF(status, bundle_setActivator(framework->bundle, activator));
            status = CELIX_DO_IF(status, bundle_getContext(framework->bundle, &validateContext));

            if (status == CELIX_SUCCESS) {
                activator->userData = userData;
                if (start != NULL) {
                    start(userData, validateContext);
                }
            } else {
            	free(activator);
            }
        } else {
            status = CELIX_ENOMEM;
        }
    }

    if (status != CELIX_SUCCESS) {
       fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not init framework");
    }

	return status;
}

celix_status_t framework_start(framework_pt framework) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;

	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	if (status == CELIX_SUCCESS) {
	    if ((state == OSGI_FRAMEWORK_BUNDLE_INSTALLED) || (state == OSGI_FRAMEWORK_BUNDLE_RESOLVED)) {
	        status = CELIX_DO_IF(status, fw_init(framework));
        }
	}

	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	if (status == CELIX_SUCCESS && state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
        bundle_setState(framework->bundle, OSGI_FRAMEWORK_BUNDLE_ACTIVE);
	}

	celix_framework_bundle_entry_t* entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, framework->bundleId);
	CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, entry));
    fw_bundleEntry_decreaseUseCount(entry);

	CELIX_DO_IF(status, fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_STARTED, framework->bundleId));

	if (status != CELIX_SUCCESS) {
       status = CELIX_BUNDLE_EXCEPTION;
       fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start framework");
       fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_ERROR, status);
    }

    bundle_context_t *fwCtx = framework_getContext(framework);
	if (fwCtx != NULL) {
        framework_autoStartConfiguredBundles(fwCtx);
    }

	if (status == CELIX_SUCCESS) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_INFO, "Celix framework started");
        fw_log(framework->logger, CELIX_LOG_LEVEL_TRACE, "Celix framework started with uuid %s", celix_framework_getUUID(framework));
	}

	return status;
}

static void framework_autoStartConfiguredBundles(bundle_context_t *fwCtx) {
    const char* cosgiKeys[] = {"cosgi.auto.start.0","cosgi.auto.start.1","cosgi.auto.start.2","cosgi.auto.start.3","cosgi.auto.start.4","cosgi.auto.start.5","cosgi.auto.start.6"};
    const char* celixKeys[] = {CELIX_AUTO_START_0, CELIX_AUTO_START_1, CELIX_AUTO_START_2, CELIX_AUTO_START_3, CELIX_AUTO_START_4, CELIX_AUTO_START_5, CELIX_AUTO_START_6};
    celix_array_list_t *installedBundles = celix_arrayList_create();
    size_t len = 7;
    for (int i = 0; i < len; ++i) {
        const char *autoStart = celix_bundleContext_getProperty(fwCtx, celixKeys[i], NULL);
        if (autoStart == NULL) {
            autoStart = celix_bundleContext_getProperty(fwCtx, cosgiKeys[i], NULL);
        }
        if (autoStart != NULL) {
            framework_autoInstallConfiguredBundlesForList(fwCtx, autoStart, installedBundles);
        }
    }
    framework_autoStartConfiguredBundlesForList(fwCtx, installedBundles);
    celix_arrayList_destroy(installedBundles);
}


static void framework_autoInstallConfiguredBundlesForList(bundle_context_t *fwCtx, const char *autoStartIn, celix_array_list_t *installedBundles) {
    char delims[] = " ";
    char *save_ptr = NULL;
    char *autoStart = celix_utils_strdup(autoStartIn);

    if (autoStart != NULL) {
        char *location = strtok_r(autoStart, delims, &save_ptr);
        while (location != NULL) {
            //first install
            bundle_t *bnd = NULL;
            celix_status_t  rc = bundleContext_installBundle(fwCtx, location, &bnd);
            if (rc == CELIX_SUCCESS) {
                celix_arrayList_add(installedBundles, bnd);
            } else {
                printf("Could not install bundle '%s'\n", location);
            }
            location = strtok_r(NULL, delims, &save_ptr);
        }
    }
    free(autoStart);
}

static void framework_autoStartConfiguredBundlesForList(bundle_context_t *fwCtx, const celix_array_list_t *installedBundles) {
    for (int i = 0; i < celix_arrayList_size(installedBundles); ++i) {
        long bndId = -1;
        bundle_t *bnd = celix_arrayList_get(installedBundles, i);
        bundle_getBundleId(bnd, &bndId);
        celix_status_t rc = bundle_startWithOptions(bnd, 0);
        if (rc != CELIX_SUCCESS) {
            printf("Could not start bundle %li\n", bndId);
        }
    }
}

celix_status_t framework_stop(framework_pt framework) {
	celix_status_t status = fw_stopBundle(framework, framework->bundleId, true);
	if (status == CELIX_SUCCESS) {
	    fw_log(framework->logger, CELIX_LOG_LEVEL_INFO, "Celix framework stopped");
        fw_log(framework->logger, CELIX_LOG_LEVEL_TRACE, "Celix framework stopped for uuid %s", celix_framework_getUUID(framework));
	}
	return status;
}

celix_status_t fw_getProperty(framework_pt framework, const char* name, const char* defaultValue, const char** out) {
	celix_status_t status = CELIX_SUCCESS;


	const char *result = NULL;

	if (framework == NULL || name == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		result = getenv(name); //NOTE that an env environment overrides the config.properties values
		if (result == NULL && framework->configurationMap != NULL) {
		    result = properties_get(framework->configurationMap, name);
		}
                if (result == NULL) {
                    result = defaultValue;
                }
	}

	if (out != NULL) {
		*out = result;
	}

	return status;
}

celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, const char * location, const char *inputFile) {
	return fw_installBundle2(framework, bundle, -1, location, inputFile, NULL);
}

static char* resolveBundleLocation(celix_framework_t *fw, const char *bndLoc, const char *p) {
    char *result = NULL;
    if (strnlen(bndLoc, 1) > 0) {
        if (bndLoc[0] == '/') {
            //absolute path, just use that
            asprintf(&result, "%s", bndLoc);
        } else if (access(bndLoc, F_OK) != -1) {
            //current relative path points to an existing file, so use that.
            result = strndup(bndLoc, 1024*1024);
        } else {
            //relative path -> resolve using BUNDLES_PATH
            char *paths = strndup(p, 1024 * 1024);
            const char *sep = ";";
            char *savePtr = NULL;

            for (char *path = strtok_r(paths, sep, &savePtr); path != NULL; path = strtok_r(NULL, sep, &savePtr)){
                char *absPath = NULL;
                asprintf(&absPath, "%s/%s", path, bndLoc);
                if (access(absPath, F_OK) != -1 ) {
                    result = absPath;
                    break;
                } else {
                    free(absPath);
                }
            }
            free(paths);
        }
    }
    return result;
}

celix_status_t fw_installBundle2(framework_pt framework, bundle_pt * bundle, long id, const char *bndLoc, const char *inputFile, bundle_archive_pt archive) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;

    const char *paths = NULL;
    fw_getProperty(framework, CELIX_BUNDLES_PATH_NAME, CELIX_BUNDLES_PATH_DEFAULT, &paths);
    char *location = resolveBundleLocation(framework, bndLoc, paths);
    if (location == NULL) {
        fw_log(framework->logger, CELIX_LOG_LEVEL_WARNING, "Cannot find bundle %s. Using %s=%s", bndLoc, CELIX_BUNDLES_PATH_NAME, paths);
        free(location);
        return CELIX_FILE_IO_EXCEPTION;
    }

    //increase use count of framework bundle to prevent a stop.
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, framework->bundleId);

  	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
  	if (status == CELIX_SUCCESS) {
        if (state == OSGI_FRAMEWORK_BUNDLE_STOPPING || state == OSGI_FRAMEWORK_BUNDLE_UNINSTALLED) {
            fw_log(framework->logger, CELIX_LOG_LEVEL_INFO,  "The framework is being shutdown");
            status = CELIX_FRAMEWORK_SHUTDOWN;
        }
  	}

    if (status == CELIX_SUCCESS) {
        *bundle = framework_getBundle(framework, location);
        if (*bundle != NULL) {
            fw_bundleEntry_decreaseUseCount(entry);
            free(location);
            return CELIX_SUCCESS;
        }

        if (archive == NULL) {
            id = framework_getNextBundleId(framework);

            status = CELIX_DO_IF(status, bundleCache_createArchive(framework->cache, id, location, inputFile, &archive));

            if (status != CELIX_SUCCESS) {
            	bundleArchive_destroy(archive);
            }
        } else {
            // purge revision
            // multiple revisions not yet implemented
        }

        if (status == CELIX_SUCCESS) {
            status = bundle_createFromArchive(bundle, framework, archive);
        }

        if (status == CELIX_SUCCESS) {
            long bndId = -1L;
            bundle_getBundleId(*bundle, &bndId);
            celix_framework_bundle_entry_t *bEntry = fw_bundleEntry_create(*bundle);
            fw_bundleEntry_increaseUseCount(bEntry);
            celixThreadMutex_lock(&framework->installedBundles.mutex);
            celix_arrayList_add(framework->installedBundles.entries, bEntry);
            celixThreadMutex_unlock(&framework->installedBundles.mutex);
            fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED, bEntry);
            fw_bundleEntry_decreaseUseCount(bEntry);
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            status = CELIX_DO_IF(status, bundleArchive_closeAndDelete(archive));
        }
    }

    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not install bundle");
    }

    fw_bundleEntry_decreaseUseCount(entry);

    free(location);

  	return status;
}

celix_status_t framework_getBundleEntry(framework_pt framework, const_bundle_pt bundle, const char* name, char** entry) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_revision_pt revision;
	bundle_archive_pt archive = NULL;
    const char *root;

	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getRoot(revision, &root));
    if (status == CELIX_SUCCESS) {
        char e[strlen(name) + strlen(root) + 2];
        strcpy(e, root);
        if ((strlen(name) > 0) && (name[0] == '/')) {
            strcat(e, name);
        } else {
            strcat(e, "/");
            strcat(e, name);
        }

        if (access(e, F_OK) == 0) {
            (*entry) = strndup(e, 1024*10);
        } else {
            (*entry) = NULL;
        }
    }

	return status;
}

celix_status_t fw_startBundle(framework_pt framework, long bndId, int options __attribute__((unused))) {
	celix_status_t status = CELIX_SUCCESS;

	linked_list_pt wires = NULL;
	bundle_context_t *context = NULL;
	bundle_state_e state;
	module_pt module = NULL;
    celix_bundle_activator_t *activator = NULL;
	char *error = NULL;
	const char *name = NULL;

    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);

    status = entry->bnd != NULL ? CELIX_SUCCESS : CELIX_ILLEGAL_ARGUMENT;

	status = CELIX_DO_IF(status, bundle_getState(entry->bnd, &state));

	if (status == CELIX_SUCCESS) {
	    switch (state) {
            case OSGI_FRAMEWORK_BUNDLE_UNKNOWN:
                error = "state is unknown";
                status = CELIX_ILLEGAL_STATE;
                break;
            case OSGI_FRAMEWORK_BUNDLE_UNINSTALLED:
                error = "bundle is uninstalled";
                status = CELIX_ILLEGAL_STATE;
                break;
            case OSGI_FRAMEWORK_BUNDLE_STARTING:
                error = "bundle is starting";
                status = CELIX_BUNDLE_EXCEPTION;
                break;
            case OSGI_FRAMEWORK_BUNDLE_STOPPING:
                error = "bundle is stopping";
                status = CELIX_BUNDLE_EXCEPTION;
                break;
            case OSGI_FRAMEWORK_BUNDLE_ACTIVE:
                break;
            case OSGI_FRAMEWORK_BUNDLE_INSTALLED:
                bundle_getCurrentModule(entry->bnd, &module);
                module_getSymbolicName(module, &name);
                if (!module_isResolved(module)) {
                    wires = resolver_resolve(module);
                    if (wires == NULL) {
                        fw_bundleEntry_decreaseUseCount(entry);
                        return CELIX_BUNDLE_EXCEPTION;
                    }
                    status = framework_markResolvedModules(framework, wires);
                    if (status != CELIX_SUCCESS) {
                        break;
                    }
                }
                /* no break */
            case OSGI_FRAMEWORK_BUNDLE_RESOLVED:
                module = NULL;
                name = NULL;
                bundle_getCurrentModule(entry->bnd, &module);
                module_getSymbolicName(module, &name);
                status = CELIX_DO_IF(status, bundleContext_create(framework, framework->logger, entry->bnd, &context));
                status = CELIX_DO_IF(status, bundle_setContext(entry->bnd, context));

                if (status == CELIX_SUCCESS) {
                    activator = calloc(1,(sizeof(*activator)));
                    if (activator == NULL) {
                        status = CELIX_ENOMEM;
                    } else {
                        void * userData = NULL;
                        create_function_fp create = (create_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE);
                        if (create == NULL) {
                            create = celix_libloader_getSymbol(bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_CREATE);
                        }
                        start_function_fp start = (start_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START);
                        if (start == NULL) {
                            start = (start_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_START);
                        }
                        stop_function_fp stop = (stop_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP);
                        if (stop == NULL) {
                            stop = (stop_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_STOP);
                        }
                        destroy_function_fp destroy = (destroy_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY);
                        if (destroy == NULL) {
                            destroy = (destroy_function_fp) celix_libloader_getSymbol((celix_library_handle_t*) bundle_getHandle(entry->bnd), OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_DESTROY);
                        }

                        activator->create = create;
                        activator->start = start;
                        activator->stop = stop;
                        activator->destroy = destroy;

                        status = CELIX_DO_IF(status, bundle_setActivator(entry->bnd, activator));

                        status = CELIX_DO_IF(status, bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_STARTING));
                        CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING, entry));

                        status = CELIX_DO_IF(status, bundle_getContext(entry->bnd, &context));

                        if (status == CELIX_SUCCESS) {
                            if (create != NULL) {
                                status = CELIX_DO_IF(status, create(context, &userData));
                                if (status == CELIX_SUCCESS) {
                                    activator->userData = userData;
                                }
                            }
                        }
                        if (status == CELIX_SUCCESS) {
                            if (start != NULL) {
                                status = CELIX_DO_IF(status, start(userData, context));
                            }
                        }

                        status = CELIX_DO_IF(status, bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_ACTIVE));
                        CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, entry));

                        if (status != CELIX_SUCCESS) {
                            //state is still STARTING, back to resolved
                            bool createCalled = activator != NULL && activator->userData != NULL;
                            if (createCalled) {
                                destroy(activator->userData, context);
                            }
                            bundle_setContext(entry->bnd, NULL);
                            bundle_setActivator(entry->bnd, NULL);
                            bundleContext_destroy(context);
                            free(activator);
                            status = bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
                        }
                    }
                }

            break;
        }
	}

	if (status != CELIX_SUCCESS) {
	    const char *symbolicName = NULL;
	    long id = 0;
	    bundle_getCurrentModule(entry->bnd, &module);
	    module_getSymbolicName(module, &symbolicName);
	    bundle_getBundleId(entry->bnd, &id);
	    if (error != NULL) {
	        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
	    } else {
	        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
	    }
	}

	if (entry != NULL) {
	    fw_bundleEntry_decreaseUseCount(entry);
    }

	return status;
}

celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, const char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e oldState;
	const char *location;
	bundle_archive_pt archive = NULL;
	char *error = NULL;

	long bndId = celix_bundle_getId(bundle);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);

    status = CELIX_DO_IF(status, bundle_getState(bundle, &oldState));
	if (status == CELIX_SUCCESS) {
        if (oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            fw_stopBundle(framework, bndId, false);
        }
	}
	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
	status = CELIX_DO_IF(status, bundleArchive_getLocation(archive, &location));

	status = CELIX_DO_IF(status, bundle_revise(bundle, location, inputFile));

	status = CELIX_DO_IF(status, bundleArchive_setLastModified(archive, time(NULL)));
	status = CELIX_DO_IF(status, bundle_setState(bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED));

	bundle_revision_pt revision = NULL;
	array_list_pt handles = NULL;
	status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getHandles(revision, &handles));
    if (handles != NULL) {
        int i;
	    for (i = celix_arrayList_size(handles) - 1; i >= 0; i--) {
	        void* handle = arrayList_get(handles, i);
	        celix_libloader_close(handle);
	    }
    }


	CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, entry));
	CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UPDATED, entry));

    // Refresh packages?

	if (status == CELIX_SUCCESS) {
	    if (oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
	        status = CELIX_DO_IF(status, fw_startBundle(framework, bndId, 1));
	    }
	}


	fw_bundleEntry_decreaseUseCount(entry);

	if (status != CELIX_SUCCESS) {
	    module_pt module = NULL;
        const char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(bundle, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(bundle, &id);
        if (error != NULL) {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot update bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot update bundle: %s [%ld]", symbolicName, id);
        }
	}

	return status;
}

celix_status_t fw_stopBundle(framework_pt framework, long bndId, bool record) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e state;
    celix_bundle_activator_t *activator = NULL;
    bundle_context_t *context = NULL;
    bool wasActive = false;
    char *error = NULL;

    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);
    if (entry == NULL) {
      status = CELIX_ILLEGAL_STATE;
      fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot stop bundle [%ld]: cannot find bundle entry", bndId);
      return status;
    }

    if (record) {
	    status = CELIX_DO_IF(status, bundle_setPersistentStateInactive(entry->bnd));
    }

	status = CELIX_DO_IF(status, bundle_getState(entry->bnd, &state));
	if (status == CELIX_SUCCESS) {
	    switch (state) {
            case OSGI_FRAMEWORK_BUNDLE_UNKNOWN:
                status = CELIX_ILLEGAL_STATE;
                error = "state is unknown";
                break;
            case OSGI_FRAMEWORK_BUNDLE_UNINSTALLED:
                status = CELIX_ILLEGAL_STATE;
                error = "bundle is uninstalled";
                break;
            case OSGI_FRAMEWORK_BUNDLE_STARTING:
                status = CELIX_BUNDLE_EXCEPTION;
                error = "bundle is starting";
                break;
            case OSGI_FRAMEWORK_BUNDLE_STOPPING:
                status = CELIX_BUNDLE_EXCEPTION;
                error = "bundle is stopping";
                break;
            case OSGI_FRAMEWORK_BUNDLE_INSTALLED:
            case OSGI_FRAMEWORK_BUNDLE_RESOLVED:
                break;
            case OSGI_FRAMEWORK_BUNDLE_ACTIVE:
                wasActive = true;
                break;
        }
	}


	CELIX_DO_IF(status, bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_STOPPING));
	CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING, entry));
	if (status == CELIX_SUCCESS) {
	    if (wasActive) {
	        activator = bundle_getActivator(entry->bnd);

	        status = CELIX_DO_IF(status, bundle_getContext(entry->bnd, &context));
	        if (status == CELIX_SUCCESS) {
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

            if (bndId > 0) {
	            celix_bundleContext_cleanup(entry->bnd->context);
                if (status == CELIX_SUCCESS) {
                    module_pt module = NULL;
                    const char *symbolicName = NULL;
                    long id = 0;
                    bundle_getCurrentModule(entry->bnd, &module);
                    module_getSymbolicName(module, &symbolicName);
                    bundle_getBundleId(entry->bnd, &id);
                }

                if (context != NULL) {
                    status = CELIX_DO_IF(status, bundleContext_destroy(context));
                    status = CELIX_DO_IF(status, bundle_setContext(entry->bnd, NULL));
                }

                status = CELIX_DO_IF(status, bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
                CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, entry));
            } else if (bndId == 0) {
                //framework bundle
                celix_bundleContext_cleanup(entry->bnd->context);

                status = CELIX_DO_IF(status, bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
                CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, entry));
            }
	    }

	    if (activator != NULL) {
	        bundle_setActivator(entry->bnd, NULL);
	        free(activator);
	    }
	}

	if (status != CELIX_SUCCESS) {
	    module_pt module = NULL;
        const char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(entry->bnd, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(entry->bnd, &id);
        if (error != NULL) {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot stop bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot stop bundle: %s [%ld]", symbolicName, id);
        }
 	} else {
        fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, entry);
 	}

	fw_bundleEntry_decreaseUseCount(entry);

	return status;
}

static celix_status_t fw_uninstallBundleEntry(celix_framework_t *framework, celix_framework_bundle_entry_t *entry) {
    celix_status_t status = CELIX_SUCCESS;

    celix_bundle_t *bnd = NULL;
    long bndId = -1L;

    if (entry == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        bnd = entry->bnd;
        bndId = entry->bndId;
    }

    if (status == CELIX_SUCCESS) {
        bundle_archive_t *archive = NULL;
        bundle_revision_t *revision = NULL;
        array_list_pt handles = NULL;
        status = CELIX_DO_IF(status, bundle_getArchive(bnd, &archive));
        status = CELIX_DO_IF(status, bundleArchive_setPersistentState(archive, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED)); //set state to uninstalled, so that next framework start will not start bundle.
        status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
        status = CELIX_DO_IF(status, bundleRevision_getHandles(revision, &handles));
        if (handles != NULL) {
            for (int i = celix_arrayList_size(handles) - 1; i >= 0; i--) {
                celix_library_handle_t *handle = arrayList_get(handles, i);
                celix_libloader_close(handle);
            }
        }

        CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, entry));

        status = CELIX_DO_IF(status, bundle_setState(bnd, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED));
        status = CELIX_DO_IF(status, bundleArchive_setLastModified(archive, time(NULL)));

        CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED, entry));

        //NOTE wait outside installedBundles.mutex
        fw_bundleEntry_decreaseUseCount(entry);
        fw_bundleEntry_destroy(entry, true); //wait till use count is 0 -> e.g. not used

        if (status == CELIX_SUCCESS) {
            celix_status_t refreshStatus = fw_refreshBundle(framework, bndId);
            if (refreshStatus != CELIX_SUCCESS) {
                printf("Could not refresh bundle");
            } else {
                celix_framework_waitForEmptyEventQueue(framework); //to ensure that the uninstall event is triggered and handled
                bundleArchive_destroy(archive);
                status = CELIX_DO_IF(status, bundle_closeModules(bnd));
                status = CELIX_DO_IF(status, bundle_destroy(bnd));
            }
        }
    }


    if (status != CELIX_SUCCESS) {
        framework_logIfError(framework->logger, status, "", "Cannot uninstall bundle");
    }

    return status;
}

celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle) {
    long bndId = celix_bundle_getId(bundle);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_removeBundleEntryAndIncreaseUseCount(framework, bndId);

    if (entry != NULL) {
        return fw_uninstallBundleEntry(framework, entry);
    } else {
        return CELIX_ILLEGAL_ARGUMENT;
    }
}

//celix_status_t fw_refreshBundles(framework_pt framework, long bundleIds[], int size) {
//    celix_status_t status = CELIX_SUCCESS;
//
//    hash_map_values_pt values;
//    bundle_pt *newTargets;
//    unsigned int nrofvalues;
//    bool restart = false;
//    hash_map_pt map = hashMap_create(NULL, NULL, NULL, NULL);
//    int targetIdx = 0;
//    for (targetIdx = 0; targetIdx < size; targetIdx++) {
//        long bndId = bundles[targetIdx];
//        hashMap_put(map, bundle, bundle);
//        fw_populateDependentGraph(framework, bundle, &map);
//    }
//    values = hashMapValues_create(map);
//    hashMapValues_toArray(values, (void ***) &newTargets, &nrofvalues);
//    hashMapValues_destroy(values);
//
//    hashMap_destroy(map, false, false);
//
//    if (newTargets != NULL) {
//        int i = 0;
//        struct fw_refreshHelper * helpers;
//        for (i = 0; i < nrofvalues && !restart; i++) {
//            bundle_pt bundle = (bundle_pt) newTargets[i];
//            if (framework->bundle == bundle) {
//                restart = true;
//            }
//        }
//
//        helpers = (struct fw_refreshHelper * )malloc(nrofvalues * sizeof(struct fw_refreshHelper));
//        for (i = 0; i < nrofvalues && !restart; i++) {
//            bundle_pt bundle = (bundle_pt) newTargets[i];
//            helpers[i].framework = framework;
//            helpers[i].bundle = bundle;
//            helpers[i].oldState = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
//        }
//
//        for (i = 0; i < nrofvalues; i++) {
//            struct fw_refreshHelper helper = helpers[i];
//            fw_refreshHelper_stop(&helper);
//            fw_refreshHelper_refreshOrRemove(&helper);
//        }
//
//        for (i = 0; i < nrofvalues; i++) {
//            struct fw_refreshHelper helper = helpers[i];
//            fw_refreshHelper_restart(&helper);
//        }
//
//        if (restart) {
//            bundle_update(framework->bundle, NULL);
//        }
//        free(helpers);
//        free(newTargets);
//    }
//
//    framework_logIfError(framework->logger, status, NULL, "Cannot refresh bundles");
//
//    return status;
//}

celix_status_t fw_refreshBundle(framework_pt framework, long bndId) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state;


    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);
    if (entry != NULL) {
        bool fire;
        bundle_getState(entry->bnd, &state);
        fire = (state != OSGI_FRAMEWORK_BUNDLE_INSTALLED);
        bundle_refresh(entry->bnd);

        if (fire) {
            bundle_setState(entry->bnd, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
            fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, entry);
        }

        fw_bundleEntry_decreaseUseCount(entry);
    } else {
        framework_logIfError(framework->logger, status, NULL, "Cannot refresh bundle");
    }


    return status;
}

celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper) {
	bundle_state_e state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
        refreshHelper->oldState = OSGI_FRAMEWORK_BUNDLE_ACTIVE;
        long bndId = celix_bundle_getId(refreshHelper->bundle);
        fw_stopBundle(refreshHelper->framework, bndId, false);
    }

    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper) {
	bundle_state_e state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == OSGI_FRAMEWORK_BUNDLE_UNINSTALLED) {
        bundle_closeAndDelete(refreshHelper->bundle);
        refreshHelper->bundle = NULL;
    } else {
        long bndId = celix_bundle_getId(refreshHelper->bundle);
        fw_refreshBundle(refreshHelper->framework, bndId);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper) {
    if ((refreshHelper->bundle != NULL) && (refreshHelper->oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE)) {
        long bndId = celix_bundle_getId(refreshHelper->bundle);
        fw_startBundle(refreshHelper->framework, bndId, 0);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_getDependentBundles(framework_pt framework, bundle_pt exporter, array_list_pt *list) {
    celix_status_t status = CELIX_SUCCESS;

    if (*list != NULL || exporter == NULL || framework == NULL) {
	return CELIX_ILLEGAL_ARGUMENT;
    }

	 array_list_pt modules;
	 unsigned int modIdx = 0;
	 arrayList_create(list);

	 modules = bundle_getModules(exporter);
	 for (modIdx = 0; modIdx < arrayList_size(modules); modIdx++) {
				module_pt module = (module_pt) arrayList_get(modules, modIdx);
				array_list_pt dependents = module_getDependents(module);
			if(dependents!=NULL){
					unsigned int depIdx = 0;
					for (depIdx = 0; depIdx < arrayList_size(dependents); depIdx++) {
							  module_pt dependent = (module_pt) arrayList_get(dependents, depIdx);
							  arrayList_add(*list, module_getBundle(dependent));
					}
					arrayList_destroy(dependents);
				}
	 }

    framework_logIfError(framework->logger, status, NULL, "Cannot get dependent bundles");

    return status;
}

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map) {
    celix_status_t status = CELIX_SUCCESS;

    if(framework == NULL || exporter == NULL){
	return CELIX_ILLEGAL_ARGUMENT;
    }

    array_list_pt dependents = NULL;
    if ((status = fw_getDependentBundles(framework, exporter, &dependents)) == CELIX_SUCCESS) {
		  if(dependents!=NULL){
         unsigned int depIdx = 0;
		for (depIdx = 0; depIdx < arrayList_size(dependents); depIdx++) {
		    if (!hashMap_containsKey(*map, arrayList_get(dependents, depIdx))) {
		        hashMap_put(*map, arrayList_get(dependents, depIdx), arrayList_get(dependents, depIdx));
		        fw_populateDependentGraph(framework, (bundle_pt) arrayList_get(dependents, depIdx), map);
		    }
		}
		arrayList_destroy(dependents);
		  }
    }

    framework_logIfError(framework->logger, status, NULL, "Cannot populate dependent graph");

    return status;
}

celix_status_t fw_registerService(framework_pt framework, service_registration_pt *registration, long bndId, const char* serviceName, const void* svcObj, celix_properties_t *properties) {
	celix_status_t status = CELIX_SUCCESS;
	char *error = NULL;
	if (serviceName == NULL || svcObj == NULL) {
	    status = CELIX_ILLEGAL_ARGUMENT;
	    error = "ServiceName and SvcObj cannot be null";
	}

    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);
    status = CELIX_DO_IF(status, serviceRegistry_registerService(framework->registry, entry->bnd, serviceName, svcObj, properties, registration));
	fw_bundleEntry_decreaseUseCount(entry);
    framework_logIfError(framework->logger, status, error, "Cannot register service: %s", serviceName);
	return status;
}

celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt *registration, long bndId, const char* serviceName, service_factory_pt factory, properties_pt properties) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
	if (serviceName == NULL || factory == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        error = "Service name and factory cannot be null";
    }

    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);

	status = CELIX_DO_IF(status, serviceRegistry_registerServiceFactory(framework->registry, entry->bnd, serviceName, factory, properties, registration));

	fw_bundleEntry_decreaseUseCount(entry);

    framework_logIfError(framework->logger, status, error, "Cannot register service factory: %s", serviceName);

    return CELIX_SUCCESS;
}

celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char * serviceName, const char * sfilter) {
    celix_status_t status = CELIX_SUCCESS;

	filter_pt filter = NULL;
	unsigned int refIdx = 0;

    if (sfilter != NULL) {
        filter = filter_create(sfilter);
	}

	status = CELIX_DO_IF(status, serviceRegistry_getServiceReferences(framework->registry, bundle, serviceName, filter, references));

	if (filter != NULL) {
		filter_destroy(filter);
	}

	if (status == CELIX_SUCCESS) {
        for (refIdx = 0; (*references != NULL) && refIdx < arrayList_size(*references); refIdx++) {
            service_reference_pt ref = (service_reference_pt) arrayList_get(*references, refIdx);
            service_registration_pt reg = NULL;
            const char* serviceNameObjectClass;
            properties_pt props = NULL;
            status = CELIX_DO_IF(status, serviceReference_getServiceRegistration(ref, &reg));
            status = CELIX_DO_IF(status, serviceRegistration_getProperties(reg, &props));
            if (status == CELIX_SUCCESS) {
                serviceNameObjectClass = properties_get(props, OSGI_FRAMEWORK_OBJECTCLASS);
                if (!serviceReference_isAssignableTo(ref, bundle, serviceNameObjectClass)) {
                    arrayList_remove(*references, refIdx);
                    refIdx--;
                }
            }
        }
	}

	framework_logIfError(framework->logger, status, NULL, "Failed to get service references");

	return status;
}

celix_status_t framework_ungetServiceReference(framework_pt framework, bundle_pt bundle, service_reference_pt reference) {
    return serviceRegistry_ungetServiceReference(framework->registry, bundle, reference);
}

celix_status_t fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, const void **service) {
	return serviceRegistry_getService(framework->registry, bundle, reference, service);
}

celix_status_t fw_getBundleRegisteredServices(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	return serviceRegistry_getRegisteredServices(framework->registry, bundle, services);
}

celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	status = serviceRegistry_getServicesInUse(framework->registry, bundle, services);
	return status;
}

celix_status_t framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, bool *result) {
	return serviceRegistry_ungetService(framework->registry, bundle, reference, result);
}

void fw_addServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener, const char* sfilter) {
    celix_serviceRegistry_addServiceListener(framework->registry, bundle, sfilter, listener);
}

void fw_removeServiceListener(framework_pt framework, bundle_pt bundle __attribute__((unused)), celix_service_listener_t *listener) {
    celix_serviceRegistry_removeServiceListener(framework->registry, listener);
}


celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
    typedef struct {
        celix_framework_bundle_entry_t* entry;
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
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(framework->installedBundles.entries, i);
        fw_bundleEntry_increaseUseCount(entry);
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
        if (state >= OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED;
            listener->bundleChanged(listener, &event);
        }
        if (state >= OSGI_FRAMEWORK_BUNDLE_RESOLVED) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED;
            listener->bundleChanged(listener, &event);
        }
        if (state >= OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            event.type = OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED;
            listener->bundleChanged(listener, &event);
        }
        fw_bundleEntry_decreaseUseCount(installedEntry->entry);
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
    for (int i = 0; i < arrayList_size(framework->bundleListeners); i++) {
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
        arrayList_add(framework->frameworkListeners, frameworkListener);
        celixThreadMutex_unlock(&framework->frameworkListenersLock);
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to add framework listener");

    return status;
}

celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i;
    fw_framework_listener_pt frameworkListener;

    celixThreadMutex_lock(&framework->frameworkListenersLock);
    for (i = 0; i < arrayList_size(framework->frameworkListeners); i++) {
        frameworkListener = (fw_framework_listener_pt) arrayList_get(framework->frameworkListeners, i);
        if (frameworkListener->listener == listener && frameworkListener->bundle == bundle) {
            arrayList_remove(framework->frameworkListeners, i);

            frameworkListener->bundle = NULL;
            frameworkListener->listener = NULL;
            free(frameworkListener);
        }
    }
    celixThreadMutex_unlock(&framework->frameworkListenersLock);

    framework_logIfError(framework->logger, status, NULL, "Failed to remove framework listener");

    return status;
}

//celix_status_t fw_isServiceAssignable(framework_pt fw, bundle_pt requester, service_reference_pt reference, bool *assignable) {
//	celix_status_t status = CELIX_SUCCESS;
//
//	*assignable = true;
//	service_registration_pt registration = NULL;
//	status = serviceReference_getServiceRegistration(reference, &registration);
//	if (status == CELIX_SUCCESS) {
//		char *serviceName = properties_get(registration->properties, (char *) OBJECTCLASS);
//		if (!serviceReference_isAssignableTo(reference, requester, serviceName)) {
//			*assignable = false;
//		}
//	}
//
//	return status;
//}

long framework_getNextBundleId(framework_pt framework) {
    long id = framework->nextBundleId;
    framework->nextBundleId++;
    return id;
}

celix_status_t framework_markResolvedModules(framework_pt framework, linked_list_pt resolvedModuleWireMap) {
    celix_status_t status = CELIX_SUCCESS;
    if (resolvedModuleWireMap != NULL) {
        // hash_map_iterator_pt iterator = hashMapIterator_create(resolvedModuleWireMap);
        linked_list_iterator_pt iterator = linkedListIterator_create(resolvedModuleWireMap, linkedList_size(resolvedModuleWireMap));
        while (linkedListIterator_hasPrevious(iterator)) {
            importer_wires_pt iw = linkedListIterator_previous(iterator);
            // hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
            module_pt module = iw->importer;

//			bundle_pt bundle = module_getBundle(module);
//			bundle_archive_pt archive = NULL;
//			bundle_getArchive(bundle, &archive);
//			bundle_revision_pt revision = NULL;
//			bundleArchive_getCurrentRevision(archive, &revision);
//			char *root = NULL;
//			bundleRevision_getRoot(revision, &root);
//			manifest_pt manifest = NULL;
//			bundleRevision_getManifest(revision, &manifest);
//
//			char *private = manifest_getValue(manifest, OSGI_FRAMEWORK_PRIVATE_LIBRARY);
//			char *export = manifest_getValue(manifest, OSGI_FRAMEWORK_EXPORT_LIBRARY);
//
//			printf("Root %s\n", root);

            // for each library update the reference to the wires, if there are any

            linked_list_pt wires = iw->wires;

//			linked_list_iterator_pt wit = linkedListIterator_create(wires, 0);
//			while (linkedListIterator_hasNext(wit)) {
//			    wire_pt wire = linkedListIterator_next(wit);
//			    module_pt importer = NULL;
//			    requirement_pt requirement = NULL;
//			    module_pt exporter = NULL;
//                capability_pt capability = NULL;
//			    wire_getImporter(wire, &importer);
//			    wire_getRequirement(wire, &requirement);
//
//			    wire_getExporter(wire, &exporter);
//			    wire_getCapability(wire, &capability);
//
//			    char *importerName = NULL;
//			    module_getSymbolicName(importer, &importerName);
//
//			    char *exporterName = NULL;
//                module_getSymbolicName(exporter, &exporterName);
//
//                version_pt version = NULL;
//                char *name = NULL;
//                capability_getServiceName(capability, &name);
//                capability_getVersion(capability, &version);
//                char *versionString = NULL;
//                version_toString(version, framework->mp, &versionString);
//
//                printf("Module %s imports library %s:%s from %s\n", importerName, name, versionString, exporterName);
//			}

            if (status == CELIX_SUCCESS) {
                module_setWires(module, wires);
                resolver_moduleResolved(module);
                const char *mname = NULL;
                module_getSymbolicName(module, &mname);
                status = framework_markBundleResolved(framework, module);
                if (status == CELIX_SUCCESS) {
                    module_setResolved(module);
                }
            }
            linkedListIterator_remove(iterator);
            free(iw);
        }
        linkedListIterator_destroy(iterator);
        linkedList_destroy(resolvedModuleWireMap);
    }
    return status;
}

celix_status_t framework_markBundleResolved(framework_pt framework, module_pt module) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_pt bundle = module_getBundle(module);
    bundle_state_e state;
    char *error = NULL;

    if (bundle != NULL) {

        long bndId = celix_bundle_getId(bundle);
        celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, bndId);

        bundle_getState(bundle, &state);
        if (state != OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
            printf("Trying to resolve a resolved bundle");
            status = CELIX_ILLEGAL_STATE;
        } else {
            // Load libraries of this module
            bool isSystemBundle = false;
            bundle_isSystemBundle(bundle, &isSystemBundle);
            if (!isSystemBundle) {
                status = CELIX_DO_IF(status, framework_loadBundleLibraries(framework, bundle));
            }

            status = CELIX_DO_IF(status, bundle_setState(bundle, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
            CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED, entry));
        }

        if (status != CELIX_SUCCESS) {
            const char *symbolicName = NULL;
            long id = 0;
            module_getSymbolicName(module, &symbolicName);
            bundle_getBundleId(bundle, &id);
            if (error != NULL) {
                fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
            } else {
                fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
            }
        }


        fw_bundleEntry_decreaseUseCount(entry);
    }

    return status;
}

array_list_pt framework_getBundles(framework_pt framework) {
    //FIXME Note that this does not increase the use count of the bundle, which can lead to race conditions.
    //promote to use the celix_bundleContext_useBundle(s) functions and deprecated this one
    array_list_pt bundles = NULL;
    arrayList_create(&bundles);

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    int size = celix_arrayList_size(framework->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celix_arrayList_add(bundles, entry->bnd);
    }
    celixThreadMutex_unlock(&framework->installedBundles.mutex);

    return bundles;
}

bundle_pt framework_getBundle(framework_pt framework, const char* location) {
    //FIXME Note that this does not increase the use count of the bundle, which can lead to race conditions.
    //promote to use the celix_bundleContext_useBundle(s) functions and deprecated this one
    bundle_t *bnd = NULL;

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    int size = celix_arrayList_size(framework->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(framework->installedBundles.entries, i);
        const char *loc = NULL;
        bundle_getBundleLocation(entry->bnd, &loc);
        if (loc != NULL && location != NULL && strncmp(loc, location, strlen(loc)) == 0) {
            bnd = entry->bnd;
            break;
        }
    }
    celixThreadMutex_unlock(&framework->installedBundles.mutex);


    return bnd;
}

celix_status_t framework_waitForStop(framework_pt framework) {
    celixThreadMutex_lock(&framework->shutdown.mutex);
    while (!framework->shutdown.done) {
        celixThreadCondition_wait(&framework->shutdown.cond, &framework->shutdown.mutex);
    }
    if (!framework->shutdown.joined) {
        celixThread_join(framework->shutdown.thread, NULL);
        framework->shutdown.joined = true;
    }

    celixThreadMutex_unlock(&framework->shutdown.mutex);

    return CELIX_SUCCESS;
}

static void* framework_shutdown(void *framework) {
    framework_pt fw = (framework_pt) framework;

    fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Celix framework shutting down");

    celix_array_list_t *stopEntries = celix_arrayList_create();
    celix_framework_bundle_entry_t *fwEntry = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    int size = celix_arrayList_size(fw->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(fw->installedBundles.entries, i);
        fw_bundleEntry_increaseUseCount(entry);
        if (entry->bndId != 0) { //i.e. not framework bundle
            celix_arrayList_add(stopEntries, entry);
        } else {
            fwEntry = entry;
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);


    size = celix_arrayList_size(stopEntries);
    for (int i = size-1; i >= 0; --i) { //note loop in reverse order -> stop later installed bundle first
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(stopEntries, i);

        bundle_t *bnd = entry->bnd;

        //NOTE possible starvation.
        fw_bundleEntry_waitTillUseCountIs(entry, 1);  //note this function has 1 use count.

        bundle_state_e state;
        bundle_getState(bnd, &state);
        if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE || state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
            fw_stopBundle(fw, entry->bndId, 0);
        }
        bundle_close(bnd);
        fw_bundleEntry_decreaseUseCount(entry);
    }
    celix_arrayList_destroy(stopEntries);


    // 'stop' framework bundle
    if (fwEntry != NULL) {
        bundle_t *bnd = fwEntry->bnd;
        fw_bundleEntry_waitTillUseCountIs(fwEntry, 1); //note this function has 1 use count.

        bundle_state_e state;
        bundle_getState(bnd, &state);
        if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE || state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
            fw_stopBundle(fw, fwEntry->bndId, 0);
        }
        bundle_close(bnd);
        fw_bundleEntry_decreaseUseCount(fwEntry);
    }

    //join dispatcher thread
    celixThreadMutex_lock(&fw->dispatcher.mutex);
    fw->dispatcher.active = false;
    celixThreadCondition_broadcast(&fw->dispatcher.cond);
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    celixThread_join(fw->dispatcher.thread, NULL);
    fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Joined event loop thread for framework %s", celix_framework_getUUID(framework));


    celixThreadMutex_lock(&fw->shutdown.mutex);
    fw->shutdown.done = true;
    celixThreadCondition_broadcast(&fw->shutdown.cond);
    celixThreadMutex_unlock(&fw->shutdown.mutex);

    celixThread_exit(NULL);
    return NULL;
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

void fw_fireBundleEvent(framework_pt framework, bundle_event_type_e eventType, celix_framework_bundle_entry_t* entry) {
    if (eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING || eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED || eventType == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED) {
        if (entry->bndId == framework->bundleId) {
            //NOTE for framework bundle not triggering events while framework is stopped (and as result in use)
            return;
        }
    }

    fw_bundleEntry_increaseUseCount(entry);

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_BUNDLE_EVENT_TYPE;
    event.bndEntry = entry;
    event.bundleEvent = eventType;
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
    } else if (fw->dispatcher.eventQueueSize < CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE) {
        size_t index = (fw->dispatcher.eventQueueFirstEntry + fw->dispatcher.eventQueueSize) %
                       CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
        fw->dispatcher.eventQueue[index] = *event; //shallow copy
        fw->dispatcher.eventQueueSize += 1;
    } else {
        //static queue is full, dynamics queue is empty. Add first entry to dynamic queue
        fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING,
               "Static event queue for celix framework is full, falling back to dynamic allocated events. Increase static event queue size, current size is %i", CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE);
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
            fw_bundle_listener_pt listener = arrayList_get(framework->bundleListeners, i);
            fw_bundleListener_increaseUseCount(listener);
            celix_arrayList_add(localListeners, listener);
        }
        celixThreadMutex_unlock(&framework->bundleListenerLock);
        for (int i = 0; i < celix_arrayList_size(localListeners); ++i) {
            fw_bundle_listener_pt listener = arrayList_get(localListeners, i);

            bundle_event_t bEvent;
            memset(&bEvent, 0, sizeof(bEvent));
            bEvent.bnd = event->bndEntry->bnd;
            bEvent.type = event->bundleEvent;
            fw_invokeBundleListener(framework, listener->listener, &bEvent, listener->bundle);

            fw_bundleListener_decreaseUseCount(listener);
        }
        celix_arrayList_destroy(localListeners);
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
    } else if (event->type == CELIX_REGISTER_SERVICE_EVENT) {
        service_registration_t* reg = NULL;
        celix_status_t status;
        if (event->factory != NULL) {
            status = celix_serviceRegistry_registerServiceFactory(framework->registry, event->bndEntry->bnd, event->serviceName, event->factory, event->properties, event->registerServiceId, &reg);
        } else {
            status = celix_serviceRegistry_registerService(framework->registry, event->bndEntry->bnd, event->serviceName, event->svc, event->properties, event->registerServiceId, &reg);
        }
        if (status != CELIX_SUCCESS) {
            fw_log(framework->logger, CELIX_LOG_LEVEL_ERROR, "Could not register service async. svc name is %s, error is %s", event->serviceName, celix_strerror(status));
        } else if (event->registerCallback != NULL) {
            event->registerCallback(event->registerData, serviceRegistration_getServiceId(reg));
        }
    } else if (event->type == CELIX_UNREGISTER_SERVICE_EVENT) {
        celix_serviceRegistry_unregisterService(framework->registry, event->bndEntry->bnd, event->unregisterServiceId);
    } else if (event->type == CELIX_GENERIC_EVENT) {
        if (event->genericProcess != NULL) {
            event->genericProcess(event->genericProcessData);
        }
    }

    if (event->doneCallback != NULL) {
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
        fw->dispatcher.eventQueueFirstEntry = (fw->dispatcher.eventQueueFirstEntry+1) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
        fw->dispatcher.eventQueueSize -= 1;
    } else if (celix_arrayList_size(fw->dispatcher.dynamicEventQueue) > 0) {
        celix_arrayList_removeAt(fw->dispatcher.dynamicEventQueue, 0);
        dynamicallyAllocated = true;
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
    return dynamicallyAllocated;
}


static inline void fw_handleEvents(celix_framework_t* framework) {
    celixThreadMutex_lock(&framework->dispatcher.mutex);
    int size = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
    if (size == 0) {
        celixThreadCondition_timedwaitRelative(&framework->dispatcher.cond, &framework->dispatcher.mutex, 1, 0);
    }
    size = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    while (size > 0) {
        celix_framework_event_t* topEvent = fw_topEventFromQueue(framework);
        fw_handleEventRequest(framework, topEvent);
        bool dynamiclyAllocatedEvent = fw_removeTopEventFromQueue(framework);

        if (topEvent->bndEntry != NULL) {
            fw_bundleEntry_decreaseUseCount(topEvent->bndEntry);
        }
        free(topEvent->serviceName);
        if (dynamiclyAllocatedEvent) {
            free(topEvent);
        }

        celixThreadMutex_lock(&framework->dispatcher.mutex);
        size = framework->dispatcher.eventQueueSize + celix_arrayList_size(framework->dispatcher.dynamicEventQueue);
        celixThreadCondition_broadcast(&framework->dispatcher.cond);
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }
}

static void *fw_eventDispatcher(void *fw) {
    framework_pt framework = (framework_pt) fw;

    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool active = framework->dispatcher.active;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    while (active) {
        fw_handleEvents(framework);
        celixThreadMutex_lock(&framework->dispatcher.mutex);
        active = framework->dispatcher.active;
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }

    //not active any more, last run for possible request left overs
    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool needLastRun = framework->dispatcher.eventQueueSize > 0 || celix_arrayList_size(framework->dispatcher.dynamicEventQueue) > 0;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);
    if (needLastRun) {
        fw_handleEvents(framework);
    }

    celixThread_exit(NULL);
    return NULL;

}

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle) {
    // We only support async bundle listeners for now
    bundle_state_e state;
    celix_status_t ret = bundle_getState(bundle, &state);
    if (state == OSGI_FRAMEWORK_BUNDLE_STARTING || state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {

        listener->bundleChanged(listener, event);
    }

    return ret;
}

celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle) {
    bundle_state_e state;
    celix_status_t ret = bundle_getState(bundle, &state);
    if (state == OSGI_FRAMEWORK_BUNDLE_STARTING || state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
        listener->frameworkEvent(listener, event);
    }

    return ret;
}

static celix_status_t frameworkActivator_start(void * userData, bundle_context_t *context) {
    // nothing to do
    return CELIX_SUCCESS;
}

static celix_status_t frameworkActivator_stop(void * userData, bundle_context_t *context) {
    celix_status_t status = CELIX_SUCCESS;
    framework_pt framework;

    if (bundleContext_getFramework(context, &framework) == CELIX_SUCCESS) {

        fw_log(framework->logger, CELIX_LOG_LEVEL_TRACE, "Start shutdown thread for framework %s", celix_framework_getUUID(framework));

        celixThreadMutex_lock(&framework->shutdown.mutex);
        bool alreadyInitialized = framework->shutdown.initialized;
        framework->shutdown.initialized = true;
        celixThreadMutex_unlock(&framework->shutdown.mutex);

        if (!alreadyInitialized) {
            celixThread_create(&framework->shutdown.thread, NULL, framework_shutdown, framework);
        }
    } else {
        status = CELIX_FRAMEWORK_EXCEPTION;
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to stop framework activator");

    return status;
}

static celix_status_t frameworkActivator_destroy(void * userData, bundle_context_t *context) {
    return CELIX_SUCCESS;
}


static celix_status_t framework_loadBundleLibraries(framework_pt framework, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    celix_library_handle_t* handle = NULL;
    bundle_archive_pt archive = NULL;
    bundle_revision_pt revision = NULL;
    manifest_pt manifest = NULL;

    status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getManifest(revision, &manifest));
    if (status == CELIX_SUCCESS) {
        const char *privateLibraries = NULL;
        const char *exportLibraries = NULL;
        const char *activator = NULL;

        privateLibraries = manifest_getValue(manifest, OSGI_FRAMEWORK_PRIVATE_LIBRARY);
        exportLibraries = manifest_getValue(manifest, OSGI_FRAMEWORK_EXPORT_LIBRARY);
        activator = manifest_getValue(manifest, OSGI_FRAMEWORK_BUNDLE_ACTIVATOR);

        if (exportLibraries != NULL) {
            status = CELIX_DO_IF(status, framework_loadLibraries(framework, exportLibraries, activator, archive, &handle));
        }

        if (privateLibraries != NULL) {
            status = CELIX_DO_IF(status,
                                 framework_loadLibraries(framework, privateLibraries, activator, archive, &handle));
        }

        if (status == CELIX_SUCCESS) {
            bundle_setHandle(bundle, handle);
        } else if (handle != NULL) {
            celix_libloader_close(handle);
        }
    }

    framework_logIfError(framework->logger, status, NULL, "Could not load all bundle libraries");

    return status;
}

static celix_status_t framework_loadLibraries(framework_pt framework, const char *librariesIn, const char *activator, bundle_archive_pt archive, void **activatorHandle) {
    celix_status_t status = CELIX_SUCCESS;

    char* last;
    char* libraries = strndup(librariesIn, 1024*10);
    char* token = strtok_r(libraries, ",", &last);
    while (token != NULL) {
        void *handle = NULL;
        char lib[128];
        lib[127] = '\0';

        char *path = NULL;
        char *pathToken = strtok_r(token, ";", &path);
        strncpy(lib, pathToken, 127);
        pathToken = strtok_r(NULL, ";", &path);

        while (pathToken != NULL) {

            /*Disable version should be part of the lib name
            if (strncmp(pathToken, "version", 7) == 0) {
                char *ver = strdup(pathToken);
                char version[strlen(ver) - 9];
                strncpy(version, ver+9, strlen(ver) - 10);
                version[strlen(ver) - 10] = '\0';

                strcat(lib, "-");
                strcat(lib, version);
            }*/
            pathToken = strtok_r(NULL, ";", &path);
        }

        char *trimmedLib = utils_stringTrim(lib);
        status = framework_loadLibrary(framework, trimmedLib, archive, &handle);

        if ( (status == CELIX_SUCCESS) && (activator != NULL) && (strcmp(trimmedLib, activator) == 0) ) {
            *activatorHandle = handle;
        }
        else if(handle!=NULL){
            celix_libloader_close(handle);
        }

        token = strtok_r(NULL, ",", &last);
    }

    free(libraries);
    return status;
}

static celix_status_t framework_loadLibrary(framework_pt framework, const char *library, bundle_archive_pt archive, void **handle) {
    celix_status_t status = CELIX_SUCCESS;
    const char *error = NULL;

#ifdef __linux__
    char * library_prefix = "lib";
    char * library_extension = ".so";
#elif __APPLE__
    char * library_prefix = "lib";
        char * library_extension = ".dylib";
    #elif WIN32
        char * library_prefix = "";
        char * library_extension = ".dll";
#endif

    char libraryPath[256];
    long refreshCount = 0;
    const char *archiveRoot = NULL;
    long revisionNumber = 0;

    status = CELIX_DO_IF(status, bundleArchive_getRefreshCount(archive, &refreshCount));
    status = CELIX_DO_IF(status, bundleArchive_getArchiveRoot(archive, &archiveRoot));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevisionNumber(archive, &revisionNumber));

    memset(libraryPath, 0, 256);
    int written = 0;
    if (strncmp("lib", library, 3) == 0) {
        written = snprintf(libraryPath, 256, "%s/version%ld.%ld/%s", archiveRoot, refreshCount, revisionNumber, library);
    } else {
        written = snprintf(libraryPath, 256, "%s/version%ld.%ld/%s%s%s", archiveRoot, refreshCount, revisionNumber, library_prefix, library, library_extension);
    }

    if (written >= 256) {
        error = "library path is too long";
        status = CELIX_FRAMEWORK_EXCEPTION;
    } else {
        celix_bundle_context_t *fwCtx = NULL;
        bundle_getContext(framework->bundle, &fwCtx);
        *handle = celix_libloader_open(fwCtx, libraryPath);
        if (*handle == NULL) {
            error = celix_libloader_getLastError();
            status =  CELIX_BUNDLE_EXCEPTION;
        } else {
            bundle_revision_pt revision = NULL;
            array_list_pt handles = NULL;

            status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
            status = CELIX_DO_IF(status, bundleRevision_getHandles(revision, &handles));

            if(handles != NULL){
                arrayList_add(handles, *handle);
            }
        }
    }

    framework_logIfError(framework->logger, status, error, "Could not load library: %s", libraryPath);

    return status;
}






/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/


void celix_framework_useBundles(framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    celix_array_list_t *bundleIds = celix_arrayList_create();

    celixThreadMutex_lock(&fw->installedBundles.mutex);
    int size = celix_arrayList_size(fw->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId > 0 || includeFrameworkBundle) {
            //NOTE bundle state is checked in celix_framework_useBundles
            celix_arrayList_addLong(bundleIds, entry->bndId);
        }
    }
    celixThreadMutex_unlock(&fw->installedBundles.mutex);

    //note that stored bundle ids can now already be invalid (race cond),
    //but the celix_framework_useBundle function should be able to handle this safely.

    size = celix_arrayList_size(bundleIds);
    for (int i = 0; i < size; ++i) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        celix_framework_useBundle(fw, true, bndId, callbackHandle, use);
    }

    celix_arrayList_destroy(bundleIds);
}

bool celix_framework_useBundle(framework_t *fw, bool onlyActive, long bundleId, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
    bool called = false;
    if (bundleId >= 0) {
        celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bundleId);

        if (entry != NULL) {
            celix_bundle_state_e bndState = celix_bundle_getState(entry->bnd);
            if (onlyActive && (bndState == OSGI_FRAMEWORK_BUNDLE_ACTIVE || bndState == OSGI_FRAMEWORK_BUNDLE_STARTING)) {
                use(callbackHandle, entry->bnd);
                called = true;
            } else if (!onlyActive) {
                use(callbackHandle, entry->bnd);
                called = true;
            }
            fw_bundleEntry_decreaseUseCount(entry);
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

    long bndId = celix_bundle_getId(bnd);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);


    if (serviceName != NULL && factory != NULL) {
        status = celix_serviceRegistry_registerServiceFactory(fw->registry, bnd, serviceName, factory, properties, 0, &reg);
    } else if (serviceName != NULL) {
        status = celix_serviceRegistry_registerService(fw->registry, bnd, serviceName, svc, properties, 0, &reg);
    } else {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Invalid arguments serviceName", serviceName);
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    fw_bundleEntry_decreaseUseCount(entry);

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
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);

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
    celix_framework_addToEventQueue(fw, &event);

    return svcId;
}

void celix_framework_unregisterAsync(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId, void *doneData, void (*doneCallback)(void*)) {
    long bndId = celix_bundle_getId(bnd);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);

    celix_framework_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = CELIX_UNREGISTER_SERVICE_EVENT;
    event.bndEntry = entry;
    event.unregisterServiceId = serviceId;
    event.doneData = doneData;
    event.doneCallback = doneCallback;

    celix_framework_addToEventQueue(fw, &event);
}

void celix_framework_unregister(celix_framework_t* fw, celix_bundle_t* bnd, long serviceId) {
    celix_serviceRegistry_unregisterService(fw->registry, bnd, serviceId);
}

void celix_framework_waitForAsyncRegistration(framework_t *fw, long svcId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool registrationsInProgress = true;
    while (registrationsInProgress) {
        registrationsInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
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
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
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
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
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
        return celix_properties_get(fw->configurationMap, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
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
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, id);
    if (entry != NULL) {
        bnd = entry->bnd;
        fw_bundleEntry_decreaseUseCount(entry); //NOTE returning bundle without increased use count -> FIXME make all getBundle api private (use bundle id instead)
    }
    return bnd;
}


bool celix_framework_isBundleInstalled(celix_framework_t *fw, long bndId) {
    bool isInstalled = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        isInstalled = true;
        fw_bundleEntry_decreaseUseCount(entry);
    }
    return isInstalled;
}

bool celix_framework_isBundleActive(celix_framework_t *fw, long bndId) {
    bool isActive = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        isActive = celix_bundle_getState(entry->bnd) == OSGI_FRAMEWORK_BUNDLE_ACTIVE;
        fw_bundleEntry_decreaseUseCount(entry);
    }
    return isActive;
}

static void celix_framework_waitForBundleEvents(celix_framework_t *fw, long bndId) {
    if (bndId >= 0 && !celix_framework_isCurrentThreadTheEventLoop(fw)) {
        celix_framework_waitUntilNoEventsForBnd(fw, bndId);
    }
}

long celix_framework_installBundle(celix_framework_t *fw, const char *bundleLoc, bool autoStart) {
    long bundleId = -1;
    bundle_t *bnd = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (fw_installBundle(fw, &bnd, bundleLoc, NULL) == CELIX_SUCCESS) {
        status = bundle_getBundleId(bnd, &bundleId);
        if (status == CELIX_SUCCESS && autoStart) {
            status = bundle_start(bnd);

        }
    }

    celix_framework_waitForBundleEvents(fw, bundleId);
    framework_logIfError(fw->logger, status, NULL, "Failed to install bundle '%s'", bundleLoc);

    return bundleId;
}

bool celix_framework_uninstallBundle(celix_framework_t *fw, long bndId) {
    bool uninstalled = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        celix_bundle_state_e bndState = celix_bundle_getState(entry->bnd);
        if (bndState == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            fw_stopBundle(fw, bndId, false);
        }

        celix_framework_bundle_entry_t* removedEntry = fw_bundleEntry_removeBundleEntryAndIncreaseUseCount(fw, bndId);
        fw_bundleEntry_decreaseUseCount(entry);
        if (removedEntry != NULL) {
            celix_status_t status = fw_uninstallBundleEntry(fw, removedEntry); //also decreases use count
            uninstalled = status == CELIX_SUCCESS;
        }
    }
    celix_framework_waitForBundleEvents(fw, bndId);
    return uninstalled;
}

bool celix_framework_stopBundle(celix_framework_t *fw, long bndId) {
    bool stopped = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        celix_bundle_state_e state = celix_bundle_getState(entry->bnd);
        if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            celix_status_t rc = bundle_stop(entry->bnd);
            stopped = rc == CELIX_SUCCESS;
        } else {
            fw_log(fw->logger, CELIX_LOG_LEVEL_WARNING, "Cannot stop bundle state is %i", state);
        }
        fw_bundleEntry_decreaseUseCount(entry);
    }
    celix_framework_waitForBundleEvents(fw, bndId);
    return stopped;
}

bool celix_framework_startBundle(celix_framework_t *fw, long bndId) {
    bool started = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        celix_bundle_state_e state = celix_bundle_getState(entry->bnd);
        if (state == OSGI_FRAMEWORK_BUNDLE_INSTALLED || state == OSGI_FRAMEWORK_BUNDLE_RESOLVED) {
            celix_status_t rc = bundle_start(entry->bnd);
            started = rc == CELIX_SUCCESS;
        }
        fw_bundleEntry_decreaseUseCount(entry);
    }
    celix_framework_waitForBundleEvents(fw, bndId);
    return started;
}

void celix_framework_waitForEmptyEventQueue(celix_framework_t *fw) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    while (fw->dispatcher.eventQueueSize > 0 || celix_arrayList_size(fw->dispatcher.dynamicEventQueue) > 0) {
        celixThreadCondition_wait(&fw->dispatcher.cond, &fw->dispatcher.mutex);
    }
    celixThreadMutex_unlock(&fw->dispatcher.mutex);
}

void celix_framework_waitUntilNoEventsForBnd(celix_framework_t* fw, long bndId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool eventInProgress = true;
    while (eventInProgress) {
        eventInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if (e->bndEntry != NULL && e->bndEntry->bndId == bndId) {
                eventInProgress = true;
                break;
            }
        }
        for (int i = 0; !eventInProgress && i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if (e->bndEntry != NULL && e->bndEntry->bndId == bndId) {
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


void celix_framework_setLogCallback(celix_framework_t* fw, void* logHandle, void (*logFunction)(void* handle, celix_log_level_e level, const char* file, const char *function, int line, const char *format, va_list formatArgs)) {
    celix_frameworkLogger_setLogCallback(fw->logger, logHandle, logFunction);
}

long celix_framework_fireGenericEvent(framework_t* fw, long eventId, long bndId, const char *eventName, void* processData, void (*processCallback)(void *data), void* doneData, void (*doneCallback)(void* doneData)) {
    celix_framework_bundle_entry_t* bndEntry = NULL;
    if (bndId >=0) {
        bndEntry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
        if (bndEntry == NULL) {
            fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Cannot find bundle for id %li", bndId);
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
    celix_framework_addToEventQueue(fw, &event);

    return eventId;
}

long celix_framework_nextEventId(framework_t *fw) {
    return __atomic_fetch_add(&fw->nextGenericEventId, 1, __ATOMIC_RELAXED);
}

void celix_framework_waitForGenericEvent(framework_t *fw, long eventId) {
    assert(!celix_framework_isCurrentThreadTheEventLoop(fw));

    celixThreadMutex_lock(&fw->dispatcher.mutex);
    bool eventInProgress = true;
    while (eventInProgress) {
        eventInProgress = false;
        for (int i = 0; i < fw->dispatcher.eventQueueSize; ++i) {
            int index = (fw->dispatcher.eventQueueFirstEntry + i) % CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;
            celix_framework_event_t* e = &fw->dispatcher.eventQueue[index];
            if (e->type == CELIX_GENERIC_EVENT && e->genericEventId == eventId) {
                eventInProgress = true;
                break;
            }
        }
        for (int i = 0; !eventInProgress && i < celix_arrayList_size(fw->dispatcher.dynamicEventQueue); ++i) {
            celix_framework_event_t* e = celix_arrayList_get(fw->dispatcher.dynamicEventQueue, i);
            if (e->type == CELIX_GENERIC_EVENT && e->genericEventId == eventId) {
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