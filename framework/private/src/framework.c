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
#include <apr_thread_cond.h>
#include <apr_thread_mutex.h>
#include <apr_thread_proc.h>
#ifdef _WIN32
#include <winbase.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif


#include "framework.h"
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
#include "bundle_context.h"
#include "linked_list_iterator.h"
#include "service_reference.h"
#include "listener_hook_service.h"
#include "service_registration.h"
#include "celix_log.h"

struct framework {
	struct bundle * bundle;
	hash_map_t installedBundleMap;
	hash_map_t installRequestMap;
	array_list_t serviceListeners;
	array_list_t bundleListeners;

	long nextBundleId;
	struct serviceRegistry * registry;
bundle_cache_t cache;

	apr_thread_cond_t *shutdownGate;
	apr_thread_cond_t *condition;

	apr_thread_mutex_t *installRequestLock;
	apr_thread_mutex_t *mutex;
	apr_thread_mutex_t *bundleLock;

	apr_os_thread_t globalLockThread;
	array_list_t globalLockWaitersList;
	int globalLockCount;

	bool interrupted;
	bool shutdown;

	apr_pool_t *mp;

	properties_t configurationMap;

	array_list_t requests;
	apr_thread_cond_t *dispatcher;
	apr_thread_mutex_t *dispatcherLock;
	apr_thread_t *dispatcherThread;
};

struct activator {
	void * userData;
	void (*start)(void * userData, bundle_context_t context);
	void (*stop)(void * userData, bundle_context_t context);
	void (*destroy)(void * userData, bundle_context_t context);
};

celix_status_t framework_setBundleStateAndNotify(framework_t framework, bundle_t bundle, int state);
celix_status_t framework_markBundleResolved(framework_t framework, module_t module);

celix_status_t framework_acquireBundleLock(framework_t framework, bundle_t bundle, int desiredStates);
bool framework_releaseBundleLock(framework_t framework, bundle_t bundle);

bool framework_acquireGlobalLock(framework_t framework);
celix_status_t framework_releaseGlobalLock(framework_t framework);

celix_status_t framework_acquireInstallLock(framework_t framework, char * location);
celix_status_t framework_releaseInstallLock(framework_t framework, char * location);

long framework_getNextBundleId(framework_t framework);

celix_status_t fw_installBundle2(framework_t framework, bundle_t * bundle, long id, char * location, char *inputFile, bundle_archive_t archive);

celix_status_t fw_refreshBundles(framework_t framework, bundle_t bundles[], int size);
celix_status_t fw_refreshBundle(framework_t framework, bundle_t bundle);

celix_status_t fw_populateDependentGraph(framework_t framework, bundle_t exporter, hash_map_t *map);

celix_status_t fw_fireBundleEvent(framework_t framework, bundle_event_type_e, bundle_t bundle);
static void *APR_THREAD_FUNC fw_eventDispatcher(apr_thread_t *thd, void *fw);
celix_status_t fw_invokeBundleListener(framework_t framework, bundle_listener_t listener, bundle_event_t event, bundle_t bundle);

struct fw_refreshHelper {
    framework_t framework;
    bundle_t bundle;
    bundle_state_e oldState;
};

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper);

struct fw_serviceListener {
	bundle_t bundle;
	service_listener_t listener;
	filter_t filter;
};

typedef struct fw_serviceListener * fw_service_listener_t;

struct fw_bundleListener {
	apr_pool_t *pool;
	bundle_t bundle;
	bundle_listener_t listener;
};

typedef struct fw_bundleListener * fw_bundle_listener_t;

enum event_type {
	FRAMEWORK_EVENT_TYPE,
	BUNDLE_EVENT_TYPE,
	EVENT_TYPE_SERVICE,
};

typedef enum event_type event_type_e;

struct request {
	event_type_e type;
	array_list_t listeners;

	int eventType;
	bundle_t bundle;

	char *filter;
};

typedef struct request *request_t;

celix_status_t framework_create(framework_t *framework, apr_pool_t *memoryPool, properties_t config) {
    celix_status_t status = CELIX_SUCCESS;

	*framework = (framework_t) apr_palloc(memoryPool, sizeof(**framework));
	if (*framework == NULL) {
		status = CELIX_ENOMEM;
	} else {
	    apr_status_t apr_status = apr_pool_create(&(*framework)->mp, memoryPool);
        if (apr_status != APR_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
        } else {
            bundle_t bundle = NULL;
            apr_pool_t *bundlePool;
            apr_status_t apr_status = apr_pool_create(&bundlePool, (*framework)->mp);
            if (apr_status != APR_SUCCESS) {
                status = CELIX_FRAMEWORK_EXCEPTION;
            } else {
                apr_status_t apr_status = bundle_create(&bundle, bundlePool);
                if (apr_status != CELIX_SUCCESS) {
                    status = CELIX_FRAMEWORK_EXCEPTION;
                } else {
                    apr_status_t apr_status = apr_thread_cond_create(&(*framework)->condition, (*framework)->mp);
                    if (apr_status != APR_SUCCESS) {
                        status = CELIX_FRAMEWORK_EXCEPTION;
                    } else {
                        apr_status_t apr_status = apr_thread_mutex_create(&(*framework)->mutex, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp);
                        if (apr_status != APR_SUCCESS) {
                            status = CELIX_FRAMEWORK_EXCEPTION;
                        } else {
                            apr_status_t apr_status = apr_thread_mutex_create(&(*framework)->bundleLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp);
                            if (apr_status != APR_SUCCESS) {
                                status = CELIX_FRAMEWORK_EXCEPTION;
                            } else {
                                apr_status_t apr_status = apr_thread_mutex_create(&(*framework)->installRequestLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp);
                                if (apr_status != APR_SUCCESS) {
                                    status = CELIX_FRAMEWORK_EXCEPTION;
                                } else {
                                	apr_status_t apr_status = apr_thread_mutex_create(&(*framework)->dispatcherLock, APR_THREAD_MUTEX_UNNESTED, (*framework)->mp);
                                	if (apr_status != CELIX_SUCCESS) {
										status = CELIX_FRAMEWORK_EXCEPTION;
									} else {
										apr_status_t apr_status = apr_thread_cond_create(&(*framework)->dispatcher, (*framework)->mp);
										if (apr_status != APR_SUCCESS) {
											status = CELIX_FRAMEWORK_EXCEPTION;
										} else {
											(*framework)->bundle = bundle;

											(*framework)->installedBundleMap = NULL;
											(*framework)->registry = NULL;

											(*framework)->interrupted = false;
											(*framework)->shutdown = false;

											(*framework)->globalLockWaitersList = NULL;
											arrayList_create((*framework)->mp, &(*framework)->globalLockWaitersList);
											(*framework)->globalLockCount = 0;
											(*framework)->globalLockThread = 0;
											(*framework)->nextBundleId = 1l;
											(*framework)->cache = NULL;

											(*framework)->installRequestMap = hashMap_create(string_hash, string_hash, string_equals, string_equals);
											(*framework)->serviceListeners = NULL;
											(*framework)->bundleListeners = NULL;
											(*framework)->requests = NULL;
											(*framework)->shutdownGate = NULL;
											(*framework)->configurationMap = config;

											status = bundle_setFramework((*framework)->bundle, (*framework));
										}
									}
                                }
                            }
                        }
                    }
                }
            }
        }
	}

	return status;
}

celix_status_t framework_destroy(framework_t framework) {
    celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_t iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		linked_list_t wires;
	    hash_map_entry_t entry = hashMapIterator_nextEntry(iterator);
		bundle_t bundle = (bundle_t) hashMapEntry_getValue(entry);
		char *location = (char *) hashMapEntry_getKey(entry);
		bundle_archive_t archive = NULL;

		// for each installed bundle, clean up memory
		module_t mod = NULL;
		bundle_getCurrentModule(bundle, &mod);
		wires = module_getWires(mod);
		if (wires != NULL) {
			linked_list_iterator_t iter = linkedListIterator_create(wires, 0);
			while (linkedListIterator_hasNext(iter)) {
				wire_t wire = (wire_t) linkedListIterator_next(iter);
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

	serviceRegistry_destroy(framework->registry);

	arrayList_destroy(framework->globalLockWaitersList);
	arrayList_destroy(framework->serviceListeners);
	arrayList_destroy(framework->bundleListeners);
	arrayList_destroy(framework->requests);

	hashMap_destroy(framework->installedBundleMap, false, false);

	apr_pool_destroy(framework->mp);

	return status;
}

typedef void *(*create_function_t)(bundle_context_t context, void **userData);
typedef void (*start_function_t)(void * handle, bundle_context_t context);
typedef void (*stop_function_t)(void * handle, bundle_context_t context);
typedef void (*destroy_function_t)(void * handle, bundle_context_t context);

celix_status_t fw_init(framework_t framework) {
	celix_status_t status = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	bundle_state_e state;
	char *location;
	module_t module = NULL;
	hash_map_t wires;
	array_list_t archives;
	bundle_archive_t archive = NULL;

	if (status != CELIX_SUCCESS) {
		framework_releaseBundleLock(framework, framework->bundle);
		return status;
	}

	framework->requests = NULL;
	arrayList_create(framework->mp, &framework->requests);
	if (apr_thread_create(&framework->dispatcherThread, NULL, fw_eventDispatcher, framework, framework->mp) != APR_SUCCESS) {
		return CELIX_FRAMEWORK_EXCEPTION;
	}

	bundle_getState(framework->bundle, &state);
	if ((state == BUNDLE_INSTALLED) || (state == BUNDLE_RESOLVED)) {
	    properties_t props = properties_create();
		properties_set(props, (char *) FRAMEWORK_STORAGE, ".cache");
		status = bundleCache_create(props, framework->mp, &framework->cache);
		if (status == CELIX_SUCCESS) {
			bundle_state_e state;
			bundle_getState(framework->bundle, &state);
            if (state == BUNDLE_INSTALLED) {
                // clean cache
                // bundleCache_delete(framework->cache);
            }
		} else {
		    return status;
		}
	}

	framework->installedBundleMap = hashMap_create(string_hash, NULL, string_equals, NULL);

	status = bundle_getArchive(framework->bundle, &archive);
	status = bundleArchive_getLocation(archive, &location);
	hashMap_put(framework->installedBundleMap, location, framework->bundle);

	status = bundle_getCurrentModule(framework->bundle, &module);
	if (status != CELIX_SUCCESS) {
		return status;
	}
	wires = resolver_resolve(module);
	if (wires == NULL) {
		printf("Unresolved constraints in System Bundle\n");
		framework_releaseBundleLock(framework, framework->bundle);
		return CELIX_BUNDLE_EXCEPTION;
	} else {
		framework_markResolvedModules(framework, wires);
		hashMap_destroy(wires, false, false);
	}

	// reload archives from cache
	status = bundleCache_getArchives(framework->cache, framework->mp, &archives);
	if (status != CELIX_SUCCESS) {
	    return status;
	} else {
#ifdef _WIN32
		HMODULE this_process;
#endif
        unsigned int arcIdx;
		void * handle;
		bundle_context_t context = NULL;
		ACTIVATOR activator;

        for (arcIdx = 0; arcIdx < arrayList_size(archives); arcIdx++) {
            bundle_archive_t archive1 = (bundle_archive_t) arrayList_get(archives, arcIdx);
            long id;
			bundle_state_e bundleState;
            bundleArchive_getId(archive1, &id);
    		framework->nextBundleId = framework->nextBundleId > id + 1 ? framework->nextBundleId : id + 1;
    		
    		bundleArchive_getPersistentState(archive1, &bundleState);
            if (bundleState == BUNDLE_UNINSTALLED) {
                bundleArchive_closeAndDelete(archive1);
            } else {
                bundle_t bundle = NULL;
                char *location1 = NULL;
				status = bundleArchive_getLocation(archive1, &location1);
                fw_installBundle2(framework, &bundle, id, location1, NULL, archive1);
            }
        }
        arrayList_destroy(archives);
        framework->registry = serviceRegistry_create(framework, fw_serviceChanged);

        framework_setBundleStateAndNotify(framework, framework->bundle, BUNDLE_STARTING);

        apr_thread_cond_create(&framework->shutdownGate, framework->mp);

#ifdef _WIN32
		GetModuleHandleEx(0,0,&this_process);
		handle = GetModuleHandle(0);
#else
        handle = dlopen(NULL, RTLD_LAZY|RTLD_LOCAL);
#endif
        if (handle == NULL) {
#ifdef _WIN32
			printf ("%s\n", GetLastError());
#else
            printf ("%s\n", dlerror());
#endif
            framework_releaseBundleLock(framework, framework->bundle);
            return CELIX_START_ERROR;
        }
        
        if (bundleContext_create(framework, framework->bundle, &context) != CELIX_SUCCESS) {
            return CELIX_START_ERROR;
        }
        bundle_setContext(framework->bundle, context);

        bundle_setHandle(framework->bundle, handle);

        activator = (ACTIVATOR) apr_palloc(framework->mp, (sizeof(*activator)));
        if (activator == NULL) {
            status = CELIX_ENOMEM;
        }  else {
			bundle_context_t context = NULL;
			void * userData = NULL;
#ifdef _WIN32
			create_function_t create = (create_function_t) GetProcAddress((HMODULE) bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_CREATE);
			start_function_t start = (start_function_t) GetProcAddress((HMODULE) bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_START);
			stop_function_t stop = (stop_function_t) GetProcAddress((HMODULE) bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_STOP);
			destroy_function_t destroy = (destroy_function_t) GetProcAddress((HMODULE) bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_DESTROY);
#else
            create_function_t create = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_CREATE);
            start_function_t start = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_START);
            stop_function_t stop = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_STOP);
            destroy_function_t destroy = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_DESTROY);
#endif
            activator->start = start;
            activator->stop = stop;
            activator->destroy = destroy;
            bundle_setActivator(framework->bundle, activator);
            
			bundle_getContext(framework->bundle, &context);
            
			if (create != NULL) {
                create(context, &userData);
            }
            activator->userData = userData;

            if (start != NULL) {
                start(userData, context);
            }

            framework->serviceListeners = NULL;
            arrayList_create(framework->mp, &framework->serviceListeners);
            framework->bundleListeners = NULL;
            arrayList_create(framework->mp, &framework->bundleListeners);
            framework->requests = NULL;
            arrayList_create(framework->mp, &framework->requests);

            framework_releaseBundleLock(framework, framework->bundle);

            status = CELIX_SUCCESS;
        }
	}
	return status;
}

celix_status_t framework_start(framework_t framework) {
	celix_status_t lock = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	bundle_state_e state;

	if (lock != CELIX_SUCCESS) {
		printf("could not get lock\n");
		framework_releaseBundleLock(framework, framework->bundle);
		return lock;
	}


	bundle_getState(framework->bundle, &state);

	if ((state == BUNDLE_INSTALLED) || (state == BUNDLE_RESOLVED)) {
		fw_init(framework);
	}

	bundle_getState(framework->bundle, &state);
	if (state == BUNDLE_STARTING) {
		framework_setBundleStateAndNotify(framework, framework->bundle, BUNDLE_ACTIVE);
	}

	framework_releaseBundleLock(framework, framework->bundle);

	return CELIX_SUCCESS;
}

void framework_stop(framework_t framework) {
	fw_stopBundle(framework, framework->bundle, true);
}

celix_status_t fw_getProperty(framework_t framework, const char *name, char **value) {
	celix_status_t status = CELIX_SUCCESS;

	if (framework == NULL || name == NULL || *value != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
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

celix_status_t fw_installBundle(framework_t framework, bundle_t * bundle, char * location, char *inputFile) {
	return fw_installBundle2(framework, bundle, -1, location, inputFile, NULL);
}

celix_status_t fw_installBundle2(framework_t framework, bundle_t * bundle, long id, char * location, char *inputFile, bundle_archive_t archive) {
    bundle_archive_t bundle_archive = NULL;
    bundle_state_e state;
	apr_pool_t *bundlePool;
  	bool locked;

	celix_status_t status = framework_acquireInstallLock(framework, location);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    bundle_getState(framework->bundle, &state);

	if (state == BUNDLE_STOPPING || state == BUNDLE_UNINSTALLED) {
		printf("The framework has been shutdown.\n");
		framework_releaseInstallLock(framework, location);
		return CELIX_FRAMEWORK_SHUTDOWN;
	}

	*bundle = framework_getBundle(framework, location);
	if (*bundle != NULL) {
		framework_releaseInstallLock(framework, location);
		return CELIX_SUCCESS;
	}

	apr_pool_create(&bundlePool, framework->mp);
	if (archive == NULL) {
		id = framework_getNextBundleId(framework);
		status = bundleCache_createArchive(framework->cache, bundlePool, id, location, inputFile, &bundle_archive);
		if (status != CELIX_SUCCESS) {
		    framework_releaseInstallLock(framework, location);
		    return status;
		} else {
		    archive = bundle_archive;
		}
	} else {
		// purge revision
		// multiple revisions not yet implemented
	}

	locked = framework_acquireGlobalLock(framework);
	if (!locked) {
		printf("Unable to acquire the global lock to install the bundle\n");
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
	    status = bundle_createFromArchive(bundle, framework, archive, bundlePool);
	}

    framework_releaseGlobalLock(framework);
	if (status == CELIX_SUCCESS) {
        hashMap_put(framework->installedBundleMap, location, *bundle);
	} else {
	    printf("Unable to install bundle: %s\n", location);
	    bundleArchive_closeAndDelete(bundle_archive);
	    apr_pool_destroy(bundlePool);
	}
    framework_releaseInstallLock(framework, location);

  	return status;
}

celix_status_t framework_getBundleEntry(framework_t framework, bundle_t bundle, char *name, apr_pool_t *pool, char **entry) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_revision_t revision;
	bundle_archive_t archive = NULL;

	status = bundle_getArchive(bundle, &archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getCurrentRevision(archive, &revision);
		if (status == CELIX_SUCCESS) {
			char *root;
			if ((strlen(name) > 0) && (name[0] == '/')) {
				name++;
			}


			status = bundleRevision_getRoot(revision, &root);
			if (status == CELIX_SUCCESS) {
				char *e = NULL;
				apr_status_t ret;
				apr_finfo_t info;
				apr_filepath_merge(&e, root, name, APR_FILEPATH_NOTABOVEROOT, framework->mp);
				ret = apr_stat(&info, e, APR_FINFO_DIRENT|APR_FINFO_TYPE, framework->mp);
				if (ret == APR_ENOENT) {
					(*entry) = NULL;
				} else if (ret == APR_SUCCESS || ret == APR_INCOMPLETE) {
					(*entry) = apr_pstrdup(pool, e);
				}
			}
		}
	}

	return status;
}

celix_status_t fw_startBundle(framework_t framework, bundle_t bundle, int options) {
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);

	hash_map_t wires = NULL;
	void * handle;
	bundle_context_t context = NULL;
	bundle_state_e state;
	module_t module = NULL;
	MANIFEST manifest = NULL;
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
	ACTIVATOR activator;
	bundle_archive_t archive = NULL;
	apr_pool_t *bundlePool = NULL;

	if (lock != CELIX_SUCCESS) {
		printf("Unable to start\n");
		framework_releaseBundleLock(framework, bundle);
		return lock;
	}

	bundle_getState(bundle, &state);

	switch (state) {
		case BUNDLE_UNINSTALLED:
			printf("Cannot start bundle since it is uninstalled.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_ILLEGAL_STATE;
		case BUNDLE_STARTING:
			printf("Cannot start bundle since it is starting.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_BUNDLE_EXCEPTION;
		case BUNDLE_STOPPING:
			printf("Cannot start bundle since it is stopping.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_ILLEGAL_STATE;
		case BUNDLE_ACTIVE:
			printf("Cannot start bundle since it is already active.");
			framework_releaseBundleLock(framework, bundle);
			return CELIX_SUCCESS;
		case BUNDLE_INSTALLED:
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
		case BUNDLE_RESOLVED:
			if (bundleContext_create(framework, bundle, &context) != CELIX_SUCCESS) {
				return CELIX_ENOMEM;
			}
			bundle_setContext(bundle, context);

			bundle_getArchive(bundle, &archive);
			bundle_getMemoryPool(bundle, &bundlePool);

			if (getManifest(archive, bundlePool, &manifest) == CELIX_SUCCESS) {
                bundle_setManifest(bundle, manifest);
                library = manifest_getValue(manifest, HEADER_LIBRARY);
			}

			bundleArchive_getRefreshCount(archive, &refreshCount);
			bundleArchive_getArchiveRoot(archive, &archiveRoot);
			bundleArchive_getCurrentRevisionNumber(archive, &revisionNumber);

			sprintf(libraryPath, "%s/version%ld.%ld/%s%s%s",
					archiveRoot,
					refreshCount,
					revisionNumber,
					library_prefix,
					library,
					library_extension
					);

			// BUG: Can't use apr_dso_load, apr assumes RTLD_GLOBAL for loading libraries.
//			apr_dso_handle_t *handle;
//			apr_dso_load(&handle, libraryPath, bundle->memoryPool);
#ifdef _WIN32
			handle = LoadLibrary(libraryPath);
#else
			handle = dlopen(libraryPath, RTLD_LAZY|RTLD_LOCAL);
#endif
			if (handle == NULL) {
#ifdef _WIN32
				printf ("%s\n", GetLastError());
#else
				printf ("%s\n", dlerror());
#endif
				framework_releaseBundleLock(framework, bundle);
				return CELIX_BUNDLE_EXCEPTION;
			}

			bundle_setHandle(bundle, handle);

			activator = (ACTIVATOR) apr_palloc(bundlePool, (sizeof(*activator)));
			if (activator == NULL) {
			    return CELIX_ENOMEM;
			} else {
				void * userData = NULL;
                bundle_context_t context;
#ifdef _WIN32
                
				create_function_t create = (create_function_t) GetProcAddress((HMODULE) bundle_getHandle(bundle), BUNDLE_ACTIVATOR_CREATE);
				start_function_t start = (start_function_t) GetProcAddress((HMODULE) bundle_getHandle(bundle), BUNDLE_ACTIVATOR_START);
				stop_function_t stop = (stop_function_t) GetProcAddress((HMODULE) bundle_getHandle(bundle), BUNDLE_ACTIVATOR_STOP);
				destroy_function_t destroy = (destroy_function_t) GetProcAddress((HMODULE) bundle_getHandle(bundle), BUNDLE_ACTIVATOR_DESTROY);
#else
				create_function_t create = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_CREATE);
                start_function_t start = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_START);
                stop_function_t stop = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_STOP);
                destroy_function_t destroy = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_DESTROY);
#endif

                activator->start = start;
                activator->stop = stop;
                activator->destroy = destroy;
                bundle_setActivator(bundle, activator);

                framework_setBundleStateAndNotify(framework, bundle, BUNDLE_STARTING);

                

                bundle_getContext(bundle, &context);

                if (create != NULL) {
                    create(context, &userData);
                }
                activator->userData = userData;

                if (start != NULL) {
                    start(userData, context);
                }

                framework_setBundleStateAndNotify(framework, bundle, BUNDLE_ACTIVE);

                fw_fireBundleEvent(framework, BUNDLE_EVENT_STARTED, bundle);
			}

			break;
	}

	framework_releaseBundleLock(framework, bundle);
	return CELIX_SUCCESS;
}

celix_status_t framework_updateBundle(framework_t framework, bundle_t bundle, char *inputFile) {
	celix_status_t status;
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_ACTIVE);
	bundle_state_e oldState;
	char *location;
	bool locked;
	bundle_archive_t archive = NULL;

	if (lock != CELIX_SUCCESS) {
		printf("Cannot update bundle, in wrong state");
		framework_releaseBundleLock(framework, bundle);
		return lock;
	}
	
	bundle_getState(bundle, &oldState);
	bundle_getArchive(bundle, &archive);
	
	bundleArchive_getLocation(archive, &location);

	if (oldState == BUNDLE_ACTIVE) {
		fw_stopBundle(framework, bundle, false);
	}

	locked = framework_acquireGlobalLock(framework);
	if (!locked) {
		printf("Unable to acquire the global lock to update the bundle\n");
		framework_releaseInstallLock(framework, location);
		return CELIX_BUNDLE_EXCEPTION;
	}

	bundle_revise(bundle, location, inputFile);
	framework_releaseGlobalLock(framework);

	status = bundleArchive_setLastModified(archive, time(NULL));
	framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);

	// Refresh packages?

	if (oldState == BUNDLE_ACTIVE) {
		fw_startBundle(framework, bundle, 1);
	}
	framework_releaseBundleLock(framework, bundle);

	return CELIX_SUCCESS;
}

celix_status_t fw_stopBundle(framework_t framework, bundle_t bundle, bool record) {
	celix_status_t status = CELIX_SUCCESS;

	status = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (status != CELIX_SUCCESS) {
		printf("Cannot stop bundle");
		framework_releaseBundleLock(framework, bundle);
	} else {
		bundle_state_e state;
		ACTIVATOR activator;
		bundle_context_t context;
		module_t module = NULL;
		MANIFEST manifest = NULL;
		if (record) {
			bundle_setPersistentStateInactive(bundle);
		}

		//if (!fw_isBundlePersistentlyStarted(framework, bundle)) {
		//}
		
		bundle_getState(bundle, &state);

		switch (state) {
			case BUNDLE_UNINSTALLED:
				printf("Cannot stop bundle since it is uninstalled.");
				framework_releaseBundleLock(framework, bundle);
				return status;
			case BUNDLE_STARTING:
				printf("Cannot stop bundle since it is starting.");
				framework_releaseBundleLock(framework, bundle);
				return status;
			case BUNDLE_STOPPING:
				printf("Cannot stop bundle since it is stopping.");
				framework_releaseBundleLock(framework, bundle);
				return status;
			case BUNDLE_INSTALLED:
			case BUNDLE_RESOLVED:
				framework_releaseBundleLock(framework, bundle);
				return status;
			case BUNDLE_ACTIVE:
				// only valid state
				break;
		}

		framework_setBundleStateAndNotify(framework, bundle, BUNDLE_STOPPING);

		activator = bundle_getActivator(bundle);
		
		bundle_getContext(bundle, &context);
		if (activator->stop != NULL) {
			activator->stop(activator->userData, context);
		}

		if (activator->destroy != NULL) {
			activator->destroy(activator->userData, context);
		}

		bundle_getCurrentModule(bundle, &module);
		if (strcmp(module_getId(module), "0") != 0) {
			activator->start = NULL;
			activator->stop = NULL;
			activator->userData = NULL;
			//free(activator);
			bundle_setActivator(bundle, NULL);

			serviceRegistry_unregisterServices(framework->registry, bundle);
			serviceRegistry_ungetServices(framework->registry, bundle);

//			dlclose(bundle_getHandle(bundle));
		}

		bundleContext_destroy(context);
		bundle_setContext(bundle, NULL);

		bundle_getManifest(bundle, &manifest);

		framework_setBundleStateAndNotify(framework, bundle, BUNDLE_RESOLVED);

		framework_releaseBundleLock(framework, bundle);
	}

	return status;
}

celix_status_t fw_uninstallBundle(framework_t framework, bundle_t bundle) {
    celix_status_t status = CELIX_SUCCESS;

    bundle_state_e state;

    celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE|BUNDLE_STOPPING);
    if (lock != CELIX_SUCCESS) {
        printf("Cannot stop bundle");
        framework_releaseBundleLock(framework, bundle);
        bundle_getState(bundle, &state);
        if (state == BUNDLE_UNINSTALLED) {
            status = CELIX_ILLEGAL_STATE;
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    } else {
		bool locked;
        fw_stopBundle(framework, bundle, true);

        locked = framework_acquireGlobalLock(framework);
        if (!locked) {
            printf("Unable to acquire the global lock to install the bundle\n");
            framework_releaseGlobalLock(framework);
            status = CELIX_ILLEGAL_STATE;
        } else {
            bundle_archive_t archive = NULL;
            char * location;
			bundle_t target;
			status = bundle_getArchive(bundle, &archive);
            status = bundleArchive_getLocation(archive, &location);
            // TODO sync issues?
            target = (bundle_t) hashMap_remove(framework->installedBundleMap, location);

            if (target != NULL) {
                bundle_setPersistentStateUninstalled(target);
                // fw_rememberUninstalledBundle(framework, target);
            }

            framework_releaseGlobalLock(framework);

            if (target == NULL) {
                printf("Could not remove bundle from installed map");
            }

            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);
            fw_fireBundleEvent(framework, BUNDLE_EVENT_UNRESOLVED, bundle);

            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_UNINSTALLED);
            status = bundleArchive_setLastModified(archive, time(NULL));
        }
        framework_releaseBundleLock(framework, bundle);

        fw_fireBundleEvent(framework, BUNDLE_EVENT_UNINSTALLED, bundle);

        locked = framework_acquireGlobalLock(framework);
        if (locked) {
            bundle_t bundles[] = { bundle };
            celix_status_t refreshStatus = fw_refreshBundles(framework, bundles, 1);
            if (refreshStatus != CELIX_SUCCESS) {
                printf("Could not refresh bundle");
            } else {
            	bundle_destroy(bundle);
            }

            framework_releaseGlobalLock(framework);
        }
    }

    return status;
}

celix_status_t fw_refreshBundles(framework_t framework, bundle_t bundles[], int size) {
    celix_status_t status = CELIX_SUCCESS;

    bool locked = framework_acquireGlobalLock(framework);
    if (!locked) {
        printf("Unable to acquire the global lock to install the bundle\n");
        framework_releaseGlobalLock(framework);
        status = CELIX_ILLEGAL_STATE;
    } else {
		hash_map_values_t values;
        bundle_t *newTargets;
        unsigned int nrofvalues;
		bool restart = false;
        hash_map_t map = hashMap_create(NULL, NULL, NULL, NULL);
        int targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            bundle_t bundle = bundles[targetIdx];
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
                bundle_t bundle = (bundle_t) newTargets[i];
                if (framework->bundle == bundle) {
                    restart = true;
                }
            }

            helpers = (struct fw_refreshHelper * )malloc(nrofvalues * sizeof(struct fw_refreshHelper));
            for (i = 0; i < nrofvalues && !restart; i++) {
                bundle_t bundle = (bundle_t) newTargets[i];
                helpers[i].framework = framework;
                helpers[i].bundle = bundle;
                helpers[i].oldState = BUNDLE_INSTALLED;
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

    return status;
}

celix_status_t fw_refreshBundle(framework_t framework, bundle_t bundle) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_state_e state;

    status = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED | BUNDLE_RESOLVED);
    if (status != CELIX_SUCCESS) {
        printf("Cannot refresh bundle");
        framework_releaseBundleLock(framework, bundle);
    } else {
    	bool fire;
		bundle_getState(bundle, &state);
        fire = (state != BUNDLE_INSTALLED);
        bundle_refresh(bundle);

        if (fire) {
            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);
        }

        framework_releaseBundleLock(framework, bundle);
    }
    return status;
}

celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper) {
	bundle_state_e state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == BUNDLE_ACTIVE) {
        refreshHelper->oldState = BUNDLE_ACTIVE;
        fw_stopBundle(refreshHelper->framework, refreshHelper->bundle, false);
    }

    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper) {
	bundle_state_e state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == BUNDLE_UNINSTALLED) {
        bundle_closeAndDelete(refreshHelper->bundle);
        refreshHelper->bundle = NULL;
    } else {
        fw_refreshBundle(refreshHelper->framework, refreshHelper->bundle);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper) {
    if ((refreshHelper->bundle != NULL) && (refreshHelper->oldState == BUNDLE_ACTIVE)) {
        fw_startBundle(refreshHelper->framework, refreshHelper->bundle, 0);
    }
    return CELIX_SUCCESS;
}

celix_status_t fw_getDependentBundles(framework_t framework, bundle_t exporter, array_list_t *list) {
    celix_status_t status = CELIX_SUCCESS;

    if (*list == NULL && exporter != NULL && framework != NULL) {
		array_list_t modules;
		unsigned int modIdx = 0;
		apr_pool_t *pool = NULL;
		bundle_getMemoryPool(exporter, &pool);
        arrayList_create(pool, list);

        modules = bundle_getModules(exporter);
        for (modIdx = 0; modIdx < arrayList_size(modules); modIdx++) {
            module_t module = (module_t) arrayList_get(modules, modIdx);
            array_list_t dependents = module_getDependents(module);
            unsigned int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                module_t dependent = (module_t) arrayList_get(dependents, depIdx);
                arrayList_add(*list, module_getBundle(dependent));
            }
            arrayList_destroy(dependents);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t fw_populateDependentGraph(framework_t framework, bundle_t exporter, hash_map_t *map) {
    celix_status_t status = CELIX_SUCCESS;

    if (exporter != NULL && framework != NULL) {
        array_list_t dependents = NULL;
        if ((status = fw_getDependentBundles(framework, exporter, &dependents)) == CELIX_SUCCESS) {
            unsigned int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                if (!hashMap_containsKey(*map, arrayList_get(dependents, depIdx))) {
                    hashMap_put(*map, arrayList_get(dependents, depIdx), arrayList_get(dependents, depIdx));
                    fw_populateDependentGraph(framework, (bundle_t) arrayList_get(dependents, depIdx), map);
                }
            }
            arrayList_destroy(dependents);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t fw_registerService(framework_t framework, service_registration_t *registration, bundle_t bundle, char * serviceName, void * svcObj, properties_t properties) {
	celix_status_t lock;
	if (serviceName == NULL) {
		printf("Service name cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	} else if (svcObj == NULL) {
		printf("Service object cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	lock = framework_acquireBundleLock(framework, bundle, BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Can only register services while bundle is active or starting");
		framework_releaseBundleLock(framework, bundle);
		return CELIX_ILLEGAL_STATE;
	}
	*registration = serviceRegistry_registerService(framework->registry, bundle, serviceName, svcObj, properties);
	framework_releaseBundleLock(framework, bundle);

	// If this is a listener hook, invoke the callback with all current listeners
	if (strcmp(serviceName, listener_hook_service_name) == 0) {
		unsigned int i;
		array_list_t infos = NULL;
		apr_pool_t *subpool;
		service_reference_t ref = NULL;
		listener_hook_service_t hook;
		apr_pool_t *pool = NULL;

		bundle_getMemoryPool(bundle, &pool);

		arrayList_create(pool, &infos);
		for (i = 0; i > arrayList_size(framework->serviceListeners); i++) {
			fw_service_listener_t listener = (fw_service_listener_t) (framework->serviceListeners, i);
			apr_pool_t *pool;
			bundle_context_t context;
			listener_hook_info_t info;
			bundle_context_t lContext = NULL;

			bundle_getContext(bundle, &context);
			bundleContext_getMemoryPool(context, &pool);
			info = (listener_hook_info_t) (pool, sizeof(*info));

			bundle_getContext(listener->bundle, &lContext);
			info->context = lContext;
			info->removed = false;
			filter_getString(listener->filter, &info->filter);

			arrayList_add(infos, info);
		}

		apr_pool_create(&subpool, pool);

		serviceRegistry_createServiceReference(framework->registry, subpool, *registration, &ref);
		hook = (listener_hook_service_t) (framework, framework->bundle, ref);
		hook->added(hook->handle, infos);
		serviceRegistry_ungetService(framework->registry, framework->bundle, ref);

		apr_pool_destroy(subpool);
	}

	return CELIX_SUCCESS;
}

celix_status_t fw_registerServiceFactory(framework_t framework, service_registration_t *registration, bundle_t bundle, char * serviceName, service_factory_t factory, properties_t properties) {
    celix_status_t lock;
	if (serviceName == NULL) {
        printf("Service name cannot be null");
        return CELIX_ILLEGAL_ARGUMENT;
    } else if (factory == NULL) {
        printf("Service factory cannot be null");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    lock = framework_acquireBundleLock(framework, bundle, BUNDLE_STARTING|BUNDLE_ACTIVE);
    if (lock != CELIX_SUCCESS) {
        printf("Can only register services while bundle is active or starting");
        framework_releaseBundleLock(framework, bundle);
        return CELIX_ILLEGAL_STATE;
    }
    *registration = serviceRegistry_registerServiceFactory(framework->registry, bundle, serviceName, factory, properties);
    framework_releaseBundleLock(framework, bundle);

    return CELIX_SUCCESS;
}

celix_status_t fw_getServiceReferences(framework_t framework, array_list_t *references, bundle_t bundle, const char * serviceName, char * sfilter) {
	filter_t filter = NULL;
	unsigned int refIdx = 0;
	apr_pool_t *pool = NULL;

	bundle_getMemoryPool(bundle, &pool);

	if (sfilter != NULL) {
		filter = filter_create(sfilter, pool);
	}

	serviceRegistry_getServiceReferences(framework->registry, pool, serviceName, filter, references);

	if (filter != NULL) {
		filter_destroy(filter);
	}

	for (refIdx = 0; (*references != NULL) && refIdx < arrayList_size(*references); refIdx++) {
		service_reference_t ref = (service_reference_t) arrayList_get(*references, refIdx);
		service_registration_t reg = NULL;
		char * serviceName;
		properties_t props = NULL;
		serviceReference_getServiceRegistration(ref, &reg);
		serviceRegistration_getProperties(reg, &props);
		serviceName = properties_get(props, (char *) OBJECTCLASS);
		if (!serviceReference_isAssignableTo(ref, bundle, serviceName)) {
			arrayList_remove(*references, refIdx);
			refIdx--;
		}
	}

	return CELIX_SUCCESS;
}

void * fw_getService(framework_t framework, bundle_t bundle, service_reference_t reference) {
	return serviceRegistry_getService(framework->registry, bundle, reference);
}

celix_status_t fw_getBundleRegisteredServices(framework_t framework, apr_pool_t *pool, bundle_t bundle, array_list_t *services) {
	return serviceRegistry_getRegisteredServices(framework->registry, pool, bundle, services);
}

celix_status_t fw_getBundleServicesInUse(framework_t framework, bundle_t bundle, array_list_t *services) {
	celix_status_t status = CELIX_SUCCESS;
	*services = serviceRegistry_getServicesInUse(framework->registry, bundle);
	return status;
}

bool framework_ungetService(framework_t framework, bundle_t bundle, service_reference_t reference) {
	return serviceRegistry_ungetService(framework->registry, bundle, reference);
}

void fw_addServiceListener(framework_t framework, bundle_t bundle, service_listener_t listener, char * sfilter) {
//	apr_pool_t *pool;
//	apr_pool_t *bpool;
//	bundle_context_t context;
//	bundle_getContext(bundle, &context);
//	bundleContext_getMemoryPool(context, &bpool);
//	apr_pool_create(&pool, bpool);
	array_list_t listenerHooks = NULL;
	apr_pool_t *subpool;
	listener_hook_info_t info;
	unsigned int i;
	
	fw_service_listener_t fwListener = (fw_service_listener_t) malloc(sizeof(*fwListener));
	apr_pool_t *pool = NULL;

	bundle_context_t context = NULL;

	bundle_getMemoryPool(bundle, &pool);

	fwListener->bundle = bundle;
	if (sfilter != NULL) {
		filter_t filter = filter_create(sfilter, pool);
		fwListener->filter = filter;
	} else {
		fwListener->filter = NULL;
	}
	fwListener->listener = listener;
	arrayList_add(framework->serviceListeners, fwListener);

	apr_pool_create(&subpool, listener->pool);
	serviceRegistry_getListenerHooks(framework->registry, subpool, &listenerHooks);

	info = (listener_hook_info_t) apr_palloc(subpool, sizeof(*info));

	bundle_getContext(bundle, &context);
	info->context = context;

	info->removed = false;
	info->filter = sfilter == NULL ? NULL : apr_pstrdup(subpool, sfilter);

	for (i = 0; i < arrayList_size(listenerHooks); i++) {
		service_reference_t ref = (service_reference_t) arrayList_get(listenerHooks, i);
		listener_hook_service_t hook = (listener_hook_service_t) fw_getService(framework, framework->bundle, ref);
		array_list_t infos = NULL;
		arrayList_create(subpool, &infos);
		arrayList_add(infos, info);
		hook->added(hook->handle, infos);
		serviceRegistry_ungetService(framework->registry, framework->bundle, ref);
	}

	arrayList_destroy(listenerHooks);
	apr_pool_destroy(subpool);
}

void fw_removeServiceListener(framework_t framework, bundle_t bundle, service_listener_t listener) {
	listener_hook_info_t info = NULL;
	apr_pool_t *pool;
	unsigned int i;
	fw_service_listener_t element;

	bundle_context_t context;
	bundle_getContext(bundle, &context);
	bundleContext_getMemoryPool(context, &pool);
	
	for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
		element = (fw_service_listener_t) arrayList_get(framework->serviceListeners, i);
		if (element->listener == listener && element->bundle == bundle) {
			bundle_context_t lContext = NULL;

			info = (listener_hook_info_t) apr_palloc(pool, sizeof(*info));

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
		array_list_t listenerHooks = NULL;
		serviceRegistry_getListenerHooks(framework->registry, pool, &listenerHooks);
		
		for (i = 0; i < arrayList_size(listenerHooks); i++) {
			service_reference_t ref = (service_reference_t) arrayList_get(listenerHooks, i);
			listener_hook_service_t hook = (listener_hook_service_t) fw_getService(framework, framework->bundle, ref);
			array_list_t infos = NULL;
			apr_pool_t *pool = NULL;

			bundle_getMemoryPool(bundle, &pool);
			arrayList_create(pool, &infos);
			arrayList_add(infos, info);
			hook->removed(hook->handle, infos);
			serviceRegistry_ungetService(framework->registry, framework->bundle, ref);
		}

		arrayList_destroy(listenerHooks);
	}
}

celix_status_t fw_addBundleListener(framework_t framework, bundle_t bundle, bundle_listener_t listener) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *pool = NULL;
	fw_bundle_listener_t bundleListener = NULL;

	apr_pool_create(&pool, framework->mp);
	bundleListener = (fw_bundle_listener_t) apr_palloc(pool, sizeof(*bundleListener));
	if (!bundleListener) {
		status = CELIX_ENOMEM;
	} else {
		bundleListener->listener = listener;
		bundleListener->bundle = bundle;
		bundleListener->pool = pool;

		arrayList_add(framework->bundleListeners, bundleListener);
	}

	return status;
}

celix_status_t fw_removeBundleListener(framework_t framework, bundle_t bundle, bundle_listener_t listener) {
	celix_status_t status = CELIX_SUCCESS;

	unsigned int i;
	fw_bundle_listener_t bundleListener;

	for (i = 0; i < arrayList_size(framework->bundleListeners); i++) {
		bundleListener = (fw_bundle_listener_t) arrayList_get(framework->bundleListeners, i);
		if (bundleListener->listener == listener && bundleListener->bundle == bundle) {
			arrayList_remove(framework->bundleListeners, i);
			apr_pool_destroy(bundleListener->pool);
		}
	}

	return status;
}

void fw_serviceChanged(framework_t framework, service_event_type_e eventType, service_registration_t registration, properties_t oldprops) {
	unsigned int i;
	fw_service_listener_t element;
	if (arrayList_size(framework->serviceListeners) > 0) {
		for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
			int matched = 0;
			properties_t props = NULL;
			bool matchResult = false;

			element = (fw_service_listener_t) arrayList_get(framework->serviceListeners, i);
			serviceRegistration_getProperties(registration, &props);
			if (element->filter != NULL) {
				filter_match(element->filter, props, &matchResult);
			}
			matched = (element->filter == NULL) || matchResult;
			if (matched) {
				service_reference_t reference = NULL;
				service_event_t event;
				apr_pool_t *spool = NULL;
				apr_pool_create(&spool, element->listener->pool);

				event = (service_event_t) apr_palloc(spool, sizeof(*event));

				serviceRegistry_createServiceReference(framework->registry, spool, registration, &reference);

				event->type = eventType;
				event->reference = reference;

				element->listener->serviceChanged(element->listener, event);
			} else if (eventType == SERVICE_EVENT_MODIFIED) {
				bool matchResult = false;
				int matched = 0;
				if (element->filter != NULL) {
					filter_match(element->filter, oldprops, &matchResult);
				}
				matched = (element->filter == NULL) || matchResult;
				if (matched) {
					service_reference_t reference = NULL;
					service_event_t endmatch = (service_event_t) malloc(sizeof(*endmatch));

					serviceRegistry_createServiceReference(framework->registry, element->listener->pool, registration, &reference);

					endmatch->reference = reference;
					endmatch->type = SERVICE_EVENT_MODIFIED_ENDMATCH;
					element->listener->serviceChanged(element->listener, endmatch);
				}
			}
		}
	}
}

//celix_status_t fw_isServiceAssignable(framework_t fw, bundle_t requester, service_reference_t reference, bool *assignable) {
//	celix_status_t status = CELIX_SUCCESS;
//
//	*assignable = true;
//	service_registration_t registration = NULL;
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

celix_status_t getManifest(bundle_archive_t archive, apr_pool_t *pool, MANIFEST *manifest) {
	celix_status_t status = CELIX_SUCCESS;
	char mf[256];
	long refreshCount;
	char *archiveRoot;
	long revisionNumber;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_getArchiveRoot(archive, &archiveRoot);
		if (status == CELIX_SUCCESS) {
			status = bundleArchive_getCurrentRevisionNumber(archive, &revisionNumber);
			if (status == CELIX_SUCCESS) {
				if (status == CELIX_SUCCESS) {
					sprintf(mf, "%s/version%ld.%ld/META-INF/MANIFEST.MF",
							archiveRoot,
							refreshCount,
							revisionNumber
							);
					status = manifest_createFromFile(pool, mf, manifest);
				}
			}
		}
	}
	return status;
}

long framework_getNextBundleId(framework_t framework) {
	long id = framework->nextBundleId;
	framework->nextBundleId++;
	return id;
}

celix_status_t framework_markResolvedModules(framework_t framework, hash_map_t resolvedModuleWireMap) {
	if (resolvedModuleWireMap != NULL) {
		hash_map_iterator_t iterator = hashMapIterator_create(resolvedModuleWireMap);
		while (hashMapIterator_hasNext(iterator)) {
			hash_map_entry_t entry = hashMapIterator_nextEntry(iterator);
			module_t module = (module_t) hashMapEntry_getKey(entry);
			linked_list_t wires = (linked_list_t) hashMapEntry_getValue(entry);

			module_setWires(module, wires);

			module_setResolved(module);
			resolver_moduleResolved(module);
			framework_markBundleResolved(framework, module);
		}
		hashMapIterator_destroy(iterator);
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_markBundleResolved(framework_t framework, module_t module) {
	bundle_t bundle = module_getBundle(module);
	bundle_state_e state;

	if (bundle != NULL) {
		framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_ACTIVE);
		bundle_getState(bundle, &state);
		if (state != BUNDLE_INSTALLED) {
			printf("Trying to resolve a resolved bundle");
		} else {
			framework_setBundleStateAndNotify(framework, bundle, BUNDLE_RESOLVED);
			// framework_fireBundleEvent(BUNDLE_EVENT_RESOLVED, bundle);
		}
		framework_releaseBundleLock(framework, bundle);
	}

	return CELIX_SUCCESS;
}

array_list_t framework_getBundles(framework_t framework) {
	array_list_t bundles = NULL;
	hash_map_iterator_t iterator;
	arrayList_create(framework->mp, &bundles);
	iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		bundle_t bundle = (bundle_t) hashMapIterator_nextValue(iterator);
		arrayList_add(bundles, bundle);
	}
	hashMapIterator_destroy(iterator);
	return bundles;
}

bundle_t framework_getBundle(framework_t framework, char * location) {
	bundle_t bundle = (bundle_t) hashMap_get(framework->installedBundleMap, location);
	return bundle;
}

bundle_t framework_getBundleById(framework_t framework, long id) {
	hash_map_iterator_t iter = hashMapIterator_create(framework->installedBundleMap);
	bundle_t bundle = NULL;
	while (hashMapIterator_hasNext(iter)) {
		bundle_t b = (bundle_t) hashMapIterator_nextValue(iter);
		bundle_archive_t archive = NULL;
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

celix_status_t framework_acquireInstallLock(framework_t framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	while (hashMap_get(framework->installRequestMap, location) != NULL) {
		apr_thread_cond_wait(framework->condition, framework->installRequestLock);
	}
	hashMap_put(framework->installRequestMap, location, location);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_releaseInstallLock(framework_t framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	hashMap_remove(framework->installRequestMap, location);
	apr_thread_cond_broadcast(framework->condition);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_setBundleStateAndNotify(framework_t framework, bundle_t bundle, int state) {
	int ret = CELIX_SUCCESS;

	int err = apr_thread_mutex_lock(framework->bundleLock);
	if (err != 0) {
		celix_log("Failed to lock");
		return CELIX_BUNDLE_EXCEPTION;
	}

	bundle_setState(bundle, state);
	err = apr_thread_cond_broadcast(framework->condition);
	if (err != 0) {
		celix_log("Failed to broadcast");
		ret = CELIX_BUNDLE_EXCEPTION;
	}

	err = apr_thread_mutex_unlock(framework->bundleLock);
	if (err != 0) {
		celix_log("Failed to unlock");
		return CELIX_BUNDLE_EXCEPTION;
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_acquireBundleLock(framework_t framework, bundle_t bundle, int desiredStates) {
	celix_status_t status = CELIX_SUCCESS;

	bool locked;
	apr_os_thread_t lockingThread = 0;

	int err = apr_thread_mutex_lock(framework->bundleLock);
	if (err != APR_SUCCESS) {
		celix_log("Failed to lock");
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

	return CELIX_SUCCESS;
}

bool framework_releaseBundleLock(framework_t framework, bundle_t bundle) {
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

bool framework_acquireGlobalLock(framework_t framework) {
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

celix_status_t framework_releaseGlobalLock(framework_t framework) {
	int ret = CELIX_SUCCESS;
	if (apr_thread_mutex_lock(framework->bundleLock) != 0) {
		celix_log("Error locking framework bundle lock");
		return CELIX_FRAMEWORK_EXCEPTION;
	}

	if (framework->globalLockThread == apr_os_thread_current()) {
		framework->globalLockCount--;
		if (framework->globalLockCount == 0) {
			framework->globalLockThread = 0;
			if (apr_thread_cond_broadcast(framework->condition) != 0) {
				celix_log("Failed to broadcast global lock release.");
				ret = CELIX_FRAMEWORK_EXCEPTION;
				// still need to unlock before returning
			}
		}
	} else {
		printf("The current thread does not own the global lock");
	}

	if (apr_thread_mutex_unlock(framework->bundleLock) != 0) {
		celix_log("Error unlocking framework bundle lock");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	return ret;
}

celix_status_t framework_waitForStop(framework_t framework) {
	if (apr_thread_mutex_lock(framework->mutex) != 0) {
		celix_log("Error locking the framework, shutdown gate not set.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	while (!framework->shutdown) {
		apr_status_t apr_status = apr_thread_cond_wait(framework->shutdownGate, framework->mutex);
		printf("FRAMEWORK: Gate opened\n");
		if (apr_status != 0) {
			celix_log("Error waiting for shutdown gate.");
			return CELIX_FRAMEWORK_EXCEPTION;
		}
	}
	if (apr_thread_mutex_unlock(framework->mutex) != 0) {
		celix_log("Error unlocking the framework.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	printf("FRAMEWORK: Successful shutdown\n");
	return CELIX_SUCCESS;
}

static void *APR_THREAD_FUNC framework_shutdown(apr_thread_t *thd, void *framework) {
//static void * framework_shutdown(void * framework) {
	framework_t fw = (framework_t) framework;
	hash_map_iterator_t iterator;
	int err;

	printf("FRAMEWORK: Shutdown\n");

	iterator = hashMapIterator_create(fw->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		bundle_t bundle = (bundle_t) hashMapIterator_nextValue(iterator);
		bundle_state_e state;
		bundle_getState(bundle, &state);
		if (state == BUNDLE_ACTIVE || state == BUNDLE_STARTING) {
			char *location;
			bundle_archive_t archive = NULL;

			bundle_getArchive(bundle, &archive);
			bundleArchive_getLocation(archive, &location);
			fw_stopBundle(fw, bundle, 0);
		}
	}
	hashMapIterator_destroy(iterator);

	err = apr_thread_mutex_lock(fw->mutex);
	if (err != 0) {
		celix_log("Error locking the framework, cannot exit clean.");
		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	fw->shutdown = true;
	printf("FRAMEWORK: Open gate\n");
	err = apr_thread_cond_broadcast(fw->shutdownGate);
	printf("FRAMEWORK: BC send %d\n", err);
	printf("FRAMEWORK: BC send\n");
	if (err != 0) {
		printf("FRAMEWORK: BC send\n");
		celix_log("Error waking the shutdown gate, cannot exit clean.");
		err = apr_thread_mutex_unlock(fw->mutex);
		if (err != 0) {
			celix_log("Error unlocking the framework, cannot exit clean.");
		}

		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	printf("FRAMEWORK: Unlock\n");
	err = apr_thread_mutex_unlock(fw->mutex);
	if (err != 0) {
		celix_log("Error unlocking the framework, cannot exit clean.");
	}

	printf("FRAMEWORK: Exit thread\n");
	apr_thread_exit(thd, APR_SUCCESS);

	printf("FRAMEWORK: Shutdown done\n");

	return NULL;
}

celix_status_t framework_getMemoryPool(framework_t framework, apr_pool_t **pool) {
	celix_status_t status = CELIX_SUCCESS;

	if (framework != NULL && *pool == NULL) {
		*pool = framework->mp;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t framework_getFrameworkBundle(framework_t framework, bundle_t *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (framework != NULL && *bundle == NULL) {
		*bundle = framework->bundle;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t fw_fireBundleEvent(framework_t framework, bundle_event_type_e eventType, bundle_t bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if ((eventType != BUNDLE_EVENT_STARTING)
			&& (eventType != BUNDLE_EVENT_STOPPING)
			&& (eventType != BUNDLE_EVENT_LAZY_ACTIVATION)) {
		request_t request = (request_t) malloc(sizeof(*request));
		if (!request) {
			status = CELIX_ENOMEM;
		} else {
			request->bundle = bundle;
			request->eventType = eventType;
			request->filter = NULL;
			request->listeners = framework->bundleListeners;
			request->type = BUNDLE_EVENT_TYPE;

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

	return status;
}

static void *APR_THREAD_FUNC fw_eventDispatcher(apr_thread_t *thd, void *fw) {
	framework_t framework = (framework_t) fw;
	request_t request = NULL;

	while (true) {
		int size;
		apr_status_t status;

		if (apr_thread_mutex_lock(framework->dispatcherLock) != 0) {
			celix_log("Error locking the dispatcher");
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
		
		request = (request_t) arrayList_remove(framework->requests, 0);

		if ((status = apr_thread_mutex_unlock(framework->dispatcherLock)) != 0) {
			celix_log("Error unlocking the dispatcher.");
			apr_thread_exit(thd, status);
			return NULL;
		}

		if (request != NULL) {
			int i;
			int size = arrayList_size(request->listeners);
			for (i = 0; i < size; i++) {
				if (request->type == BUNDLE_EVENT_TYPE) {
					fw_bundle_listener_t listener = (fw_bundle_listener_t) (request->listeners, i);
					bundle_event_t event = (bundle_event_t) apr_palloc(listener->listener->pool, sizeof(*event));
					event->bundle = request->bundle;
					event->type = request->type;

					fw_invokeBundleListener(framework, listener->listener, event, listener->bundle);
				}
			}
		}
	}

	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;

}

celix_status_t fw_invokeBundleListener(framework_t framework, bundle_listener_t listener, bundle_event_t event, bundle_t bundle) {
	// We only support async bundle listeners for now
	bundle_state_e state;
	bundle_getState(bundle, &state);
	if (state == BUNDLE_STARTING || state == BUNDLE_ACTIVE) {

		listener->bundleChanged(listener, event);
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_t context) {
	// nothing to do
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_t context) {
    celix_status_t status = CELIX_SUCCESS;

	apr_thread_t *shutdownThread;
	framework_t framework;

	if (bundleContext_getFramework(context, &framework) == CELIX_SUCCESS) {

		printf("FRAMEWORK: Start shutdownthread\n");
	    if (apr_thread_create(&shutdownThread, NULL, framework_shutdown, framework, framework->mp) == APR_SUCCESS) {
            apr_thread_detach(shutdownThread);
	    } else {
            celix_log("Could not create shutdown thread, normal exit not possible.");
	        status = CELIX_FRAMEWORK_EXCEPTION;
	    }
	} else {
		status = CELIX_FRAMEWORK_EXCEPTION;
	}
	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_t context) {
	return CELIX_SUCCESS;
}
