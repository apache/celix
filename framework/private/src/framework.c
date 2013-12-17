/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * framework.c
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "celixbool.h"
#include <math.h>
#include <apr_file_io.h>
#include <apr_general.h>
#include <apr_strings.h>
#include <apr_thread_mutex.h>
#include <apr_thread_proc.h>
#include <apr_uuid.h>
#ifdef _WIN32
#include <winbase.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif


#include "framework_private.h"
#include "filter.h"
#include "constants.h"
#include "archive.h"
#include "bundle.h"
#include "wire.h"
#include "resolver.h"
#include "utils.h"
#include "bundle_activator.h"
#include "service_registry.h"
#include "bundle_cache.h"
#include "bundle_archive.h"
#include "bundle_revision.h"
#include "bundle_context.h"
#include "linked_list_iterator.h"
#include "service_reference.h"
#include "listener_hook_service.h"
#include "service_registration.h"
#include "celix_log.h"

typedef celix_status_t (*create_function_pt)(bundle_context_pt context, void **userData);
typedef celix_status_t (*start_function_pt)(void * handle, bundle_context_pt context);
typedef celix_status_t (*stop_function_pt)(void * handle, bundle_context_pt context);
typedef celix_status_t (*destroy_function_pt)(void * handle, bundle_context_pt context);

struct activator {
    void * userData;
    start_function_pt start;
    stop_function_pt stop;
    destroy_function_pt destroy;
};

celix_status_t framework_setBundleStateAndNotify(framework_pt framework, bundle_pt bundle, int state);
celix_status_t framework_markBundleResolved(framework_pt framework, module_pt module);

celix_status_t framework_acquireBundleLock(framework_pt framework, bundle_pt bundle, int desiredStates);
bool framework_releaseBundleLock(framework_pt framework, bundle_pt bundle);

bool framework_acquireGlobalLock(framework_pt framework);
celix_status_t framework_releaseGlobalLock(framework_pt framework);

celix_status_t framework_acquireInstallLock(framework_pt framework, char * location);
celix_status_t framework_releaseInstallLock(framework_pt framework, char * location);

long framework_getNextBundleId(framework_pt framework);

celix_status_t fw_installBundle2(framework_pt framework, bundle_pt * bundle, long id, char * location, char *inputFile, bundle_archive_pt archive);

celix_status_t fw_refreshBundles(framework_pt framework, bundle_pt bundles[], int size);
celix_status_t fw_refreshBundle(framework_pt framework, bundle_pt bundle);

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map);

celix_status_t fw_fireBundleEvent(framework_pt framework, bundle_event_type_e, bundle_pt bundle);
celix_status_t fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, bundle_pt bundle, celix_status_t errorCode);
static void *APR_THREAD_FUNC fw_eventDispatcher(apr_thread_t *thd, void *fw);

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle);
celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle);

struct fw_refreshHelper {
    framework_pt framework;
    bundle_pt bundle;
    bundle_state_e oldState;
};

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper);

struct fw_serviceListener {
	bundle_pt bundle;
	service_listener_pt listener;
	filter_pt filter;
};

typedef struct fw_serviceListener * fw_service_listener_pt;

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
	bundle_pt bundle;
	celix_status_t errorCode;
	char *error;

	char *filter;
};

typedef struct request *request_pt;

celix_status_t framework_create(framework_pt *framework, apr_pool_t *memoryPool, properties_pt config) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;

    *framework = (framework_pt) apr_palloc(memoryPool, sizeof(**framework));
    if (*framework != NULL) {
        apr_status_t apr_status = APR_SUCCESS;
        apr_status = CELIX_DO_IF(apr_status, apr_pool_create(&(*framework)->mp, memoryPool));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_cond_create(&(*framework)->condition, (*framework)->mp));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_mutex_create(&(*framework)->mutex, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_mutex_create(&(*framework)->bundleLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_mutex_create(&(*framework)->installRequestLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_mutex_create(&(*framework)->dispatcherLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp));
        apr_status = CELIX_DO_IF(apr_status, apr_thread_cond_create(&(*framework)->dispatcher, (*framework)->mp));
        if (apr_status == APR_SUCCESS) {
            (*framework)->bundle = NULL;
            (*framework)->installedBundleMap = NULL;
            (*framework)->registry = NULL;
            (*framework)->interrupted = false;
            (*framework)->shutdown = false;
            (*framework)->globalLockWaitersList = NULL;
            (*framework)->globalLockCount = 0;
            (*framework)->globalLockThread = 0;
            (*framework)->nextBundleId = 1l;
            (*framework)->cache = NULL;
            (*framework)->installRequestMap = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
            (*framework)->serviceListeners = NULL;
            (*framework)->bundleListeners = NULL;
            (*framework)->frameworkListeners = NULL;
            (*framework)->requests = NULL;
            (*framework)->shutdownGate = NULL;
            (*framework)->configurationMap = config;

            apr_pool_t *pool = NULL;
            apr_pool_create(&pool, (*framework)->mp);
            status = CELIX_DO_IF(status, bundle_create(&(*framework)->bundle, pool));
            status = CELIX_DO_IF(status, arrayList_create(&(*framework)->globalLockWaitersList));
            status = CELIX_DO_IF(status, bundle_setFramework((*framework)->bundle, (*framework)));
            if (status == CELIX_SUCCESS) {
                //
            } else {
                status = CELIX_FRAMEWORK_EXCEPTION;
                fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not create framework");
            }
        } else {
            status = CELIX_FRAMEWORK_EXCEPTION;
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not create framework");
        }
    } else {
        error = "FW exception";
        status = CELIX_FRAMEWORK_EXCEPTION;
        fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, CELIX_ENOMEM, "Could not create framework");
    }

    return status;
}

celix_status_t framework_destroy(framework_pt framework) {
    celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		linked_list_pt wires;
	    hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
		bundle_pt bundle = (bundle_pt) hashMapEntry_getValue(entry);
		char *location = (char *) hashMapEntry_getKey(entry);
		bundle_archive_pt archive = NULL;

		// for each installed bundle, clean up memory
		module_pt mod = NULL;
		bundle_getCurrentModule(bundle, &mod);
		wires = module_getWires(mod);
		if (wires != NULL) {
			linked_list_iterator_pt iter = linkedListIterator_create(wires, 0);
			while (linkedListIterator_hasNext(iter)) {
				wire_pt wire = (wire_pt) linkedListIterator_next(iter);
				linkedListIterator_remove(iter);
			}
			linkedListIterator_destroy(iter);
		}

//		if (bundle_getArchive(bundle, &archive) == CELIX_SUCCESS) {
//			bundleArchive_destroy(archive);
//		}
		bundle_destroy(bundle);
		hashMapIterator_remove(iterator);
	}
	hashMapIterator_destroy(iterator);

	hashMap_destroy(framework->installRequestMap, false, false);

//	serviceRegistry_destroy(framework->registry);

	arrayList_destroy(framework->globalLockWaitersList);
	arrayList_destroy(framework->serviceListeners);
	arrayList_destroy(framework->bundleListeners);
	arrayList_destroy(framework->frameworkListeners);
	arrayList_destroy(framework->requests);

	hashMap_destroy(framework->installedBundleMap, false, false);

	apr_pool_destroy(framework->mp);

	return status;
}

#ifdef _WIN32
    #define handle_t HMODULE
    #define fw_getSystemLibrary() fw_getCurrentModule()
    #define fw_openLibrary(path) LoadLibrary(path)
    #define fw_closeLibrary(handle) FreeLibrary(handle)

    #define fw_getSymbol(handle, name) GetProcAddress(handle, name)

    #define fw_getLastError() GetLastError()

    HMODULE fw_getCurrentModule() {
        HMODULE hModule = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)fw_getCurrentModule, &hModule);
        return hModule;
    }
#else
    #define handle_t void *
    #define fw_getSystemLibrary() dlopen(NULL, RTLD_LAZY|RTLD_LOCAL)
    #define fw_openLibrary(path) dlopen(path, RTLD_LAZY|RTLD_LOCAL)
    #define fw_closeLibrary(handle) dlclose(handle)
    #define fw_getSymbol(handle, name) dlsym(handle, name)
    #define fw_getLastError() dlerror()
#endif

celix_status_t fw_init(framework_pt framework) {
	bundle_state_e state;
	char *location;
	module_pt module = NULL;
	hash_map_pt wires;
	array_list_pt archives;
	bundle_archive_pt archive = NULL;
	char uuid[APR_UUID_FORMATTED_LENGTH+1];

	celix_status_t status = CELIX_SUCCESS;
	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, framework->bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	status = CELIX_DO_IF(status, arrayList_create(&framework->serviceListeners));
	status = CELIX_DO_IF(status, arrayList_create(&framework->bundleListeners));
	status = CELIX_DO_IF(status, arrayList_create(&framework->frameworkListeners));
	status = CELIX_DO_IF(status, arrayList_create(&framework->requests));
	status = CELIX_DO_IF(status, apr_thread_create(&framework->dispatcherThread, NULL, fw_eventDispatcher, framework, framework->mp));
	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	if (status == CELIX_SUCCESS) {
	    if ((state == OSGI_FRAMEWORK_BUNDLE_INSTALLED) || (state == OSGI_FRAMEWORK_BUNDLE_RESOLVED)) {
	        bundle_state_e state;
	        properties_pt props = properties_create();
	        properties_set(props, (char *) OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".cache");

	        status = CELIX_DO_IF(status, bundleCache_create(props, framework->mp, &framework->cache));
	        status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
	        if (status == CELIX_SUCCESS) {
	            if (state == OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
	                // clean cache
	                // bundleCache_delete(framework->cache);
	            }
            }
        }
	}

	if (status == CELIX_SUCCESS) {
        /*create and store framework uuid*/
        apr_uuid_t aprUuid;
        apr_uuid_get(&aprUuid);
        apr_uuid_format(uuid, &aprUuid);
        setenv(OSGI_FRAMEWORK_FRAMEWORK_UUID, uuid, true);

        framework->installedBundleMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	}

    status = CELIX_DO_IF(status, bundle_getArchive(framework->bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getLocation(archive, &location));
    if (status == CELIX_SUCCESS) {
        hashMap_put(framework->installedBundleMap, location, framework->bundle);
    }
    status = CELIX_DO_IF(status, bundle_getCurrentModule(framework->bundle, &module));
    if (status == CELIX_SUCCESS) {
        wires = resolver_resolve(module);
        if (wires != NULL) {
            framework_markResolvedModules(framework, wires);
            hashMap_destroy(wires, false, false);
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Unresolved constraints in System Bundle");
        }
    }

    status = CELIX_DO_IF(status, bundleCache_getArchives(framework->cache, framework->mp, &archives));
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
                char *location1 = NULL;
                status = bundleArchive_getLocation(archive1, &location1);
                fw_installBundle2(framework, &bundle, id, location1, NULL, archive1);
            }
        }
        arrayList_destroy(archives);
    }

    status = CELIX_DO_IF(status, serviceRegistry_create(framework->mp, framework, fw_serviceChanged, &framework->registry));
    status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, framework->bundle, OSGI_FRAMEWORK_BUNDLE_STARTING));
    status = CELIX_DO_IF(status, apr_thread_cond_create(&framework->shutdownGate, framework->mp));
    if (status == CELIX_SUCCESS) {
        handle_t handle = NULL;
        handle = fw_getSystemLibrary();
        if (handle != NULL) {
            bundle_setHandle(framework->bundle, handle);
        } else {
            status = CELIX_FRAMEWORK_EXCEPTION;
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR,  status, "Could not get handle to framework library");
        }
    }

    bundle_context_pt context = NULL;
    status = CELIX_DO_IF(status, bundleContext_create(framework, framework->bundle, &context));
    status = CELIX_DO_IF(status, bundle_setContext(framework->bundle, context));
    if (status == CELIX_SUCCESS) {
        activator_pt activator = NULL;
        activator = (activator_pt) apr_palloc(framework->mp, (sizeof(*activator)));
        if (activator != NULL) {
            bundle_context_pt context = NULL;
            void * userData = NULL;

            create_function_pt create = (create_function_pt) fw_getSymbol((handle_t) bundle_getHandle(framework->bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE);
            start_function_pt start = (start_function_pt) fw_getSymbol((handle_t) bundle_getHandle(framework->bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START);
            stop_function_pt stop = (stop_function_pt) fw_getSymbol((handle_t) bundle_getHandle(framework->bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP);
            destroy_function_pt destroy = (destroy_function_pt) fw_getSymbol((handle_t) bundle_getHandle(framework->bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY);

            activator->start = start;
            activator->stop = stop;
            activator->destroy = destroy;
            status = CELIX_DO_IF(status, bundle_setActivator(framework->bundle, activator));
            status = CELIX_DO_IF(status, bundle_getContext(framework->bundle, &context));

            if (status == CELIX_SUCCESS) {
                if (create != NULL) {
                    create(context, &userData);
                }
                activator->userData = userData;

                if (start != NULL) {
                    start(userData, context);
                }
            }
        } else {
            status = CELIX_ENOMEM;
        }
    }

    if (status != CELIX_SUCCESS) {
       fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not init framework");
    }

    framework_releaseBundleLock(framework, framework->bundle);

	return status;
}

celix_status_t framework_start(framework_pt framework) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e state;

	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, framework->bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
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

	    framework_releaseBundleLock(framework, framework->bundle);
	}

	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, framework->bundle));
	status = CELIX_DO_IF(status, fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_STARTED, framework->bundle, 0));

	if (status != CELIX_SUCCESS) {
       status = CELIX_BUNDLE_EXCEPTION;
       fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start framework");
       fw_fireFrameworkEvent(framework, OSGI_FRAMEWORK_EVENT_ERROR, framework->bundle, status);
    }

	return status;
}

void framework_stop(framework_pt framework) {
	fw_stopBundle(framework, framework->bundle, true);
}

celix_status_t fw_getProperty(framework_pt framework, const char *name, char **value) {
	celix_status_t status = CELIX_SUCCESS;

	if (framework == NULL || name == NULL || *value != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
		fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Missing required arguments");
	} else {
		if (framework->configurationMap != NULL) {
			*value = properties_get(framework->configurationMap, (char *) name);
		}
		if (*value == NULL) {
			*value = getenv(name);
		}
	}

	return status;
}

celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, char * location, char *inputFile) {
	return fw_installBundle2(framework, bundle, -1, location, inputFile, NULL);
}

celix_status_t fw_installBundle2(framework_pt framework, bundle_pt * bundle, long id, char * location, char *inputFile, bundle_archive_pt archive) {
    celix_status_t status = CELIX_SUCCESS;
//    bundle_archive_pt bundle_archive = NULL;
    bundle_state_e state;
	apr_pool_t *bundlePool;
  	bool locked;

  	status = CELIX_DO_IF(status, framework_acquireInstallLock(framework, location));
  	status = CELIX_DO_IF(status, bundle_getState(framework->bundle, &state));
  	if (status == CELIX_SUCCESS) {
        if (state == OSGI_FRAMEWORK_BUNDLE_STOPPING || state == OSGI_FRAMEWORK_BUNDLE_UNINSTALLED) {
            fw_log(OSGI_FRAMEWORK_LOG_INFO,  "The framework is being shutdown");
            status = CELIX_DO_IF(status, framework_releaseInstallLock(framework, location));
            status = CELIX_FRAMEWORK_SHUTDOWN;
        }
  	}

    if (status == CELIX_SUCCESS) {
        *bundle = framework_getBundle(framework, location);
        if (*bundle != NULL) {
            framework_releaseInstallLock(framework, location);
            return CELIX_SUCCESS;
        }

        apr_pool_create(&bundlePool, framework->mp);
        if (archive == NULL) {
            id = framework_getNextBundleId(framework);
            status = CELIX_DO_IF(status, bundleCache_createArchive(framework->cache, bundlePool, id, location, inputFile, &archive));
        } else {
            // purge revision
            // multiple revisions not yet implemented
        }

        if (status == CELIX_SUCCESS) {
            locked = framework_acquireGlobalLock(framework);
            if (!locked) {
                status = CELIX_BUNDLE_EXCEPTION;
            } else {
                status = CELIX_DO_IF(status, bundle_createFromArchive(bundle, framework, archive, bundlePool));

                framework_releaseGlobalLock(framework);
                if (status == CELIX_SUCCESS) {
                    hashMap_put(framework->installedBundleMap, location, *bundle);
                } else {
                    status = CELIX_BUNDLE_EXCEPTION;
                    status = CELIX_DO_IF(status, bundleArchive_closeAndDelete(archive));
                    apr_pool_destroy(bundlePool);
                }
            }
        }
    }
    status = CELIX_DO_IF(status, framework_releaseInstallLock(framework, location));

    if (status != CELIX_SUCCESS) {
    	fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not install bundle");
    } else {
        status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED, *bundle));
    }

  	return status;
}

celix_status_t framework_getBundleEntry(framework_pt framework, bundle_pt bundle, char *name, apr_pool_t *pool, char **entry) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_revision_pt revision;
	bundle_archive_pt archive = NULL;
    char *root;

	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
    status = CELIX_DO_IF(status, bundleRevision_getRoot(revision, &root));
    if (status == CELIX_SUCCESS) {
        char *e = NULL;
        apr_status_t ret;
        apr_finfo_t info;
        if ((strlen(name) > 0) && (name[0] == '/')) {
            name++;
        }
        apr_filepath_merge(&e, root, name, APR_FILEPATH_NOTABOVEROOT, framework->mp);
        ret = apr_stat(&info, e, APR_FINFO_DIRENT|APR_FINFO_TYPE, framework->mp);
        if (ret == APR_ENOENT) {
            (*entry) = NULL;
        } else if (ret == APR_SUCCESS || ret == APR_INCOMPLETE) {
            (*entry) = apr_pstrdup(pool, e);
        }
    }

	return status;
}

celix_status_t fw_startBundle(framework_pt framework, bundle_pt bundle, int options) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_pt wires = NULL;
	handle_t handle;
	bundle_context_pt context = NULL;
	bundle_state_e state;
	module_pt module = NULL;
	manifest_pt manifest = NULL;
	char *library;
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
	long refreshCount;
	char *archiveRoot;
	long revisionNumber;
	activator_pt activator = NULL;
	bundle_archive_pt archive = NULL;
	bundle_revision_pt revision = NULL;
	apr_pool_t *bundlePool = NULL;
	char *error = NULL;


	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	status = CELIX_DO_IF(status, bundle_getState(bundle, &state));

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
                bundle_getCurrentModule(bundle, &module);
                if (!module_isResolved(module)) {
                    wires = resolver_resolve(module);
                    if (wires == NULL) {
                        framework_releaseBundleLock(framework, bundle);
                        return CELIX_BUNDLE_EXCEPTION;
                    }
                    framework_markResolvedModules(framework, wires);
                }
                if (wires != NULL) {
                    hashMap_destroy(wires, false, false);
                }
                /* no break */
            case OSGI_FRAMEWORK_BUNDLE_RESOLVED:
                status = CELIX_DO_IF(status, bundleContext_create(framework, bundle, &context));
                status = CELIX_DO_IF(status, bundle_setContext(bundle, context));

                status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
                status = CELIX_DO_IF(status, bundle_getMemoryPool(bundle, &bundlePool));
                status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
                status = CELIX_DO_IF(status, bundleRevision_getManifest(revision, &manifest));
                if (status == CELIX_SUCCESS) {
                    library = manifest_getValue(manifest, OSGI_FRAMEWORK_HEADER_LIBRARY);
                }

                status = CELIX_DO_IF(status, bundleArchive_getRefreshCount(archive, &refreshCount));
                status = CELIX_DO_IF(status, bundleArchive_getArchiveRoot(archive, &archiveRoot));
                status = CELIX_DO_IF(status, bundleArchive_getCurrentRevisionNumber(archive, &revisionNumber));

                if (status == CELIX_SUCCESS) {
                    sprintf(libraryPath, "%s/version%ld.%ld/%s%s%s", archiveRoot, refreshCount, revisionNumber, library_prefix, library, library_extension);

                    // BUG: Can't use apr_dso_load, apr assumes RTLD_GLOBAL for loading libraries.
                    // apr_dso_handle_t *handle;
                    // apr_dso_load(&handle, libraryPath, bundle->memoryPool);
                    handle = fw_openLibrary(libraryPath);
                    if (handle == NULL) {
                        char err[1024];
                        sprintf(err, "library could not be opened: %s", fw_getLastError());
                        // #TODO this is wrong
                        error = err;
                        status =  CELIX_BUNDLE_EXCEPTION;
                    }

                    if (status == CELIX_SUCCESS) {
                        bundle_setHandle(bundle, handle);

                        activator = (activator_pt) apr_palloc(bundlePool, (sizeof(*activator)));
                        if (activator == NULL) {
                            status = CELIX_ENOMEM;
                        } else {
                            void * userData = NULL;
                            bundle_context_pt context;
                            create_function_pt create = (create_function_pt) fw_getSymbol((handle_t) bundle_getHandle(bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE);
                            start_function_pt start = (start_function_pt) fw_getSymbol((handle_t) bundle_getHandle(bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START);
                            stop_function_pt stop = (stop_function_pt) fw_getSymbol((handle_t) bundle_getHandle(bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP);
                            destroy_function_pt destroy = (destroy_function_pt) fw_getSymbol((handle_t) bundle_getHandle(bundle), OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY);

                            activator->start = start;
                            activator->stop = stop;
                            activator->destroy = destroy;
                            status = CELIX_DO_IF(status, bundle_setActivator(bundle, activator));

                            status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_STARTING));
                            status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING, bundle));

                            status = CELIX_DO_IF(status, bundle_getContext(bundle, &context));

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

                            status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_ACTIVE));
                            status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED, bundle));
                        }
                    }
                }

            break;
        }
	}

	framework_releaseBundleLock(framework, bundle);

	if (status != CELIX_SUCCESS) {
	    module_pt module = NULL;
	    char *symbolicName = NULL;
	    long id = 0;
	    bundle_getCurrentModule(bundle, &module);
	    module_getSymbolicName(module, &symbolicName);
	    bundle_getBundleId(bundle, &id);
	    if (error != NULL) {
	        fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]; cause: %s", symbolicName, id, error);
	    } else {
	        fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not start bundle: %s [%ld]", symbolicName, id);
	    }
	}

	return status;
}

celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e oldState;
	char *location;
	bundle_archive_pt archive = NULL;
	char *error = NULL;

	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	status = CELIX_DO_IF(status, bundle_getState(bundle, &oldState));
	if (status == CELIX_SUCCESS) {
        if (oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
            fw_stopBundle(framework, bundle, false);
        }
	}
	status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
	status = CELIX_DO_IF(status, bundleArchive_getLocation(archive, &location));

	if (status == CELIX_SUCCESS) {
	    bool locked = framework_acquireGlobalLock(framework);
	    if (!locked) {
	        status = CELIX_BUNDLE_EXCEPTION;
	        error = "Unable to acquire the global lock to update the bundle";
	    }
	}
	
	status = CELIX_DO_IF(status, bundle_revise(bundle, location, inputFile));
	status = CELIX_DO_IF(status, framework_releaseGlobalLock(framework));

	status = CELIX_DO_IF(status, bundleArchive_setLastModified(archive, time(NULL)));
	status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED));

	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bundle));
	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UPDATED, bundle));

    // Refresh packages?

	if (status == CELIX_SUCCESS) {
	    if (oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
	        status = CELIX_DO_IF(status, fw_startBundle(framework, bundle, 1));
	    }
	}

	framework_releaseBundleLock(framework, bundle);

	if (status != CELIX_SUCCESS) {
	    module_pt module = NULL;
        char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(bundle, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(bundle, &id);
        if (error != NULL) {
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot update bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot update bundle: %s [%ld]", symbolicName, id);
        }
	}

	return status;
}

celix_status_t fw_stopBundle(framework_pt framework, bundle_pt bundle, bool record) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_state_e state;
    activator_pt activator = NULL;
    bundle_context_pt context = NULL;
    module_pt module = NULL;
    manifest_pt manifest = NULL;
    bool wasActive = false;
    long id = 0;
    char *error = NULL;

	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));

	if (record) {
	    status = CELIX_DO_IF(status, bundle_setPersistentStateInactive(bundle));
    }

	status = CELIX_DO_IF(status, bundle_getState(bundle, &state));
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


	status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_STOPPING));
	status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING, bundle));
    status = CELIX_DO_IF(status, bundle_getBundleId(bundle, &id));
	if (status == CELIX_SUCCESS) {
	    if (wasActive || (id == 0)) {
	        activator = bundle_getActivator(bundle);

	        status = CELIX_DO_IF(status, bundle_getContext(bundle, &context));
	        if (status == CELIX_SUCCESS) {
                if (activator->stop != NULL) {
                    status = CELIX_DO_IF(status, activator->stop(activator->userData, context));
                }
	        }
            if (status == CELIX_SUCCESS) {
                if (activator->destroy != NULL) {
                    status = CELIX_DO_IF(status, activator->destroy(activator->userData, context));
                }
	        }
	    }

	    status = CELIX_DO_IF(status, bundle_getCurrentModule(bundle, &module));
	    if (status == CELIX_SUCCESS) {
            if (strcmp(module_getId(module), "0") != 0) {
                if (activator != NULL) {
                    activator->start = NULL;
                    activator->stop = NULL;
                    activator->userData = NULL;
                    //free(activator);
                    status = CELIX_DO_IF(status, bundle_setActivator(bundle, NULL));
                }

                status = CELIX_DO_IF(status, serviceRegistry_unregisterServices(framework->registry, bundle));
                if (status == CELIX_SUCCESS) {
                    serviceRegistry_ungetServices(framework->registry, bundle);
                }
                // #TODO remove listeners for bundle

                if (context != NULL) {
                    status = CELIX_DO_IF(status, bundleContext_destroy(context));
                    status = CELIX_DO_IF(status, bundle_setContext(bundle, NULL));
                }

                // #TODO enable dlclose call
                dlclose(bundle_getHandle(bundle));

                status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_RESOLVED));
            }
	    }
	}

	framework_releaseBundleLock(framework, bundle);

	if (status != CELIX_SUCCESS) {
	    module_pt module = NULL;
        char *symbolicName = NULL;
        long id = 0;
        bundle_getCurrentModule(bundle, &module);
        module_getSymbolicName(module, &symbolicName);
        bundle_getBundleId(bundle, &id);
        if (error != NULL) {
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot stop bundle: %s [%ld]; cause: %s", symbolicName, id, error);
        } else {
            fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Cannot stop bundle: %s [%ld]", symbolicName, id);
        }
 	} else {
        fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPED, bundle);
 	}

	return status;
}

celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state;
    bool locked;
    bundle_archive_pt archive = NULL;
    char * location;
    bundle_pt target;
    char *error = NULL;

    status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE|OSGI_FRAMEWORK_BUNDLE_STOPPING));
    status = CELIX_DO_IF(status, fw_stopBundle(framework, bundle, true));
    if (status == CELIX_SUCCESS) {
        locked = framework_acquireGlobalLock(framework);
        if (!locked) {
            status = CELIX_ILLEGAL_STATE;
            error = "Unable to acquire the global lock to uninstall the bundle";
        }
    }

    status = CELIX_DO_IF(status, bundle_getArchive(bundle, &archive));
    status = CELIX_DO_IF(status, bundleArchive_getLocation(archive, &location));
    if (status == CELIX_SUCCESS) {
        // TODO sync issues?
        target = (bundle_pt) hashMap_remove(framework->installedBundleMap, location);

        if (target != NULL) {
            status = CELIX_DO_IF(status, bundle_setPersistentStateUninstalled(target));
            // fw_rememberUninstalledBundle(framework, target);
        }
    }

    framework_releaseGlobalLock(framework);

    if (status == CELIX_SUCCESS) {
        if (target == NULL) {
            fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Could not remove bundle from installed map");
        }
    }

    status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED));

    status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bundle));

    status = CELIX_DO_IF(status, framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_UNINSTALLED));
    status = CELIX_DO_IF(status, bundleArchive_setLastModified(archive, time(NULL)));

    framework_releaseBundleLock(framework, bundle);

    status = CELIX_DO_IF(status, fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNINSTALLED, bundle));

    if (status == CELIX_SUCCESS) {
        locked = framework_acquireGlobalLock(framework);
        if (locked) {
            bundle_pt bundles[] = { bundle };
            celix_status_t refreshStatus = fw_refreshBundles(framework, bundles, 1);
            if (refreshStatus != CELIX_SUCCESS) {
                printf("Could not refresh bundle");
            } else {
                status = CELIX_DO_IF(status, bundle_destroy(bundle));
            }

            status = CELIX_DO_IF(status, framework_releaseGlobalLock(framework));
        }
    }


    if (status != CELIX_SUCCESS) {
//        module_pt module = NULL;
//        char *symbolicName = NULL;
//        long id = 0;
//        bundle_getCurrentModule(bundle, &module);
//        module_getSymbolicName(module, &symbolicName);
//        bundle_getBundleId(bundle, &id);

        framework_logIfError(status, error, "Cannot uninstall bundle");
    }

    return status;
}

celix_status_t fw_refreshBundles(framework_pt framework, bundle_pt bundles[], int size) {
    celix_status_t status = CELIX_SUCCESS;

    bool locked = framework_acquireGlobalLock(framework);
    if (!locked) {
        framework_releaseGlobalLock(framework);
        status = CELIX_ILLEGAL_STATE;
    } else {
		hash_map_values_pt values;
        bundle_pt *newTargets;
        unsigned int nrofvalues;
		bool restart = false;
        hash_map_pt map = hashMap_create(NULL, NULL, NULL, NULL);
        int targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            bundle_pt bundle = bundles[targetIdx];
            hashMap_put(map, bundle, bundle);
            fw_populateDependentGraph(framework, bundle, &map);
        }
        values = hashMapValues_create(map);
        hashMapValues_toArray(values, (void ***) &newTargets, &nrofvalues);
        hashMapValues_destroy(values);

        hashMap_destroy(map, false, false);
            
        if (newTargets != NULL) {
            int i = 0;
			struct fw_refreshHelper * helpers;
            for (i = 0; i < nrofvalues && !restart; i++) {
                bundle_pt bundle = (bundle_pt) newTargets[i];
                if (framework->bundle == bundle) {
                    restart = true;
                }
            }

            helpers = (struct fw_refreshHelper * )malloc(nrofvalues * sizeof(struct fw_refreshHelper));
            for (i = 0; i < nrofvalues && !restart; i++) {
                bundle_pt bundle = (bundle_pt) newTargets[i];
                helpers[i].framework = framework;
                helpers[i].bundle = bundle;
                helpers[i].oldState = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
            }

            for (i = 0; i < nrofvalues; i++) {
                struct fw_refreshHelper helper = helpers[i];
                fw_refreshHelper_stop(&helper);
                fw_refreshHelper_refreshOrRemove(&helper);
            }

            for (i = 0; i < nrofvalues; i++) {
                struct fw_refreshHelper helper = helpers[i];
                fw_refreshHelper_restart(&helper);
            }

            if (restart) {
                bundle_update(framework->bundle, NULL);
            }
			free(helpers);
			free(newTargets);
        }

        framework_releaseGlobalLock(framework);
    }

    framework_logIfError(status, NULL, "Cannot refresh bundles");

    return status;
}

celix_status_t fw_refreshBundle(framework_pt framework, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state;

    status = framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED | OSGI_FRAMEWORK_BUNDLE_RESOLVED);
    if (status != CELIX_SUCCESS) {
        printf("Cannot refresh bundle");
        framework_releaseBundleLock(framework, bundle);
    } else {
    	bool fire;
		bundle_getState(bundle, &state);
        fire = (state != OSGI_FRAMEWORK_BUNDLE_INSTALLED);
        bundle_refresh(bundle);

        if (fire) {
            framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED);
            fw_fireBundleEvent(framework, OSGI_FRAMEWORK_BUNDLE_EVENT_UNRESOLVED, bundle);
        }

        framework_releaseBundleLock(framework, bundle);
    }

    framework_logIfError(status, NULL, "Cannot refresh bundle");

    return status;
}

celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper) {
	bundle_state_e state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
        refreshHelper->oldState = OSGI_FRAMEWORK_BUNDLE_ACTIVE;
        fw_stopBundle(refreshHelper->framework, refreshHelper->bundle, false);
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
        fw_refreshBundle(refreshHelper->framework, refreshHelper->bundle);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper) {
    if ((refreshHelper->bundle != NULL) && (refreshHelper->oldState == OSGI_FRAMEWORK_BUNDLE_ACTIVE)) {
        fw_startBundle(refreshHelper->framework, refreshHelper->bundle, 0);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_getDependentBundles(framework_pt framework, bundle_pt exporter, array_list_pt *list) {
    celix_status_t status = CELIX_SUCCESS;

    if (*list == NULL && exporter != NULL && framework != NULL) {
		array_list_pt modules;
		unsigned int modIdx = 0;
        arrayList_create(list);

        modules = bundle_getModules(exporter);
        for (modIdx = 0; modIdx < arrayList_size(modules); modIdx++) {
            module_pt module = (module_pt) arrayList_get(modules, modIdx);
            array_list_pt dependents = module_getDependents(module);
            unsigned int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                module_pt dependent = (module_pt) arrayList_get(dependents, depIdx);
                arrayList_add(*list, module_getBundle(dependent));
            }
            arrayList_destroy(dependents);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(status, NULL, "Cannot get dependent bundles");

    return status;
}

celix_status_t fw_populateDependentGraph(framework_pt framework, bundle_pt exporter, hash_map_pt *map) {
    celix_status_t status = CELIX_SUCCESS;

    if (exporter != NULL && framework != NULL) {
        array_list_pt dependents = NULL;
        if ((status = fw_getDependentBundles(framework, exporter, &dependents)) == CELIX_SUCCESS) {
            unsigned int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                if (!hashMap_containsKey(*map, arrayList_get(dependents, depIdx))) {
                    hashMap_put(*map, arrayList_get(dependents, depIdx), arrayList_get(dependents, depIdx));
                    fw_populateDependentGraph(framework, (bundle_pt) arrayList_get(dependents, depIdx), map);
                }
            }
            arrayList_destroy(dependents);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(status, NULL, "Cannot populate dependent graph");

    return status;
}

celix_status_t fw_registerService(framework_pt framework, service_registration_pt *registration, bundle_pt bundle, char * serviceName, void * svcObj, properties_pt properties) {
	celix_status_t status = CELIX_SUCCESS;
	char *error = NULL;
	if (serviceName == NULL || svcObj == NULL) {
	    status = CELIX_ILLEGAL_ARGUMENT;
	    error = "ServiceName and SvcObj cannot be null";
	}

	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	status = CELIX_DO_IF(status, serviceRegistry_registerService(framework->registry, bundle, serviceName, svcObj, properties, registration));
	bool res = framework_releaseBundleLock(framework, bundle);
	if (!res) {
	    status = CELIX_ILLEGAL_STATE;
	    error = "Could not release bundle lock";
	}

	if (status == CELIX_SUCCESS) {
	    // If this is a listener hook, invoke the callback with all current listeners
        if (strcmp(serviceName, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME) == 0) {
            unsigned int i;
            array_list_pt infos = NULL;
            apr_pool_t *subpool;
            service_reference_pt ref = NULL;
            listener_hook_service_pt hook;
            apr_pool_t *pool = NULL;

            status = CELIX_DO_IF(status, bundle_getMemoryPool(bundle, &pool));
            status = CELIX_DO_IF(status, arrayList_create(&infos));

            if (status == CELIX_SUCCESS) {
                celix_status_t subs = CELIX_SUCCESS;
                for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
                    fw_service_listener_pt listener =(fw_service_listener_pt) arrayList_get(framework->serviceListeners, i);
                    apr_pool_t *pool = NULL;
                    bundle_context_pt context = NULL;
                    listener_hook_info_pt info = NULL;
                    bundle_context_pt lContext = NULL;

                    subs = CELIX_DO_IF(subs, bundle_getContext(bundle, &context));
                    subs = CELIX_DO_IF(subs, bundleContext_getMemoryPool(context, &pool));
                    if (subs == CELIX_SUCCESS) {
                        info = (listener_hook_info_pt) apr_palloc(pool, sizeof(*info));
                        if (info == NULL) {
                            subs = CELIX_ENOMEM;
                        }
                    }

                    subs = CELIX_DO_IF(subs, bundle_getContext(listener->bundle, &lContext));
                    if (subs == CELIX_SUCCESS) {
                        info->context = lContext;
                        info->removed = false;
                    }
                    subs = CELIX_DO_IF(subs, filter_getString(listener->filter, &info->filter));

                    if (subs == CELIX_SUCCESS) {
                        arrayList_add(infos, info);
                    }
                    if (subs != CELIX_SUCCESS) {
                        fw_logCode(OSGI_FRAMEWORK_LOG_ERROR, status, "Could not pass all listeners to the hook: %s", serviceName);
                    }
                }

                bool ungetResult = false;

                status = CELIX_DO_IF(status, apr_pool_create(&subpool, pool));

                status = CELIX_DO_IF(status, serviceRegistry_createServiceReference(framework->registry, subpool, *registration, &ref));
                status = CELIX_DO_IF(status, fw_getService(framework,framework->bundle, ref, (void **) &hook));
                if (status == CELIX_SUCCESS) {
                    hook->added(hook->handle, infos);
                }
                status = CELIX_DO_IF(status, serviceRegistry_ungetService(framework->registry, framework->bundle, ref, &ungetResult));

                apr_pool_destroy(subpool);
             }
        }
	}

    framework_logIfError(status, error, "Cannot register service: %s", serviceName);

	return status;
}

celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt *registration, bundle_pt bundle, char * serviceName, service_factory_pt factory, properties_pt properties) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
	if (serviceName == NULL || factory == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        error = "Service name and factory cannot be null";
    }

	status = CELIX_DO_IF(status, framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_STARTING|OSGI_FRAMEWORK_BUNDLE_ACTIVE));
	status = CELIX_DO_IF(status, serviceRegistry_registerServiceFactory(framework->registry, bundle, serviceName, factory, properties, registration));
    if (!framework_releaseBundleLock(framework, bundle)) {
        status = CELIX_ILLEGAL_STATE;
        error = "Could not release bundle lock";
    }

    framework_logIfError(status, error, "Cannot register service factory: %s", serviceName);

    return CELIX_SUCCESS;
}

celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char * serviceName, char * sfilter) {
    celix_status_t status = CELIX_SUCCESS;

	filter_pt filter = NULL;
	unsigned int refIdx = 0;
	apr_pool_t *pool = NULL;

	status = CELIX_DO_IF(status, bundle_getMemoryPool(bundle, &pool));
	if (status == CELIX_SUCCESS) {
        if (sfilter != NULL) {
            filter = filter_create(sfilter, pool);
        }
	}

	status = CELIX_DO_IF(status, serviceRegistry_getServiceReferences(framework->registry, pool, serviceName, filter, references));

	if (filter != NULL) {
		filter_destroy(filter);
	}

	if (status == CELIX_SUCCESS) {
        for (refIdx = 0; (*references != NULL) && refIdx < arrayList_size(*references); refIdx++) {
            service_reference_pt ref = (service_reference_pt) arrayList_get(*references, refIdx);
            service_registration_pt reg = NULL;
            char * serviceName;
            properties_pt props = NULL;
            status = CELIX_DO_IF(status, serviceReference_getServiceRegistration(ref, &reg));
            status = CELIX_DO_IF(status, serviceRegistration_getProperties(reg, &props));
            if (status == CELIX_SUCCESS) {
                serviceName = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
                if (!serviceReference_isAssignableTo(ref, bundle, serviceName)) {
                    arrayList_remove(*references, refIdx);
                    refIdx--;
                }
            }
        }
	}

	framework_logIfError(status, NULL, "Failed to get service references");

	return status;
}

celix_status_t fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, void **service) {
	return serviceRegistry_getService(framework->registry, bundle, reference, service);
}

celix_status_t fw_getBundleRegisteredServices(framework_pt framework, apr_pool_t *pool, bundle_pt bundle, array_list_pt *services) {
	return serviceRegistry_getRegisteredServices(framework->registry, pool, bundle, services);
}

celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	status = serviceRegistry_getServicesInUse(framework->registry, bundle, services);
	return status;
}

celix_status_t framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, bool *result) {
	return serviceRegistry_ungetService(framework->registry, bundle, reference, result);
}

void fw_addServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener, char * sfilter) {
//	apr_pool_t *pool;
//	apr_pool_t *bpool;
//	bundle_context_pt context;
//	bundle_getContext(bundle, &context);
//	bundleContext_getMemoryPool(context, &bpool);
//	apr_pool_create(&pool, bpool);
	array_list_pt listenerHooks = NULL;
	apr_pool_t *subpool;
	listener_hook_info_pt info;
	unsigned int i;
	
	fw_service_listener_pt fwListener = (fw_service_listener_pt) malloc(sizeof(*fwListener));
	apr_pool_t *pool = NULL;

	bundle_context_pt context = NULL;

	bundle_getMemoryPool(bundle, &pool);

	fwListener->bundle = bundle;
	if (sfilter != NULL) {
		filter_pt filter = filter_create(sfilter, pool);
		fwListener->filter = filter;
	} else {
		fwListener->filter = NULL;
	}
	fwListener->listener = listener;
	arrayList_add(framework->serviceListeners, fwListener);

	apr_pool_create(&subpool, listener->pool);
	serviceRegistry_getListenerHooks(framework->registry, subpool, &listenerHooks);

	info = (listener_hook_info_pt) apr_palloc(subpool, sizeof(*info));

	bundle_getContext(bundle, &context);
	info->context = context;

	info->removed = false;
	info->filter = sfilter == NULL ? NULL : apr_pstrdup(subpool, sfilter);

	for (i = 0; i < arrayList_size(listenerHooks); i++) {
		service_reference_pt ref = (service_reference_pt) arrayList_get(listenerHooks, i);
		listener_hook_service_pt hook = NULL;
		array_list_pt infos = NULL;
		bool ungetResult = false;

		celix_status_t status = fw_getService(framework, framework->bundle, ref, (void **) &hook);

		arrayList_create(&infos);
		arrayList_add(infos, info);
		hook->added(hook->handle, infos);
		serviceRegistry_ungetService(framework->registry, framework->bundle, ref, &ungetResult);
	}

	arrayList_destroy(listenerHooks);
	apr_pool_destroy(subpool);
}

void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener) {
	listener_hook_info_pt info = NULL;
	apr_pool_t *pool;
	unsigned int i;
	fw_service_listener_pt element;

	bundle_context_pt context;
	bundle_getContext(bundle, &context);
	bundleContext_getMemoryPool(context, &pool);
	
	for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
		element = (fw_service_listener_pt) arrayList_get(framework->serviceListeners, i);
		if (element->listener == listener && element->bundle == bundle) {
			bundle_context_pt lContext = NULL;

			info = (listener_hook_info_pt) apr_palloc(pool, sizeof(*info));

			bundle_getContext(element->bundle, &context);
			info->context = lContext;

			// TODO Filter toString;
			filter_getString(element->filter, &info->filter);
			info->removed = true;

			arrayList_remove(framework->serviceListeners, i);
			i--;
			element->bundle = NULL;
			filter_destroy(element->filter);
			element->filter = NULL;
			element->listener = NULL;
			free(element);
			element = NULL;
			break;
		}
	}

	if (info != NULL) {
		unsigned int i;
		array_list_pt listenerHooks = NULL;
		serviceRegistry_getListenerHooks(framework->registry, pool, &listenerHooks);
		
		for (i = 0; i < arrayList_size(listenerHooks); i++) {
			service_reference_pt ref = (service_reference_pt) arrayList_get(listenerHooks, i);
			listener_hook_service_pt hook = NULL;
			array_list_pt infos = NULL;
			bool ungetResult;

			celix_status_t status = fw_getService(framework, framework->bundle, ref, (void **) &hook);

			arrayList_create(&infos);
			arrayList_add(infos, info);
			hook->removed(hook->handle, infos);
			serviceRegistry_ungetService(framework->registry, framework->bundle, ref, &ungetResult);
		}

		arrayList_destroy(listenerHooks);
	}
}

celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *pool = NULL;
	fw_bundle_listener_pt bundleListener = NULL;

	apr_pool_create(&pool, framework->mp);
	bundleListener = (fw_bundle_listener_pt) apr_palloc(pool, sizeof(*bundleListener));
	if (!bundleListener) {
		status = CELIX_ENOMEM;
	} else {
		bundleListener->listener = listener;
		bundleListener->bundle = bundle;

		arrayList_add(framework->bundleListeners, bundleListener);
	}

	framework_logIfError(status, NULL, "Failed to add bundle listener");

	return status;
}

celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
	celix_status_t status = CELIX_SUCCESS;

	unsigned int i;
	fw_bundle_listener_pt bundleListener;

	for (i = 0; i < arrayList_size(framework->bundleListeners); i++) {
		bundleListener = (fw_bundle_listener_pt) arrayList_get(framework->bundleListeners, i);
		if (bundleListener->listener == listener && bundleListener->bundle == bundle) {
			arrayList_remove(framework->bundleListeners, i);
		}
	}

	framework_logIfError(status, NULL, "Failed to remove bundle listener");

	return status;
}

celix_status_t fw_addFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *pool = NULL;
	fw_framework_listener_pt frameworkListener = NULL;

	apr_pool_create(&pool, framework->mp);
	frameworkListener = (fw_framework_listener_pt) apr_palloc(pool, sizeof(*frameworkListener));
	if (!frameworkListener) {
		status = CELIX_ENOMEM;
	} else {
		frameworkListener->listener = listener;
		frameworkListener->bundle = bundle;

		arrayList_add(framework->frameworkListeners, frameworkListener);
	}

	framework_logIfError(status, NULL, "Failed to add framework listener");

	return status;
}

celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
	celix_status_t status = CELIX_SUCCESS;

	unsigned int i;
	fw_framework_listener_pt frameworkListener;

	for (i = 0; i < arrayList_size(framework->frameworkListeners); i++) {
		frameworkListener = (fw_framework_listener_pt) arrayList_get(framework->frameworkListeners, i);
		if (frameworkListener->listener == listener && frameworkListener->bundle == bundle) {
			arrayList_remove(framework->frameworkListeners, i);
		}
	}

	framework_logIfError(status, NULL, "Failed to remove framework listener");

	return status;
}

void fw_serviceChanged(framework_pt framework, service_event_type_e eventType, service_registration_pt registration, properties_pt oldprops) {
	unsigned int i;
	fw_service_listener_pt element;
	if (arrayList_size(framework->serviceListeners) > 0) {
		for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
			int matched = 0;
			properties_pt props = NULL;
			bool matchResult = false;

			element = (fw_service_listener_pt) arrayList_get(framework->serviceListeners, i);
			serviceRegistration_getProperties(registration, &props);
			if (element->filter != NULL) {
				filter_match(element->filter, props, &matchResult);
			}
			matched = (element->filter == NULL) || matchResult;
			if (matched) {
				service_reference_pt reference = NULL;
				service_event_pt event;
				apr_pool_t *spool = NULL;
				apr_pool_create(&spool, element->listener->pool);

				event = (service_event_pt) apr_palloc(spool, sizeof(*event));

				serviceRegistry_createServiceReference(framework->registry, spool, registration, &reference);

				event->type = eventType;
				event->reference = reference;

				element->listener->serviceChanged(element->listener, event);
			} else if (eventType == OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED) {
				bool matchResult = false;
				int matched = 0;
				if (element->filter != NULL) {
					filter_match(element->filter, oldprops, &matchResult);
				}
				matched = (element->filter == NULL) || matchResult;
				if (matched) {
					service_reference_pt reference = NULL;
					service_event_pt endmatch = (service_event_pt) malloc(sizeof(*endmatch));

					serviceRegistry_createServiceReference(framework->registry, element->listener->pool, registration, &reference);

					endmatch->reference = reference;
					endmatch->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH;
					element->listener->serviceChanged(element->listener, endmatch);
				}
			}
		}
	}
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

celix_status_t framework_markResolvedModules(framework_pt framework, hash_map_pt resolvedModuleWireMap) {
	if (resolvedModuleWireMap != NULL) {
		hash_map_iterator_pt iterator = hashMapIterator_create(resolvedModuleWireMap);
		while (hashMapIterator_hasNext(iterator)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
			module_pt module = (module_pt) hashMapEntry_getKey(entry);
			linked_list_pt wires = (linked_list_pt) hashMapEntry_getValue(entry);

			module_setWires(module, wires);

			module_setResolved(module);
			resolver_moduleResolved(module);
			framework_markBundleResolved(framework, module);
		}
		hashMapIterator_destroy(iterator);
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_markBundleResolved(framework_pt framework, module_pt module) {
	bundle_pt bundle = module_getBundle(module);
	bundle_state_e state;

	if (bundle != NULL) {
		framework_acquireBundleLock(framework, bundle, OSGI_FRAMEWORK_BUNDLE_INSTALLED|OSGI_FRAMEWORK_BUNDLE_RESOLVED|OSGI_FRAMEWORK_BUNDLE_ACTIVE);
		bundle_getState(bundle, &state);
		if (state != OSGI_FRAMEWORK_BUNDLE_INSTALLED) {
			printf("Trying to resolve a resolved bundle");
		} else {
			framework_setBundleStateAndNotify(framework, bundle, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
			// framework_fireBundleEvent(BUNDLE_EVENT_RESOLVED, bundle);
		}
		framework_releaseBundleLock(framework, bundle);
	}

	return CELIX_SUCCESS;
}

array_list_pt framework_getBundles(framework_pt framework) {
	array_list_pt bundles = NULL;
	hash_map_iterator_pt iterator;
	arrayList_create(&bundles);
	iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		bundle_pt bundle = (bundle_pt) hashMapIterator_nextValue(iterator);
		arrayList_add(bundles, bundle);
	}
	hashMapIterator_destroy(iterator);
	return bundles;
}

bundle_pt framework_getBundle(framework_pt framework, char * location) {
	bundle_pt bundle = (bundle_pt) hashMap_get(framework->installedBundleMap, location);
	return bundle;
}

bundle_pt framework_getBundleById(framework_pt framework, long id) {
	hash_map_iterator_pt iter = hashMapIterator_create(framework->installedBundleMap);
	bundle_pt bundle = NULL;
	while (hashMapIterator_hasNext(iter)) {
		bundle_pt b = (bundle_pt) hashMapIterator_nextValue(iter);
		bundle_archive_pt archive = NULL;
		long bid;
		bundle_getArchive(b, &archive);
		bundleArchive_getId(archive, &bid);
		if (bid == id) {
			bundle = b;
			break;
		}
	}
	hashMapIterator_destroy(iter);
	return bundle;
}

celix_status_t framework_acquireInstallLock(framework_pt framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	while (hashMap_get(framework->installRequestMap, location) != NULL) {
		apr_thread_cond_wait(framework->condition, framework->installRequestLock);
	}
	hashMap_put(framework->installRequestMap, location, location);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_releaseInstallLock(framework_pt framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	hashMap_remove(framework->installRequestMap, location);
	apr_thread_cond_broadcast(framework->condition);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_setBundleStateAndNotify(framework_pt framework, bundle_pt bundle, int state) {
	int ret = CELIX_SUCCESS;

	int err = apr_thread_mutex_lock(framework->bundleLock);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Failed to lock");
		return CELIX_BUNDLE_EXCEPTION;
	}

	bundle_setState(bundle, state);
	err = apr_thread_cond_broadcast(framework->condition);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Failed to broadcast");
		ret = CELIX_BUNDLE_EXCEPTION;
	}

	err = apr_thread_mutex_unlock(framework->bundleLock);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Failed to unlock");
		return CELIX_BUNDLE_EXCEPTION;
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_acquireBundleLock(framework_pt framework, bundle_pt bundle, int desiredStates) {
	celix_status_t status = CELIX_SUCCESS;

	bool locked;
	apr_os_thread_t lockingThread = 0;

	int err = apr_thread_mutex_lock(framework->bundleLock);
	if (err != APR_SUCCESS) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Failed to lock");
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		bool lockable = false;
		bool isSelf = false;

		bundle_isLockable(bundle, &lockable);
		thread_equalsSelf(framework->globalLockThread, &isSelf);

		while (!lockable
				|| ((framework->globalLockThread != 0)
				&& !isSelf)) {
			bundle_state_e state;
			bundle_getState(bundle, &state);
			if ((desiredStates & state) == 0) {
				status = CELIX_ILLEGAL_STATE;
				break;
			} else
				bundle_getLockingThread(bundle, &lockingThread);
				if (isSelf
					&& (lockingThread != 0)
					&& arrayList_contains(framework->globalLockWaitersList, &lockingThread)) {
				framework->interrupted = true;
	//			pthread_cond_signal_thread_np(&framework->condition, bundle_getLockingThread(bundle));
				apr_thread_cond_signal(framework->condition);
			}

			apr_thread_cond_wait(framework->condition, framework->bundleLock);

			status = bundle_isLockable(bundle, &lockable);
			if (status != CELIX_SUCCESS) {
				break;
			}
		}

		if (status == CELIX_SUCCESS) {
			bundle_state_e state;
			bundle_getState(bundle, &state);
			if ((desiredStates & state) == 0) {
				status = CELIX_ILLEGAL_STATE;
			} else {
				if (bundle_lock(bundle, &locked)) {
					if (!locked) {
						status = CELIX_ILLEGAL_STATE;
					}
				}
			}
		}
		apr_thread_mutex_unlock(framework->bundleLock);
	}

	framework_logIfError(status, NULL, "Failed to get bundle lock");

	return status;
}

bool framework_releaseBundleLock(framework_pt framework, bundle_pt bundle) {
    bool unlocked;
    apr_os_thread_t lockingThread = 0;

    apr_thread_mutex_lock(framework->bundleLock);

    bundle_unlock(bundle, &unlocked);
	if (!unlocked) {
	    apr_thread_mutex_unlock(framework->bundleLock);
		return false;
	}
	bundle_getLockingThread(bundle, &lockingThread);
	if (lockingThread == 0) {
	    apr_thread_cond_broadcast(framework->condition);
	}

	apr_thread_mutex_unlock(framework->bundleLock);

	return true;
}

bool framework_acquireGlobalLock(framework_pt framework) {
    bool interrupted = false;
	bool isSelf = false;

	apr_thread_mutex_lock(framework->bundleLock);

	thread_equalsSelf(framework->globalLockThread, &isSelf);

	while (!interrupted
			&& (framework->globalLockThread != 0)
			&& (!isSelf)) {
		apr_os_thread_t currentThread = apr_os_thread_current();
		arrayList_add(framework->globalLockWaitersList, &currentThread);
		apr_thread_cond_broadcast(framework->condition);

		apr_thread_cond_wait(framework->condition, framework->bundleLock);
		if (framework->interrupted) {
			interrupted = true;
			framework->interrupted = false;
		}

		arrayList_removeElement(framework->globalLockWaitersList, &currentThread);
	}

	if (!interrupted) {
		framework->globalLockCount++;
		framework->globalLockThread = apr_os_thread_current();
	}

	apr_thread_mutex_unlock(framework->bundleLock);

	return !interrupted;
}

celix_status_t framework_releaseGlobalLock(framework_pt framework) {
	int status = CELIX_SUCCESS;
	if (apr_thread_mutex_lock(framework->bundleLock) != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error locking framework bundle lock");
		return CELIX_FRAMEWORK_EXCEPTION;
	}

	if (framework->globalLockThread == apr_os_thread_current()) {
		framework->globalLockCount--;
		if (framework->globalLockCount == 0) {
			framework->globalLockThread = 0;
			if (apr_thread_cond_broadcast(framework->condition) != 0) {
				fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Failed to broadcast global lock release.");
				status = CELIX_FRAMEWORK_EXCEPTION;
				// still need to unlock before returning
			}
		}
	} else {
		printf("The current thread does not own the global lock");
	}

	if (apr_thread_mutex_unlock(framework->bundleLock) != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error unlocking framework bundle lock");
		return CELIX_FRAMEWORK_EXCEPTION;
	}

	framework_logIfError(status, NULL, "Failed to release global lock");

	return status;
}

celix_status_t framework_waitForStop(framework_pt framework) {
	if (apr_thread_mutex_lock(framework->mutex) != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Error locking the framework, shutdown gate not set.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	while (!framework->shutdown) {
		apr_status_t apr_status = apr_thread_cond_wait(framework->shutdownGate, framework->mutex);
		if (apr_status != 0) {
			fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Error waiting for shutdown gate.");
			return CELIX_FRAMEWORK_EXCEPTION;
		}
	}
	if (apr_thread_mutex_unlock(framework->mutex) != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Error unlocking the framework.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	fw_log(OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Successful shutdown");
	return CELIX_SUCCESS;
}

static void *APR_THREAD_FUNC framework_shutdown(apr_thread_t *thd, void *framework) {
//static void * framework_shutdown(void * framework) {
	framework_pt fw = (framework_pt) framework;
	hash_map_iterator_pt iterator;
	int err;

	fw_log(OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Shutdown");

	iterator = hashMapIterator_create(fw->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		bundle_pt bundle = (bundle_pt) hashMapIterator_nextValue(iterator);
		bundle_state_e state;
		bundle_getState(bundle, &state);
		if (state == OSGI_FRAMEWORK_BUNDLE_ACTIVE || state == OSGI_FRAMEWORK_BUNDLE_STARTING) {
			char *location;
			bundle_archive_pt archive = NULL;

			bundle_getArchive(bundle, &archive);
			bundleArchive_getLocation(archive, &location);
			fw_stopBundle(fw, bundle, 0);
		}
	}
	hashMapIterator_destroy(iterator);

	err = apr_thread_mutex_lock(fw->mutex);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error locking the framework, cannot exit clean.");
		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	fw->shutdown = true;
	err = apr_thread_cond_broadcast(fw->shutdownGate);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error waking the shutdown gate, cannot exit clean.");
		err = apr_thread_mutex_unlock(fw->mutex);
		if (err != 0) {
			fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error unlocking the framework, cannot exit clean.");
		}

		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	err = apr_thread_mutex_unlock(fw->mutex);
	if (err != 0) {
		fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error unlocking the framework, cannot exit clean.");
	}

	fw_log(OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Shutdown done\n");
	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;
}

celix_status_t framework_getFrameworkBundle(framework_pt framework, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (framework != NULL && *bundle == NULL) {
		*bundle = framework->bundle;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to get framework bundle");

	return status;
}

celix_status_t fw_fireBundleEvent(framework_pt framework, bundle_event_type_e eventType, bundle_pt bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if ((eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_STARTING)
			&& (eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING)
			&& (eventType != OSGI_FRAMEWORK_BUNDLE_EVENT_LAZY_ACTIVATION)) {
		request_pt request = (request_pt) malloc(sizeof(*request));
		if (!request) {
			status = CELIX_ENOMEM;
		} else {
			request->bundle = bundle;
			request->eventType = eventType;
			request->filter = NULL;
			request->listeners = framework->bundleListeners;
			request->type = BUNDLE_EVENT_TYPE;
			request->error = NULL;

			arrayList_add(framework->requests, request);
			if (apr_thread_mutex_lock(framework->dispatcherLock) != APR_SUCCESS) {
				status = CELIX_FRAMEWORK_EXCEPTION;
			} else {
				if (apr_thread_cond_broadcast(framework->dispatcher)) {
					status = CELIX_FRAMEWORK_EXCEPTION;
				} else {
					if (apr_thread_mutex_unlock(framework->dispatcherLock)) {
						status = CELIX_FRAMEWORK_EXCEPTION;
					}
				}
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to fire bundle event");

	return status;
}

celix_status_t fw_fireFrameworkEvent(framework_pt framework, framework_event_type_e eventType, bundle_pt bundle, celix_status_t errorCode) {
	celix_status_t status = CELIX_SUCCESS;

	request_pt request = (request_pt) malloc(sizeof(*request));
	if (!request) {
		status = CELIX_ENOMEM;
	} else {
		request->bundle = bundle;
		request->eventType = eventType;
		request->filter = NULL;
		request->listeners = framework->frameworkListeners;
		request->type = FRAMEWORK_EVENT_TYPE;
		request->errorCode = errorCode;

		if (errorCode != CELIX_SUCCESS) {
		    char message[256];
            celix_strerror(errorCode, message, 256);
            request->error = message;
		}

		arrayList_add(framework->requests, request);
		if (apr_thread_mutex_lock(framework->dispatcherLock) != APR_SUCCESS) {
			status = CELIX_FRAMEWORK_EXCEPTION;
		} else {
			if (apr_thread_cond_broadcast(framework->dispatcher)) {
				status = CELIX_FRAMEWORK_EXCEPTION;
			} else {
				if (apr_thread_mutex_unlock(framework->dispatcherLock)) {
					status = CELIX_FRAMEWORK_EXCEPTION;
				}
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to fire framework event");

	return status;
}

static void *APR_THREAD_FUNC fw_eventDispatcher(apr_thread_t *thd, void *fw) {
	framework_pt framework = (framework_pt) fw;
	request_pt request = NULL;

	while (true) {
		int size;
		apr_status_t status;

		if (apr_thread_mutex_lock(framework->dispatcherLock) != 0) {
			fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error locking the dispatcher");
			return NULL;
		}

		size = arrayList_size(framework->requests);
		while (size == 0 && !framework->shutdown) {
			apr_status_t apr_status = apr_thread_cond_wait(framework->dispatcher, framework->dispatcherLock);
			// Ignore status and just keep waiting
			size = arrayList_size(framework->requests);
		}

		if (size == 0 && framework->shutdown) {
			apr_thread_exit(thd, APR_SUCCESS);
			return NULL;
		}
		
		request = (request_pt) arrayList_remove(framework->requests, 0);

		if ((status = apr_thread_mutex_unlock(framework->dispatcherLock)) != 0) {
			fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Error unlocking the dispatcher.");
			apr_thread_exit(thd, status);
			return NULL;
		}

		if (request != NULL) {
			int i;
			int size = arrayList_size(request->listeners);
			for (i = 0; i < size; i++) {
				if (request->type == BUNDLE_EVENT_TYPE) {
					fw_bundle_listener_pt listener = (fw_bundle_listener_pt) arrayList_get(request->listeners, i);
					bundle_event_pt event = (bundle_event_pt) apr_palloc(listener->listener->pool, sizeof(*event));
					event->bundle = request->bundle;
					event->type = request->eventType;

					fw_invokeBundleListener(framework, listener->listener, event, listener->bundle);
				} else if (request->type == FRAMEWORK_EVENT_TYPE) {
					fw_framework_listener_pt listener = (fw_framework_listener_pt) arrayList_get(request->listeners, i);
					framework_event_pt event = (framework_event_pt) apr_palloc(listener->listener->pool, sizeof(*event));
					event->bundle = request->bundle;
					event->type = request->eventType;
					event->error = request->error;
					event->errorCode = request->errorCode;

					fw_invokeFrameworkListener(framework, listener->listener, event, listener->bundle);
				}
			}
		}
	}

	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;

}

celix_status_t fw_invokeBundleListener(framework_pt framework, bundle_listener_pt listener, bundle_event_pt event, bundle_pt bundle) {
	// We only support async bundle listeners for now
	bundle_state_e state;
	bundle_getState(bundle, &state);
	if (state == OSGI_FRAMEWORK_BUNDLE_STARTING || state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {

		listener->bundleChanged(listener, event);
	}

	return CELIX_SUCCESS;
}

celix_status_t fw_invokeFrameworkListener(framework_pt framework, framework_listener_pt listener, framework_event_pt event, bundle_pt bundle) {
	bundle_state_e state;
	bundle_getState(bundle, &state);
	if (state == OSGI_FRAMEWORK_BUNDLE_STARTING || state == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
		listener->frameworkEvent(listener, event);
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	// nothing to do
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;

	apr_thread_t *shutdownThread;
	framework_pt framework;

	if (bundleContext_getFramework(context, &framework) == CELIX_SUCCESS) {

	    fw_log(OSGI_FRAMEWORK_LOG_INFO, "FRAMEWORK: Start shutdownthread");
	    if (apr_thread_create(&shutdownThread, NULL, framework_shutdown, framework, framework->mp) == APR_SUCCESS) {
//            apr_thread_join(&status, shutdownThread);
            apr_thread_detach(shutdownThread);
	    } else {
            fw_log(OSGI_FRAMEWORK_LOG_ERROR,  "Could not create shutdown thread, normal exit not possible.");
	        status = CELIX_FRAMEWORK_EXCEPTION;
	    }
	} else {
		status = CELIX_FRAMEWORK_EXCEPTION;
	}

	framework_logIfError(status, NULL, "Failed to stop framework activator");

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	return CELIX_SUCCESS;
}
