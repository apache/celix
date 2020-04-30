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

typedef struct celix_framework_bundle_entry {
    celix_bundle_t *bnd;
    long bndId;

    celix_thread_mutex_t useMutex; //protects useCount
    celix_thread_cond_t useCond;
    size_t useCount;
} celix_framework_bundle_entry_t;


static inline celix_framework_bundle_entry_t* fw_bundleEntry_create(celix_bundle_t *bnd) {
    celix_framework_bundle_entry_t *entry = calloc(1, sizeof(*entry));
    entry->bnd = bnd;

    entry->bndId = celix_bundle_getId(bnd);
    entry->useCount = 0;
    celixThreadMutex_create(&entry->useMutex, NULL);
    celixThreadCondition_init(&entry->useCond, NULL);
    return entry;
}


static inline void fw_bundleEntry_waitTillNotUsed(celix_framework_bundle_entry_t *entry) {
    celixThreadMutex_lock(&entry->useMutex);
    while (entry->useCount != 0) {
        celixThreadCondition_wait(&entry->useCond, &entry->useMutex);
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
    //pre condition mutex is taken on  fw->installedBundles.mutex
    assert(entry != NULL);
    celixThreadMutex_lock(&entry->useMutex);
    ++entry->useCount;
    celixThreadMutex_unlock(&entry->useMutex);
}

static inline void fw_bundleEntry_decreaseUseCount(celix_framework_bundle_entry_t *entry) {
    //pre condition mutex is taken on  fw->installedBundles.mutex
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

celix_status_t framework_setBundleStateAndNotify(framework_pt framework, bundle_pt bundle, int state);
celix_status_t framework_markBundleResolved(framework_pt framework, module_pt module);

long framework_getNextBundleId(framework_pt framework);

celix_status_t fw_installBundle2(framework_pt framework, bundle_pt * bundle, long id, const char * location, const char *inputFile, bundle_archive_pt archive);

celix_status_t fw_refreshBundles(framework_pt framework, long bundleIds[], int size);
celix_status_t fw_refreshBundle(framework_pt framework, long bndId);

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map);

celix_status_t fw_fireBundleEvent(framework_pt framework, bundle_event_type_e, bundle_pt bundle);
celix_status_t fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, bundle_pt bundle, celix_status_t errorCode);
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
};

typedef struct fw_bundleListener * fw_bundle_listener_pt;

struct fw_frameworkListener {
	bundle_pt bundle;
	framework_listener_pt listener;
};

typedef struct fw_frameworkListener * fw_framework_listener_pt;

enum event_type {
	FRAMEWORK_EVENT_TYPE,
	BUNDLE_EVENT_TYPE,
	EVENT_TYPE_SERVICE,
};

typedef enum event_type event_type_e;

struct request {
	event_type_e type;
	array_list_pt listeners;

	int eventType;
	long bundleId;
	char* bundleSymbolicName;
	celix_status_t errorCode;
	char *error;

	char *filter;
};

typedef struct request *request_pt;

framework_logger_pt logger;

static celix_thread_once_t loggerInit = CELIX_THREAD_ONCE_INIT;
static void framework_loggerInit(void) {
    logger = malloc(sizeof(*logger));
    logger->logFunction = frameworkLogger_log;
}

celix_status_t framework_create(framework_pt *framework, properties_pt config) {
    celix_status_t status = CELIX_SUCCESS;

    logger = hashMap_get(config, "logger");
    if (logger == NULL) {
        celixThread_once(&loggerInit, framework_loggerInit);
    }

    *framework = (framework_pt) malloc(sizeof(**framework));
    if (*framework != NULL) {
        celix_thread_mutexattr_t attr;
        celixThreadMutexAttr_create(&attr);
        celixThreadMutexAttr_settype(&attr, CELIX_THREAD_MUTEX_RECURSIVE);

        status = CELIX_DO_IF(status, celixThreadCondition_init(&(*framework)->shutdown.cond, NULL));
        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*framework)->shutdown.mutex, NULL));
        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*framework)->dispatcher.mutex, NULL));
        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*framework)->frameworkListenersLock, &attr));
        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*framework)->bundleListenerLock, NULL));
        status = CELIX_DO_IF(status, celixThreadMutex_create(&(*framework)->installedBundles.mutex, NULL));
        status = CELIX_DO_IF(status, celixThreadCondition_init(&(*framework)->dispatcher.cond, NULL));
        if (status == CELIX_SUCCESS) {
            (*framework)->bundle = NULL;
            (*framework)->registry = NULL;
            (*framework)->shutdown.done = false;
            (*framework)->shutdown.initialized = false;
            (*framework)->dispatcher.active = true;
            (*framework)->nextBundleId = 1L; //system bundle is 0
            (*framework)->cache = NULL;
            (*framework)->installRequestMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
            (*framework)->installedBundles.entries = celix_arrayList_create();
            (*framework)->bundleListeners = NULL;
            (*framework)->frameworkListeners = NULL;
            (*framework)->dispatcher.requests = NULL;
            (*framework)->configurationMap = config;
            (*framework)->logger = logger;


            status = CELIX_DO_IF(status, bundle_create(&(*framework)->bundle));
            status = CELIX_DO_IF(status, bundle_getBundleId((*framework)->bundle, &(*framework)->bundleId));
            status = CELIX_DO_IF(status, bundle_setFramework((*framework)->bundle, (*framework)));
            if (status == CELIX_SUCCESS) {
                //nop
            } else {
                status = CELIX_FRAMEWORK_EXCEPTION;
                fw_logCode((*framework)->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not create framework");
            }
        } else {
            status = CELIX_FRAMEWORK_EXCEPTION;
            fw_logCode((*framework)->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not create framework");
        }
    } else {
        status = CELIX_FRAMEWORK_EXCEPTION;
        fw_logCode(logger, OSGI_FRAMEWORK_LOG_ERROR, CELIX_ENOMEM, "Could not create framework");
    }

    return status;
}

celix_status_t framework_destroy(framework_pt framework) {
    celix_status_t status = CELIX_SUCCESS;

    //Note the shutdown thread can not be joined on the framework_shutdown (which is normally more logical),
    //because a shutdown can be initiated from a bundle.
    //A bundle cannot be stopped when it is waiting for a framework shutdown -> hence a shutdown thread which
    //has not been joined yet.
    celixThread_join(framework->shutdown.thread, NULL);

    serviceRegistry_destroy(framework->registry);

    celixThreadMutex_lock(&framework->installedBundles.mutex);
    for (int i = 0; i < celix_arrayList_size(framework->installedBundles.entries); ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(framework->installedBundles.entries, i);
        celixThreadMutex_lock(&entry->useMutex);
        size_t count = entry->useCount;
        celixThreadMutex_unlock(&entry->useMutex);
        bundle_t *bnd = entry->bnd;
        if (count > 0) {
            fw_log(framework->logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot destroy framework (yet), a bundle use count is not 0 (%ui)", count);
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
    celix_arrayList_destroy(framework->installedBundles.entries);
    celixThreadMutex_destroy(&framework->installedBundles.mutex);

	hashMap_destroy(framework->installRequestMap, false, false);

    if (framework->bundleListeners) {
        arrayList_destroy(framework->bundleListeners);
    }
    if (framework->frameworkListeners) {
        arrayList_destroy(framework->frameworkListeners);
    }

	if(framework->dispatcher.requests){
	    int i;
	    for (i = 0; i < arrayList_size(framework->dispatcher.requests); i++) {
	        request_pt request = arrayList_get(framework->dispatcher.requests, i);
	        free(request->bundleSymbolicName);
	        free(request);
	    }
	    arrayList_destroy(framework->dispatcher.requests);
	}

	bundleCache_destroy(&framework->cache);

	celixThreadCondition_destroy(&framework->dispatcher.cond);
    celixThreadMutex_destroy(&framework->frameworkListenersLock);
	celixThreadMutex_destroy(&framework->bundleListenerLock);
	celixThreadMutex_destroy(&framework->dispatcher.mutex);
	celixThreadMutex_destroy(&framework->shutdown.mutex);
	celixThreadCondition_destroy(&framework->shutdown.cond);

    logger = hashMap_get(framework->configurationMap, "logger");
    if (logger == NULL) {
        free(framework->logger);
    }

    properties_destroy(framework->configurationMap);

    free(framework);

	return status;
}

celix_status_t fw_init(framework_pt framework) {
	bundle_state_e state = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
	const char *location = NULL;
	module_pt module = NULL;
	linked_list_pt wires = NULL;
	array_list_pt archives = NULL;
	bundle_archive_pt archive = NULL;

    /*create and store framework uuid*/
    char uuid[37];

    uuid_t uid;
    uuid_generate(uid);
    uuid_unparse(uid, uuid);

    properties_set(framework->configurationMap, (char*) OSGI_FRAMEWORK_FRAMEWORK_UUID, uuid);

	celix_status_t status = CELIX_SUCCESS;
	status = CELIX_DO_IF(status, arrayList_create(&framework->bundleListeners));
	status = CELIX_DO_IF(status, arrayList_create(&framework->frameworkListeners));
	status = CELIX_DO_IF(status, arrayList_create(&framework->dispatcher.requests));
	status = CELIX_DO_IF(status, celixThread_create(&framework->dispatcher.thread, NULL, fw_eventDispatcher, framework));
	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	if (status == CELIX_SUCCESS) {
	    if ((state == OSGI_FRAMEWORK_BUNDLE_INSTALLED) || (state == OSGI_FRAMEWORK_BUNDLE_RESOLVED)) {
	        status = CELIX_DO_IF(status, bundleCache_create(uuid, framework->configurationMap,&framework->cache));
	        status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	        if (status == CELIX_SUCCESS) {
	            if (state == OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
	                const char *clean = properties_get(framework->configurationMap, OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME);
	                bool cleanCache = clean == NULL ? true : strcmp(clean, "false") != 0;
	                if (cleanCache) {
	                    bundleCache_delete(framework->cache);
	                }
	            }
            }
        }
	}

    status = CELIX_DO_IF(status, bundle_getArchive(framework->bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getLocation(archive, &location));
    if (status == CELIX_SUCCESS) {
        long bndId = -1L;
        bundle_getBundleId(framework->bundle, &bndId);
        celix_framework_bundle_entry_t *entry = fw_bundleEntry_create(framework->bundle);
        celixThreadMutex_lock(&framework->installedBundles.mutex);
        celix_arrayList_add(framework->installedBundles.entries, entry);
        celixThreadMutex_unlock(&framework->installedBundles.mutex);
    }
    status = CELIX_DO_IF(status, bundle_getCurrentModule(framework->bundle, &module));
    if (status == CELIX_SUCCESS) {
        wires = resolver_resolve(module);
        if (wires != NULL) {
            framework_markResolvedModules(framework, wires);
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Unresolved constraints in System Bundle");
        }
    }

    status = CELIX_DO_IF(status, bundleCache_getArchives(framework->cache, &archives));
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

    status = CELIX_DO_IF(status, serviceRegistry_create(framework, &framework->registry));
    status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, framework->bundle, OSGI_FRAMEWORK_BUNDLE_STARTING));

    bundle_context_t *context = NULL;
    status = CELIX_DO_IF(status, bundleContext_create(framework, framework->logger, framework->bundle, &context));
    status = CELIX_DO_IF(status, bundle_setContext(framework->bundle, context));
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
                /* This code part is in principle dead, but in future it may do something.
                 * That's why it's outcommented and not deleted
		        if (create != NULL) {
                    create(context, &userData);
                }
                */
                activator->userData = userData;

                if (start != NULL) {
                    start(userData, validateContext);
                }
            }
            else{
            	free(activator);
            }
        } else {
            status = CELIX_ENOMEM;
        }
    }

    if (status != CELIX_SUCCESS) {
       fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not init framework");
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
	if (status == CELIX_SUCCESS) {
	    if (state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
	        status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, framework->bundle, OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	    }
	}

	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, framework->bundle));
	status = CELIX_DO_IF(status, fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_STARTED, framework->bundle, framework->bundleId));

	if (status != CELIX_SUCCESS) {
       status = CELIX_BUNDLE_EXCEPTION;
       fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start framework");
       fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_ERROR, framework->bundle, status);
    }

    bundle_context_t *fwCtx = framework_getContext(framework);
	if (fwCtx != NULL) {
        framework_autoStartConfiguredBundles(fwCtx);
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
    for (int i = 0; i < len; ++i) {
        framework_autoStartConfiguredBundlesForList(fwCtx, installedBundles);
    }
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
	return fw_stopBundle(framework, framework->bundleId, true);
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
        fw_log(framework->logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot find bundle %s. Using %s=%s", bndLoc, CELIX_BUNDLES_PATH_NAME, paths);
        free(location);
        return CELIX_FILE_IO_EXCEPTION;
    }

    //increase use count of framework bundle to prevent a stop.
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(framework, framework->bundleId);

  	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
  	if (status == CELIX_SUCCESS) {
        if (state == OSGI_FRAMEWORK_BUNDLE_STOPPING || state == OSGI_FRAMEWORK_BUNDLE_UNINSTALLED) {
            fw_log(framework->logger, OSGI_FRAMEWORK_LOG_INFO,  "The framework is being shutdown");
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
            celixThreadMutex_lock(&framework->installedBundles.mutex);
            celix_arrayList_add(framework->installedBundles.entries, bEntry);
            celixThreadMutex_unlock(&framework->installedBundles.mutex);

        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            status = CELIX_DO_IF(status, bundleArchive_closeAndDelete(archive));
        }
    }

    fw_bundleEntry_decreaseUseCount(entry);

    if (status != CELIX_SUCCESS) {
    	fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not install bundle");
    } else {
        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED, *bundle));
    }

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

                        status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_STARTING));
                        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING, entry->bnd));

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

                        status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_ACTIVE));
                        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, entry->bnd));

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
                            framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
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
	        fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
	    } else {
	        fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
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
	status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED));

	bundle_revision_pt revision = NULL;
	array_list_pt handles = NULL;
	status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getHandles(revision, &handles));
    if (handles != NULL) {
        int i;
	    for (i = arrayList_size(handles) - 1; i >= 0; i--) {
	        void* handle = arrayList_get(handles, i);
	        celix_libloader_close(handle);
	    }
    }


	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bundle));
	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UPDATED, bundle));

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
            fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot update bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot update bundle: %s [%ld]", symbolicName, id);
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
      fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot stop bundle [%ld]: cannot find bundle entry", bndId);
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


	status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_STOPPING));
	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING, entry->bnd));
	if (status == CELIX_SUCCESS) {
	    if (wasActive || (bndId == 0)) {
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
	            celix_serviceTracker_syncForContext(entry->bnd->context);
                status = CELIX_DO_IF(status, serviceRegistry_clearServiceRegistrations(framework->registry, entry->bnd));
                if (status == CELIX_SUCCESS) {
                    module_pt module = NULL;
                    const char *symbolicName = NULL;
                    long id = 0;
                    bundle_getCurrentModule(entry->bnd, &module);
                    module_getSymbolicName(module, &symbolicName);
                    bundle_getBundleId(entry->bnd, &id);

                    serviceRegistry_clearReferencesFor(framework->registry, entry->bnd);
                }

                if (context != NULL) {
                    status = CELIX_DO_IF(status, bundleContext_destroy(context));
                    status = CELIX_DO_IF(status, bundle_setContext(entry->bnd, NULL));
                }

                status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
            } else if (bndId == 0) {
                //framework bundle
                celix_serviceTracker_syncForContext(framework->bundle->context);
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
            fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot stop bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot stop bundle: %s [%ld]", symbolicName, id);
        }
 	} else {
        fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, entry->bnd);
 	}

	fw_bundleEntry_decreaseUseCount(entry);
    celix_serviceTracker_syncForFramework(framework);

	return status;
}

static celix_status_t fw_uninstallBundleEntry(celix_framework_t *framework, celix_framework_bundle_entry_t *entry) {
    celix_status_t status = CELIX_SUCCESS;

    celix_bundle_t *bnd = NULL;
    long bndId = -1L;

    if (entry != NULL) {
        //NOTE wait outside installedBundles.mutex
        bnd = entry->bnd;
        bndId = entry->bndId;
        fw_bundleEntry_decreaseUseCount(entry);
        fw_bundleEntry_destroy(entry, true); //wait till use count is 0 -> e.g. not used
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
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

        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bnd));

        status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bnd, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED));
        status = CELIX_DO_IF(status, bundleArchive_setLastModified(archive, time(NULL)));

        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED, bnd));

        if (status == CELIX_SUCCESS) {
            celix_status_t refreshStatus = fw_refreshBundle(framework, bndId);
            if (refreshStatus != CELIX_SUCCESS) {
                printf("Could not refresh bundle");
            } else {
                bundleArchive_destroy(archive);
                status = CELIX_DO_IF(status, bundle_destroy(bnd));
            }
        }
    }


    if (status != CELIX_SUCCESS) {
//        module_pt module = NULL;
//        char *symbolicName = NULL;
//        long id = 0;
//        bundle_getCurrentModule(bundle, &module);
//        module_getSymbolicName(module, &symbolicName);
//        bundle_getBundleId(bundle, &id);

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
            framework_setBundleStateAndNotify(framework, entry->bnd, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
            fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, entry->bnd);
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
    celix_status_t status = CELIX_SUCCESS;
    fw_bundle_listener_pt bundleListener = NULL;

    bundleListener = (fw_bundle_listener_pt) malloc(sizeof(*bundleListener));
    if (!bundleListener) {
        status = CELIX_ENOMEM;
    } else {
        bundleListener->listener = listener;
        bundleListener->bundle = bundle;

        if (celixThreadMutex_lock(&framework->bundleListenerLock) != CELIX_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
        } else {
            arrayList_add(framework->bundleListeners, bundleListener);

            if (celixThreadMutex_unlock(&framework->bundleListenerLock)) {
                status = CELIX_FRAMEWORK_EXCEPTION;
            }
        }
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to add bundle listener");

    return status;
}

celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i;
    fw_bundle_listener_pt bundleListener;

    if (celixThreadMutex_lock(&framework->bundleListenerLock) != CELIX_SUCCESS) {
        status = CELIX_FRAMEWORK_EXCEPTION;
    }
    else {
        for (i = 0; i < arrayList_size(framework->bundleListeners); i++) {
            bundleListener = (fw_bundle_listener_pt) arrayList_get(framework->bundleListeners, i);
            if (bundleListener->listener == listener && bundleListener->bundle == bundle) {
                arrayList_remove(framework->bundleListeners, i);

                bundleListener->bundle = NULL;
                bundleListener->listener = NULL;
                free(bundleListener);
            }
        }
        if (celixThreadMutex_unlock(&framework->bundleListenerLock)) {
            status = CELIX_FRAMEWORK_EXCEPTION;
        }
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to remove bundle listener");

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

            status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
            status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_RESOLVED, bundle));
        }

        if (status != CELIX_SUCCESS) {
            const char *symbolicName = NULL;
            long id = 0;
            module_getSymbolicName(module, &symbolicName);
            bundle_getBundleId(bundle, &id);
            if (error != NULL) {
                fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
            } else {
                fw_logCode(framework->logger, OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
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

celix_status_t framework_setBundleStateAndNotify(framework_pt framework, bundle_pt bundle, int state) {
    int ret = CELIX_SUCCESS;
    bundle_setState(bundle, state);
    return ret;
}

celix_status_t framework_waitForStop(framework_pt framework) {
    celixThreadMutex_lock(&framework->shutdown.mutex);
    while (!framework->shutdown.done) {
        celixThreadCondition_wait(&framework->shutdown.cond, &framework->shutdown.mutex);
    }
    celixThreadMutex_unlock(&framework->shutdown.mutex);

    fw_log(framework->logger, OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Successful shutdown");
    return CELIX_SUCCESS;
}

static void* framework_shutdown(void *framework) {
    framework_pt fw = (framework_pt) framework;

    fw_log(fw->logger, OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Shutdown");

    //celix_framework_bundle_entry_t *fwEntry = NULL;
    celix_array_list_t *stopEntries = celix_arrayList_create();
    celix_framework_bundle_entry_t *fwEntry = NULL;
    celixThreadMutex_lock(&fw->installedBundles.mutex);
    int size = celix_arrayList_size(fw->installedBundles.entries);
    for (int i = 0; i < size; ++i) {
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(fw->installedBundles.entries, i);
        if (entry->bndId != 0) { //i.e. not framework bundle
            celix_arrayList_add(stopEntries, entry);
        } else {
            fwEntry = entry;
        }
    }
//	celix_arrayList_clear(fw->installedBundles.entries);
    celixThreadMutex_unlock(&fw->installedBundles.mutex);


    size = celix_arrayList_size(stopEntries);
    for (int i = size-1; i >= 0; --i) { //note loop in reverse order -> stop later installed bundle first
        celix_framework_bundle_entry_t *entry = celix_arrayList_get(stopEntries, i);

        //wait until entry use counts is 0
        bundle_t *bnd = entry->bnd;
        fw_bundleEntry_waitTillNotUsed(entry);

        bundle_state_e state;
        bundle_getState(bnd, &state);
        if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE || state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
            fw_stopBundle(fw, entry->bndId, 0);
        }
        bundle_close(bnd);
    }
    celix_arrayList_destroy(stopEntries);


    // 'stop' framework bundle
    if (fwEntry != NULL) {
        bundle_t *bnd = fwEntry->bnd;
        fw_bundleEntry_waitTillNotUsed(fwEntry);

        bundle_state_e state;
        bundle_getState(bnd, &state);
        if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE || state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
            fw_stopBundle(fw, fwEntry->bndId, 0);
        }
        bundle_close(bnd);
    }

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

celix_status_t fw_fireBundleEvent(framework_pt framework, bundle_event_type_e eventType, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    if ((eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING)
        && (eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING)
        && (eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_LAZY_ACTIVATION)) {
        request_pt request = (request_pt) calloc(1, sizeof(*request));
        if (!request) {
            status = CELIX_ENOMEM;
        } else {
            bundle_archive_pt archive = NULL;
            module_pt module = NULL;

            request->eventType = eventType;
            request->filter = NULL;
            request->listeners = framework->bundleListeners;
            request->type = BUNDLE_EVENT_TYPE;
            request->error = NULL;
            request->bundleId = -1;

            status = bundle_getArchive(bundle, &archive);

            if (status == CELIX_SUCCESS) {
                long bundleId;

                status = bundleArchive_getId(archive, &bundleId);

                if (status == CELIX_SUCCESS) {
                    request->bundleId = bundleId;
                }
            }

            if (status == CELIX_SUCCESS) {
                status = bundle_getCurrentModule(bundle, &module);

                if (status == CELIX_SUCCESS) {
                    const char *symbolicName = NULL;
                    status = module_getSymbolicName(module, &symbolicName);
                    if (status == CELIX_SUCCESS) {
                        request->bundleSymbolicName = strdup(symbolicName);
                    }
                }
            }

            celixThreadMutex_lock(&framework->dispatcher.mutex);
            arrayList_add(framework->dispatcher.requests, request);
            celixThreadCondition_broadcast(&framework->dispatcher.cond);
            celixThreadMutex_unlock(&framework->dispatcher.mutex);

        }
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to fire bundle event");

    return status;
}

celix_status_t fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, bundle_pt bundle, celix_status_t errorCode) {
    celix_status_t status = CELIX_SUCCESS;

    request_pt request = (request_pt) malloc(sizeof(*request));
    if (!request) {
        status = CELIX_ENOMEM;
    } else {
        bundle_archive_pt archive = NULL;
        module_pt module = NULL;

        request->eventType = eventType;
        request->filter = NULL;
        request->listeners = framework->frameworkListeners;
        request->type = FRAMEWORK_EVENT_TYPE;
        request->errorCode = errorCode;
        request->error = "";
        request->bundleId = -1;

        status = bundle_getArchive(bundle, &archive);

        if (status == CELIX_SUCCESS) {
            long bundleId;

            status = bundleArchive_getId(archive, &bundleId);

            if (status == CELIX_SUCCESS) {
                request->bundleId = bundleId;
            }
        }

        if (status == CELIX_SUCCESS) {
            status = bundle_getCurrentModule(bundle, &module);

            if (status == CELIX_SUCCESS) {
                const char *symbolicName = NULL;
                status = module_getSymbolicName(module, &symbolicName);
                if (status == CELIX_SUCCESS) {
                    request->bundleSymbolicName = strdup(symbolicName);
                }
            }
        }

        if (errorCode != CELIX_SUCCESS) {
            char message[256];
            celix_strerror(errorCode, message, 256);
            request->error = message;
        }

        celixThreadMutex_lock(&framework->dispatcher.mutex);
        arrayList_add(framework->dispatcher.requests, request);
        celixThreadCondition_broadcast(&framework->dispatcher.cond);
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }

    framework_logIfError(framework->logger, status, NULL, "Failed to fire framework event");

    return status;
}

static void *fw_eventDispatcher(void *fw) {
    framework_pt framework = (framework_pt) fw;

    celixThreadMutex_lock(&framework->dispatcher.mutex);
    bool active = framework->dispatcher.active;
    celixThreadMutex_unlock(&framework->dispatcher.mutex);

    celix_array_list_t *localRequests = celix_arrayList_create();

    while (active) {
        celixThreadMutex_lock(&framework->dispatcher.mutex);
        if (celix_arrayList_size(framework->dispatcher.requests) == 0) {
            celixThreadCondition_wait(&framework->dispatcher.cond, &framework->dispatcher.mutex);
        }
        for (int i = 0; i < celix_arrayList_size(framework->dispatcher.requests); ++i) {
            void *r = celix_arrayList_get(framework->dispatcher.requests, i);
            celix_arrayList_add(localRequests, r);
        }
        celix_arrayList_clear(framework->dispatcher.requests);
        celixThreadMutex_unlock(&framework->dispatcher.mutex);

        //FIXME strange locking  of bundleListenerLock and frameworkListenerLock inside a loop.
        //Seems like a request depends on bundle listeners or framework listeners -> this should be protected with
        //a use count!!
        for (int i = 0; i < celix_arrayList_size(localRequests); ++i) {
            request_pt request = celix_arrayList_get(localRequests, i);
            int size = arrayList_size(request->listeners);
            for (int k = 0; k < size; k++) {
                if (request->type == BUNDLE_EVENT_TYPE) {
                    celixThreadMutex_lock(&framework->bundleListenerLock);
                    fw_bundle_listener_pt listener = (fw_bundle_listener_pt) arrayList_get(request->listeners, k);
                    bundle_event_t event;
                    memset(&event, 0, sizeof(event));
                    event.bundleId = request->bundleId;
                    event.bundleSymbolicName = request->bundleSymbolicName;
                    event.type = request->eventType;

                    fw_invokeBundleListener(framework, listener->listener, &event, listener->bundle);
                    celixThreadMutex_unlock(&framework->bundleListenerLock);
                } else if (request->type == FRAMEWORK_EVENT_TYPE) {
                    celixThreadMutex_lock(&framework->frameworkListenersLock);
                    fw_framework_listener_pt listener = (fw_framework_listener_pt) arrayList_get(request->listeners, k);
                    framework_event_t event;
                    memset(&event, 0, sizeof(event));
                    event.bundleId = request->bundleId;
                    event.bundleSymbolicName = request->bundleSymbolicName;
                    event.type = request->eventType;
                    event.error = request->error;
                    event.errorCode = request->errorCode;

                    fw_invokeFrameworkListener(framework, listener->listener, &event, listener->bundle);
                    celixThreadMutex_unlock(&framework->frameworkListenersLock);
                }
            }
            free(request->bundleSymbolicName);
            //NOTE next 2 free calls not needed? why it is a char* not a const char*
            //free(request->filter);
            //free(request->error);
            free(request);
        }
        celix_arrayList_clear(localRequests);

        celixThreadMutex_lock(&framework->dispatcher.mutex);
        active = framework->dispatcher.active;
        celixThreadMutex_unlock(&framework->dispatcher.mutex);
    }

    celix_arrayList_destroy(localRequests);
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

        fw_log(framework->logger, OSGI_FRAMEWORK_LOG_DEBUG, "FRAMEWORK: Start shutdownthread");

        celixThreadMutex_lock(&framework->shutdown.mutex);
        bool alreadyInitialized = framework->shutdown.initialized;
        framework->shutdown.initialized = true;
        celixThreadMutex_unlock(&framework->shutdown.mutex);

        if (!alreadyInitialized) {
            celixThreadMutex_lock(&framework->dispatcher.mutex);
            framework->dispatcher.active = false;
            celixThreadCondition_broadcast(&framework->dispatcher.cond);
            celixThreadMutex_unlock(&framework->dispatcher.mutex);
            celixThread_join(framework->dispatcher.thread, NULL);

            celixThread_create(&framework->shutdown.thread, NULL, &framework_shutdown, framework);
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

service_registration_t* celix_framework_registerServiceFactory(framework_t *fw , const celix_bundle_t *bnd, const char* serviceName, celix_service_factory_t *factory, celix_properties_t *properties) {
    const char *error = NULL;
    celix_status_t status = CELIX_SUCCESS;
    service_registration_t *reg = NULL;

    long bndId = celix_bundle_getId(bnd);
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);


    if (serviceName != NULL && factory != NULL) {
        status = CELIX_DO_IF(status, celix_serviceRegistry_registerServiceFactory(fw->registry, bnd, serviceName, factory, properties, &reg));
    }

    fw_bundleEntry_decreaseUseCount(entry);

    framework_logIfError(fw->logger, status, error, "Cannot register service factory: %s", serviceName);

    return reg;
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

long celix_framework_installBundle(celix_framework_t *fw, const char *bundleLoc, bool autoStart) {
    long bundleId = -1;
    bundle_t *bnd = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (fw_installBundle(fw, &bnd, bundleLoc, NULL) == CELIX_SUCCESS) {
        status = bundle_getBundleId(bnd, &bundleId); //TODO FIXME race condition with fw_installBundle, bundle can be uninstalled (no use count increase)
        if (status == CELIX_SUCCESS && autoStart) {
            status = bundle_start(bnd);
        }
    }

    framework_logIfError(logger, status, NULL, "Failed to install bundle '%s'", bundleLoc);

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
        fw_bundleEntry_decreaseUseCount(entry);
    }

    entry = fw_bundleEntry_removeBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        celix_bundle_state_e bndState = celix_bundle_getState(entry->bnd);
        if (bndState == OSGI_FRAMEWORK_BUNDLE_RESOLVED || bndState == OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
            celix_status_t status = fw_uninstallBundleEntry(fw, entry); //also decreases use count
            uninstalled = status == CELIX_SUCCESS;
        } else {
            fw_bundleEntry_decreaseUseCount(entry);
        }
    }
    return uninstalled;
}

bool celix_framework_stopBundle(celix_framework_t *fw, long bndId) {
    bool stopped = false;
    celix_framework_bundle_entry_t *entry = fw_bundleEntry_getBundleEntryAndIncreaseUseCount(fw, bndId);
    if (entry != NULL) {
        if (celix_bundle_getState(entry->bnd) == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            celix_status_t rc = bundle_stop(entry->bnd);
            stopped = rc == CELIX_SUCCESS;
        }
        fw_bundleEntry_decreaseUseCount(entry);
    }
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
    return started;
}
