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
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
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

struct activator {
	void * userData;
	void (*start)(void * userData, BUNDLE_CONTEXT context);
	void (*stop)(void * userData, BUNDLE_CONTEXT context);
	void (*destroy)(void * userData, BUNDLE_CONTEXT context);
};

celix_status_t framework_setBundleStateAndNotify(FRAMEWORK framework, BUNDLE bundle, int state);
celix_status_t framework_markBundleResolved(FRAMEWORK framework, MODULE module);

celix_status_t framework_acquireBundleLock(FRAMEWORK framework, BUNDLE bundle, int desiredStates);
bool framework_releaseBundleLock(FRAMEWORK framework, BUNDLE bundle);

bool framework_acquireGlobalLock(FRAMEWORK framework);
celix_status_t framework_releaseGlobalLock(FRAMEWORK framework);

celix_status_t framework_acquireInstallLock(FRAMEWORK framework, char * location);
celix_status_t framework_releaseInstallLock(FRAMEWORK framework, char * location);

long framework_getNextBundleId(FRAMEWORK framework);

celix_status_t fw_installBundle2(FRAMEWORK framework, BUNDLE * bundle, long id, char * location, BUNDLE_ARCHIVE archive);

celix_status_t fw_refreshBundles(FRAMEWORK framework, BUNDLE bundles[], int size);
celix_status_t fw_refreshBundle(FRAMEWORK framework, BUNDLE bundle);

celix_status_t fw_populateDependentGraph(FRAMEWORK framework, BUNDLE exporter, HASH_MAP *map);

struct fw_refreshHelper {
    FRAMEWORK framework;
    BUNDLE bundle;
    BUNDLE_STATE oldState;
};

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_restart(struct fw_refreshHelper * refreshHelper);
celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper);

struct fw_serviceListener {
	BUNDLE bundle;
	SERVICE_LISTENER listener;
	FILTER filter;
};

typedef struct fw_serviceListener * FW_SERVICE_LISTENER;


celix_status_t framework_create(FRAMEWORK *framework, apr_pool_t *memoryPool) {
    celix_status_t status = CELIX_SUCCESS;

	*framework = (FRAMEWORK) apr_palloc(memoryPool, sizeof(**framework));
	if (*framework == NULL) {
		status = CELIX_ENOMEM;
	} else {
	    apr_status_t apr_status = apr_pool_create(&(*framework)->mp, memoryPool);
        if (apr_status != APR_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
        } else {
            BUNDLE bundle = NULL;
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
                                    (*framework)->bundle = bundle;
                                    (*framework)->bundle->framework = (*framework);

                                    (*framework)->installedBundleMap = NULL;
                                    (*framework)->registry = NULL;

                                    (*framework)->interrupted = false;
                                    (*framework)->shutdown = false;

                                    (*framework)->globalLockWaitersList = arrayList_create();
                                    (*framework)->globalLockCount = 0;
                                    (*framework)->globalLockThread = NULL;
                                    (*framework)->nextBundleId = 1l;
                                    (*framework)->cache = NULL;

                                    (*framework)->installRequestMap = hashMap_create(string_hash, string_hash, string_equals, string_equals);
                                    (*framework)->serviceListeners = NULL;
                                    (*framework)->shutdownGate = NULL;
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

celix_status_t framework_destroy(FRAMEWORK framework) {
    celix_status_t status = CELIX_SUCCESS;

	HASH_MAP_ITERATOR iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
	    HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iterator);
		BUNDLE bundle = hashMapEntry_getValue(entry);
		char * location = hashMapEntry_getKey(entry);

		// for each installed bundle, clean up memory
		LINKED_LIST wires = module_getWires(bundle_getCurrentModule(bundle));
		if (wires != NULL) {
			LINKED_LIST_ITERATOR iter = linkedListIterator_create(wires, 0);
			while (linkedListIterator_hasNext(iter)) {
				WIRE wire = linkedListIterator_next(iter);
				linkedListIterator_remove(iter);
				wire_destroy(wire);
			}
			linkedListIterator_destroy(iter);
		}

		bundleArchive_destroy(bundle->archive);
		bundle_destroy(bundle);
		hashMapIterator_remove(iterator);
	}
	hashMapIterator_destroy(iterator);

	hashMap_destroy(framework->installRequestMap, false, false);

	serviceRegistry_destroy(framework->registry);

	//bundleCache_destroy(framework->cache);

	apr_pool_destroy(framework->mp);

	return status;
}

celix_status_t fw_init(FRAMEWORK framework) {
	celix_status_t status = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	BUNDLE_STATE state;

	if (status != CELIX_SUCCESS) {
		framework_releaseBundleLock(framework, framework->bundle);
		return status;
	}

	bundle_getState(framework->bundle, &state);
	if ((state == BUNDLE_INSTALLED) || (state == BUNDLE_RESOLVED)) {
	    PROPERTIES props = properties_create();
		properties_set(props, (char *) FRAMEWORK_STORAGE, ".cache");
		status = bundleCache_create(props, framework->mp, &framework->cache);
		if (status == CELIX_SUCCESS) {
			BUNDLE_STATE state;
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

	char *location;
	status = bundleArchive_getLocation(bundle_getArchive(framework->bundle), &location);
	hashMap_put(framework->installedBundleMap, location, framework->bundle);

	HASH_MAP wires = resolver_resolve(bundle_getCurrentModule(framework->bundle));
	if (wires == NULL) {
		printf("Unresolved constraints in System Bundle\n");
		framework_releaseBundleLock(framework, framework->bundle);
		return CELIX_BUNDLE_EXCEPTION;
	} else {
		framework_markResolvedModules(framework, wires);
		hashMap_destroy(wires, false, false);
	}

	// reload archives from cache
	ARRAY_LIST archives;
	status = bundleCache_getArchives(framework->cache, &archives);
	if (status != CELIX_SUCCESS) {
	    return status;
	} else {
        int arcIdx;
        for (arcIdx = 0; arcIdx < arrayList_size(archives); arcIdx++) {
            BUNDLE_ARCHIVE archive = (BUNDLE_ARCHIVE) arrayList_get(archives, arcIdx);
            long id;
            bundleArchive_getId(archive, &id);
    		framework->nextBundleId = fmaxl(framework->nextBundleId,  id + 1);
    		BUNDLE_STATE bundleState;

    		bundleArchive_getPersistentState(archive, &bundleState);
            if (bundleState == BUNDLE_UNINSTALLED) {
                bundleArchive_closeAndDelete(archive);
            } else {
                BUNDLE bundle;
                char *location;
				status = bundleArchive_getLocation(archive, &location);
                fw_installBundle2(framework, &bundle, id, location, archive);
            }
        }
        arrayList_destroy(archives);
        framework->registry = serviceRegistry_create(framework, fw_serviceChanged);

        framework_setBundleStateAndNotify(framework, framework->bundle, BUNDLE_STARTING);

        apr_thread_cond_create(&framework->shutdownGate, framework->mp);

        void * handle = dlopen(NULL, RTLD_LAZY|RTLD_LOCAL);
        if (handle == NULL) {
            printf ("%s\n", dlerror());
            framework_releaseBundleLock(framework, framework->bundle);
            return CELIX_START_ERROR;
        }

        BUNDLE_CONTEXT context = NULL;
        if (bundleContext_create(framework, framework->bundle, &context) != CELIX_SUCCESS) {
            return CELIX_START_ERROR;
        }
        bundle_setContext(framework->bundle, context);

        bundle_setHandle(framework->bundle, handle);

        ACTIVATOR activator = (ACTIVATOR) apr_palloc(framework->mp, (sizeof(*activator)));
        if (activator == NULL) {
            status = CELIX_ENOMEM;
        }  else {
            void * (*create)(BUNDLE_CONTEXT context);
            void (*start)(void * handle, BUNDLE_CONTEXT context);
            void (*stop)(void * handle, BUNDLE_CONTEXT context);
            void (*destroy)(void * handle, BUNDLE_CONTEXT context);
            create = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_CREATE);
            start = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_START);
            stop = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_STOP);
            destroy = dlsym(bundle_getHandle(framework->bundle), BUNDLE_ACTIVATOR_DESTROY);
            activator->start = start;
            activator->stop = stop;
            activator->destroy = destroy;
            bundle_setActivator(framework->bundle, activator);

            BUNDLE_CONTEXT context = NULL;
			bundle_getContext(framework->bundle, &context);

            void * userData = NULL;
            if (create != NULL) {
                userData = create(context);
            }
            activator->userData = userData;

            if (start != NULL) {
                start(userData, context);
            }

            framework->serviceListeners = arrayList_create();
            framework_releaseBundleLock(framework, framework->bundle);

            status = CELIX_SUCCESS;
        }
	}
	return status;
}

celix_status_t framework_start(FRAMEWORK framework) {
	celix_status_t lock = framework_acquireBundleLock(framework, framework->bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	BUNDLE_STATE state;

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

void framework_stop(FRAMEWORK framework) {
	fw_stopBundle(framework, framework->bundle, true);
}

celix_status_t fw_installBundle(FRAMEWORK framework, BUNDLE * bundle, char * location) {
	return fw_installBundle2(framework, bundle, -1, location, NULL);
}

celix_status_t fw_installBundle2(FRAMEWORK framework, BUNDLE * bundle, long id, char * location, BUNDLE_ARCHIVE archive) {
    BUNDLE_ARCHIVE bundle_archive = NULL;
    BUNDLE_STATE state;

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

	apr_pool_t *bundlePool;
	apr_pool_create(&bundlePool, framework->mp);
	if (archive == NULL) {
		id = framework_getNextBundleId(framework);
		status = bundleCache_createArchive(framework->cache, id, location, bundlePool, &bundle_archive);
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

	bool locked = framework_acquireGlobalLock(framework);
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

celix_status_t framework_getBundleEntry(FRAMEWORK framework, BUNDLE bundle, char *name, apr_pool_t *pool, char **entry) {
	celix_status_t status = CELIX_SUCCESS;

	BUNDLE_REVISION revision;

	status = bundleArchive_getCurrentRevision(bundle_getArchive(bundle), &revision);
	if (status == CELIX_SUCCESS) {
		if ((strlen(name) > 0) && (name[0] == '/')) {
			name++;
		}

		char *root;
		status = bundleRevision_getRoot(revision, &root);
		if (status == CELIX_SUCCESS) {
			char *e = NULL;
			apr_filepath_merge(&e, root, name, APR_FILEPATH_NOTABOVEROOT, framework->mp);
			apr_finfo_t info;
			apr_status_t ret = apr_stat(&info, e, APR_FINFO_DIRENT|APR_FINFO_TYPE, framework->mp);
			if (ret == APR_ENOENT) {
				(*entry) = NULL;
			} else if (ret == APR_SUCCESS || ret == APR_INCOMPLETE) {
				(*entry) = apr_pstrdup(pool, e);
			}
		}
	}

	return status;
}

celix_status_t fw_startBundle(FRAMEWORK framework, BUNDLE bundle, int options ATTRIBUTE_UNUSED) {
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Unable to start\n");
		framework_releaseBundleLock(framework, bundle);
		return lock;
	}

	HASH_MAP wires;

	void * handle;
	BUNDLE_CONTEXT context = NULL;

	BUNDLE_STATE state;

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
			if (!module_isResolved(bundle_getCurrentModule(bundle))) {
                wires = resolver_resolve(bundle_getCurrentModule(bundle));
                if (wires == NULL) {
                    framework_releaseBundleLock(framework, bundle);
                    return CELIX_BUNDLE_EXCEPTION;
                }
                framework_markResolvedModules(framework, wires);
		    }
			hashMap_destroy(wires, false, false);
			// nb
		case BUNDLE_RESOLVED:
			if (bundleContext_create(framework, bundle, &context) != CELIX_SUCCESS) {
				return CELIX_ENOMEM;
			}
			bundle_setContext(bundle, context);

			MANIFEST manifest = NULL;
			char *library;
			if (getManifest(bundle_getArchive(bundle), &manifest) == CELIX_SUCCESS) {
                bundle_setManifest(bundle, manifest);
                library = manifest_getValue(manifest, HEADER_LIBRARY);
			}

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

			bundleArchive_getRefreshCount(bundle_getArchive(bundle), &refreshCount);
			bundleArchive_getArchiveRoot(bundle_getArchive(bundle), &archiveRoot);
			bundleArchive_getCurrentRevisionNumber(bundle_getArchive(bundle), &revisionNumber);

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
			handle = dlopen(libraryPath, RTLD_LAZY|RTLD_LOCAL);
			if (handle == NULL) {
				printf ("%s\n", dlerror());
				framework_releaseBundleLock(framework, bundle);
				return CELIX_BUNDLE_EXCEPTION;
			}

			bundle_setHandle(bundle, handle);

			ACTIVATOR activator = (ACTIVATOR) apr_palloc(bundle->memoryPool, (sizeof(*activator)));
			if (activator == NULL) {
			    return CELIX_ENOMEM;
			} else {
                void * (*create)(BUNDLE_CONTEXT context, void **userData);
                void (*start)(void * userData, BUNDLE_CONTEXT context);
                void (*stop)(void * userData, BUNDLE_CONTEXT context);
                void (*destroy)(void * userData, BUNDLE_CONTEXT context);
                create = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_CREATE);
                start = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_START);
                stop = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_STOP);
                destroy = dlsym(bundle_getHandle(bundle), BUNDLE_ACTIVATOR_DESTROY);
                activator->start = start;
                activator->stop = stop;
                activator->destroy = destroy;
                bundle_setActivator(bundle, activator);

                framework_setBundleStateAndNotify(framework, bundle, BUNDLE_STARTING);

                void * userData = NULL;
                BUNDLE_CONTEXT context;

                bundle_getContext(bundle, &context);

                if (create != NULL) {
                    create(context, &userData);
                }
                activator->userData = userData;

                if (start != NULL) {
                    start(userData, context);
                }

                framework_setBundleStateAndNotify(framework, bundle, BUNDLE_ACTIVE);
			}

			break;
	}

	framework_releaseBundleLock(framework, bundle);
	return CELIX_SUCCESS;
}

celix_status_t framework_updateBundle(FRAMEWORK framework, BUNDLE bundle, char *inputFile) {
	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Cannot update bundle, in wrong state");
		framework_releaseBundleLock(framework, bundle);
		return lock;
	}

	BUNDLE_STATE oldState;
	bundle_getState(bundle, &oldState);
	char *location;
	bundleArchive_getLocation(bundle_getArchive(bundle), &location);

	if (oldState == BUNDLE_ACTIVE) {
		fw_stopBundle(framework, bundle, false);
	}

	bool locked = framework_acquireGlobalLock(framework);
	if (!locked) {
		printf("Unable to acquire the global lock to update the bundle\n");
		framework_releaseInstallLock(framework, location);
		return CELIX_BUNDLE_EXCEPTION;
	}

	bundle_revise(bundle, location, inputFile);
	framework_releaseGlobalLock(framework);

	celix_status_t status = bundleArchive_setLastModified(bundle_getArchive(bundle), time(NULL));
	framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);

	// Refresh packages?

	if (oldState == BUNDLE_ACTIVE) {
		fw_startBundle(framework, bundle, 1);
	}
	framework_releaseBundleLock(framework, bundle);

	return CELIX_SUCCESS;
}

celix_status_t fw_stopBundle(FRAMEWORK framework, BUNDLE bundle, bool record) {
	celix_status_t status = CELIX_SUCCESS;

	status = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED|BUNDLE_RESOLVED|BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (status != CELIX_SUCCESS) {
		printf("Cannot stop bundle");
		framework_releaseBundleLock(framework, bundle);
	} else {

		if (record) {
			bundle_setPersistentStateInactive(bundle);
		}

		//if (!fw_isBundlePersistentlyStarted(framework, bundle)) {
		//}

		BUNDLE_STATE state;
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

		ACTIVATOR activator = bundle_getActivator(bundle);
		BUNDLE_CONTEXT context;
		bundle_getContext(bundle, &context);
		if (activator->stop != NULL) {
			activator->stop(activator->userData, context);
		}

		if (activator->destroy != NULL) {
			activator->destroy(activator->userData, context);
		}

		if (strcmp(module_getId(bundle_getCurrentModule(bundle)), "0") != 0) {
			activator->start = NULL;
			activator->stop = NULL;
			activator->userData = NULL;
			//free(activator);
			bundle_setActivator(bundle, NULL);

			serviceRegistry_unregisterServices(framework->registry, bundle);
			serviceRegistry_ungetServices(framework->registry, bundle);

			dlclose(bundle_getHandle(bundle));
		}

		bundleContext_destroy(context);
		bundle_setContext(bundle, NULL);

		MANIFEST manifest = NULL;
		bundle_getManifest(bundle, &manifest);

		manifest_destroy(manifest);

		framework_setBundleStateAndNotify(framework, bundle, BUNDLE_RESOLVED);

		framework_releaseBundleLock(framework, bundle);
	}

	return status;
}

celix_status_t fw_uninstallBundle(FRAMEWORK framework, BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;

    BUNDLE_STATE state;

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
        fw_stopBundle(framework, bundle, true);

        bool locked = framework_acquireGlobalLock(framework);
        if (!locked) {
            printf("Unable to acquire the global lock to install the bundle\n");
            framework_releaseGlobalLock(framework);
            status = CELIX_ILLEGAL_STATE;
        } else {
            BUNDLE_ARCHIVE archive = bundle_getArchive(bundle);
            char * location;
            status = bundleArchive_getLocation(archive, &location);
            // TODO sync issues?
            BUNDLE target = (BUNDLE) hashMap_remove(framework->installedBundleMap, location);

            if (target != NULL) {
                bundle_setPersistentStateUninstalled(target);
                // fw_rememberUninstalledBundle(framework, target);
            }

            framework_releaseGlobalLock(framework);

            if (target == NULL) {
                printf("Could not remove bundle from installed map");
            }

            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);
            // TODO: fw_fireBundleEvent(framework BUNDLE_EVENT_UNRESOLVED, bundle);

            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_UNINSTALLED);
            status = bundleArchive_setLastModified(archive, time(NULL));
        }
        framework_releaseBundleLock(framework, bundle);

        // TODO: fw_fireBundleEvent(framework BUNDLE_EVENT_UNINSTALLED, bundle);

        locked = framework_acquireGlobalLock(framework);
        if (locked) {
            BUNDLE bundles[] = { bundle };
            celix_status_t refreshStatus = fw_refreshBundles(framework, bundles, 1);
            if (refreshStatus != CELIX_SUCCESS) {
                printf("Could not refresh bundle");
            }

            framework_releaseGlobalLock(framework);
        }
    }

    return status;
}

celix_status_t fw_refreshBundles(FRAMEWORK framework, BUNDLE bundles[], int size) {
    celix_status_t status = CELIX_SUCCESS;

    bool locked = framework_acquireGlobalLock(framework);
    if (!locked) {
        printf("Unable to acquire the global lock to install the bundle\n");
        framework_releaseGlobalLock(framework);
        status = CELIX_ILLEGAL_STATE;
    } else {
        HASH_MAP map = hashMap_create(NULL, NULL, NULL, NULL);
        int targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            BUNDLE bundle = bundles[targetIdx];
            hashMap_put(map, bundle, bundle);
            fw_populateDependentGraph(framework, bundle, &map);
        }
        HASH_MAP_VALUES values = hashMapValues_create(map);
        BUNDLE *newTargets;
        int nrofvalues;
        hashMapValues_toArray(values, (void *) &newTargets, &nrofvalues);
    
        bool restart = false;
        if (newTargets != NULL) {
            int i = 0;
            for (i = 0; i < nrofvalues && !restart; i++) {
                BUNDLE bundle = (BUNDLE) newTargets[i];
                if (framework->bundle == bundle) {
                    restart = true;
                }
            }

            struct fw_refreshHelper * helpers[nrofvalues];
            for (i = 0; i < nrofvalues && !restart; i++) {
                BUNDLE bundle = (BUNDLE) newTargets[i];
                helpers[i] = malloc(sizeof(struct fw_refreshHelper));
                helpers[i] ->framework = framework;
                helpers[i]->bundle = bundle;
                helpers[i]->oldState = BUNDLE_INSTALLED;
            }

            for (i = 0; i < nrofvalues; i++) {
                struct fw_refreshHelper * helper = helpers[i];
                fw_refreshHelper_stop(helper);
                fw_refreshHelper_refreshOrRemove(helper);
            }

            for (i = 0; i < nrofvalues; i++) {
                struct fw_refreshHelper * helper = helpers[i];
                fw_refreshHelper_restart(helper);
            }

            if (restart) {
                bundle_update(framework->bundle, NULL);
            }
        }

        framework_releaseGlobalLock(framework);
    }

    return status;
}

celix_status_t fw_refreshBundle(FRAMEWORK framework, BUNDLE bundle) {
    celix_status_t status = CELIX_SUCCESS;
    BUNDLE_STATE state;

    status = framework_acquireBundleLock(framework, bundle, BUNDLE_INSTALLED | BUNDLE_RESOLVED);
    if (status != CELIX_SUCCESS) {
        printf("Cannot refresh bundle");
        framework_releaseBundleLock(framework, bundle);
    } else {
    	bundle_getState(bundle, &state);
        bool fire = (state != BUNDLE_INSTALLED);
        bundle_refresh(bundle);

        if (fire) {
            framework_setBundleStateAndNotify(framework, bundle, BUNDLE_INSTALLED);
        }

        framework_releaseBundleLock(framework, bundle);
    }
    return status;
}

celix_status_t fw_refreshHelper_stop(struct fw_refreshHelper * refreshHelper) {
	BUNDLE_STATE state;
	bundle_getState(refreshHelper->bundle, &state);
    if (state == BUNDLE_ACTIVE) {
        refreshHelper->oldState = BUNDLE_ACTIVE;
        fw_stopBundle(refreshHelper->framework, refreshHelper->bundle, false);
    }

    return CELIX_SUCCESS;
}

celix_status_t fw_refreshHelper_refreshOrRemove(struct fw_refreshHelper * refreshHelper) {
	BUNDLE_STATE state;
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

celix_status_t fw_getDependentBundles(FRAMEWORK framework, BUNDLE exporter, ARRAY_LIST *list) {
    celix_status_t status = CELIX_SUCCESS;

    if (*list == NULL && exporter != NULL && framework != NULL) {
        *list = arrayList_create();

        ARRAY_LIST modules = bundle_getModules(exporter);
        int modIdx = 0;
        for (modIdx = 0; modIdx < arrayList_size(modules); modIdx++) {
            MODULE module = arrayList_get(modules, modIdx);
            ARRAY_LIST dependents = module_getDependents(module);
            int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                MODULE dependent = arrayList_get(dependents, depIdx);
                arrayList_add(*list, module_getBundle(dependent));
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t fw_populateDependentGraph(FRAMEWORK framework, BUNDLE exporter, HASH_MAP *map) {
    celix_status_t status = CELIX_SUCCESS;

    if (exporter != NULL && framework != NULL) {
        ARRAY_LIST dependents = NULL;
        if ((status = fw_getDependentBundles(framework, exporter, &dependents)) == CELIX_SUCCESS) {
            int depIdx = 0;
            for (depIdx = 0; (dependents != NULL) && (depIdx < arrayList_size(dependents)); depIdx++) {
                if (!hashMap_containsKey(*map, arrayList_get(dependents, depIdx))) {
                    hashMap_put(*map, arrayList_get(dependents, depIdx), arrayList_get(dependents, depIdx));
                    fw_populateDependentGraph(framework, arrayList_get(dependents, depIdx), map);
                }
            }
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t fw_registerService(FRAMEWORK framework, SERVICE_REGISTRATION *registration, BUNDLE bundle, char * serviceName, void * svcObj, PROPERTIES properties) {
	if (serviceName == NULL) {
		printf("Service name cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	} else if (svcObj == NULL) {
		printf("Service object cannot be null");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_STARTING|BUNDLE_ACTIVE);
	if (lock != CELIX_SUCCESS) {
		printf("Can only register services while bundle is active or starting");
		framework_releaseBundleLock(framework, bundle);
		return CELIX_ILLEGAL_STATE;
	}
	*registration = serviceRegistry_registerService(framework->registry, bundle, serviceName, svcObj, properties);
	framework_releaseBundleLock(framework, bundle);

	return CELIX_SUCCESS;
}

celix_status_t fw_registerServiceFactory(FRAMEWORK framework, SERVICE_REGISTRATION *registration, BUNDLE bundle, char * serviceName, service_factory_t factory, PROPERTIES properties) {
    if (serviceName == NULL) {
        printf("Service name cannot be null");
        return CELIX_ILLEGAL_ARGUMENT;
    } else if (factory == NULL) {
        printf("Service factory cannot be null");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t lock = framework_acquireBundleLock(framework, bundle, BUNDLE_STARTING|BUNDLE_ACTIVE);
    if (lock != CELIX_SUCCESS) {
        printf("Can only register services while bundle is active or starting");
        framework_releaseBundleLock(framework, bundle);
        return CELIX_ILLEGAL_STATE;
    }
    *registration = serviceRegistry_registerServiceFactory(framework->registry, bundle, serviceName, factory, properties);
    framework_releaseBundleLock(framework, bundle);

    return CELIX_SUCCESS;
}

celix_status_t fw_getServiceReferences(FRAMEWORK framework, ARRAY_LIST *references, BUNDLE bundle ATTRIBUTE_UNUSED, char * serviceName, char * sfilter) {
	FILTER filter = NULL;
	if (sfilter != NULL) {
		filter = filter_create(sfilter);
	}

	*references = serviceRegistry_getServiceReferences(framework->registry, serviceName, filter);

	if (filter != NULL) {
		filter_destroy(filter);
	}

	int refIdx = 0;
	for (refIdx = 0; (*references != NULL) && refIdx < arrayList_size(*references); refIdx++) {
		SERVICE_REFERENCE ref = (SERVICE_REFERENCE) arrayList_get(*references, refIdx);
		SERVICE_REGISTRATION reg = ref->registration;
		char * serviceName = properties_get(reg->properties, (char *) OBJECTCLASS);
		if (!serviceReference_isAssignableTo(ref, bundle, serviceName)) {
			arrayList_remove(*references, refIdx);
			refIdx--;
		}
	}

	return CELIX_SUCCESS;
}

void * fw_getService(FRAMEWORK framework, BUNDLE bundle ATTRIBUTE_UNUSED, SERVICE_REFERENCE reference) {
	return serviceRegistry_getService(framework->registry, bundle, reference);
}

bool framework_ungetService(FRAMEWORK framework, BUNDLE bundle ATTRIBUTE_UNUSED, SERVICE_REFERENCE reference) {
	return serviceRegistry_ungetService(framework->registry, bundle, reference);
}

void fw_addServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener, char * sfilter) {
	FW_SERVICE_LISTENER fwListener = (FW_SERVICE_LISTENER) malloc(sizeof(*fwListener));
	fwListener->bundle = bundle;
	if (sfilter != NULL) {
		FILTER filter = filter_create(sfilter);
		fwListener->filter = filter;
	}
	fwListener->listener = listener;
	arrayList_add(framework->serviceListeners, fwListener);
}

void fw_removeServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener) {
	unsigned int i;
	FW_SERVICE_LISTENER element;
	for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
		element = (FW_SERVICE_LISTENER) arrayList_get(framework->serviceListeners, i);
		if (element->listener == listener && element->bundle == bundle) {
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
}

void fw_serviceChanged(FRAMEWORK framework, SERVICE_EVENT event, PROPERTIES oldprops) {
	unsigned int i;
	FW_SERVICE_LISTENER element;
	SERVICE_REGISTRATION registration = event->reference->registration;
	if (arrayList_size(framework->serviceListeners) > 0) {
		for (i = 0; i < arrayList_size(framework->serviceListeners); i++) {
			int matched = 0;
			element = (FW_SERVICE_LISTENER) arrayList_get(framework->serviceListeners, i);
			matched = (element->filter == NULL) || filter_match(element->filter, registration->properties);
			if (matched) {
				bool assignable;
				fw_isServiceAssignable(framework, element->bundle, event->reference, &assignable);
				if (assignable) {
					element->listener->serviceChanged(element->listener, event);
				}
			} else if (event->type == MODIFIED) {
				int matched = (element->filter == NULL) || filter_match(element->filter, oldprops);
				if (matched) {
					SERVICE_EVENT endmatch = (SERVICE_EVENT) malloc(sizeof(*endmatch));
					endmatch->reference = event->reference;
					endmatch->type = MODIFIED_ENDMATCH;
					element->listener->serviceChanged(element->listener, endmatch);
				}
			}
		}
	}
}

celix_status_t fw_isServiceAssignable(FRAMEWORK fw, BUNDLE requester, SERVICE_REFERENCE reference, bool *assignable) {
	celix_status_t status = CELIX_SUCCESS;

	*assignable = true;
	char *serviceName = properties_get(reference->registration->properties, (char *) OBJECTCLASS);
	if (!serviceReference_isAssignableTo(reference, requester, serviceName)) {
		*assignable = false;
	}

	return status;
}

celix_status_t getManifest(BUNDLE_ARCHIVE archive, MANIFEST *manifest) {
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
					sprintf(mf, "%s/version%ld.%ld/MANIFEST/MANIFEST.MF",
							archiveRoot,
							refreshCount,
							revisionNumber
							);
					status = manifest_read(mf, manifest);
				}
			}
		}
	}
	return status;
}

long framework_getNextBundleId(FRAMEWORK framework) {
	long id = framework->nextBundleId;
	framework->nextBundleId++;
	return id;
}

celix_status_t framework_markResolvedModules(FRAMEWORK framework, HASH_MAP resolvedModuleWireMap) {
	if (resolvedModuleWireMap != NULL) {
		HASH_MAP_ITERATOR iterator = hashMapIterator_create(resolvedModuleWireMap);
		while (hashMapIterator_hasNext(iterator)) {
			HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iterator);
			MODULE module = (MODULE) hashMapEntry_getKey(entry);
			LINKED_LIST wires = (LINKED_LIST) hashMapEntry_getValue(entry);

			module_setWires(module, wires);

			module_setResolved(module);
			resolver_moduleResolved(module);
			framework_markBundleResolved(framework, module);
		}
		hashMapIterator_destroy(iterator);
	}
	return CELIX_SUCCESS;
}

celix_status_t framework_markBundleResolved(FRAMEWORK framework, MODULE module) {
	BUNDLE bundle = module_getBundle(module);
	BUNDLE_STATE state;

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

ARRAY_LIST framework_getBundles(FRAMEWORK framework) {
	ARRAY_LIST bundles = arrayList_create();
	HASH_MAP_ITERATOR iterator = hashMapIterator_create(framework->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		BUNDLE bundle = hashMapIterator_nextValue(iterator);
		arrayList_add(bundles, bundle);
	}
	hashMapIterator_destroy(iterator);
	return bundles;
}

BUNDLE framework_getBundle(FRAMEWORK framework, char * location) {
	BUNDLE bundle = hashMap_get(framework->installedBundleMap, location);
	return bundle;
}

BUNDLE framework_getBundleById(FRAMEWORK framework, long id) {
	HASH_MAP_ITERATOR iter = hashMapIterator_create(framework->installedBundleMap);
	BUNDLE bundle = NULL;
	while (hashMapIterator_hasNext(iter)) {
		BUNDLE b = hashMapIterator_nextValue(iter);
		long bid;
		bundleArchive_getId(bundle_getArchive(b), &bid);
		if (bid == id) {
			bundle = b;
			break;
		}
	}
	hashMapIterator_destroy(iter);
	return bundle;
}

celix_status_t framework_acquireInstallLock(FRAMEWORK framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	while (hashMap_get(framework->installRequestMap, location) != NULL) {
		apr_thread_cond_wait(framework->condition, framework->installRequestLock);
	}
	hashMap_put(framework->installRequestMap, location, location);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_releaseInstallLock(FRAMEWORK framework, char * location) {
    apr_thread_mutex_lock(framework->installRequestLock);

	hashMap_remove(framework->installRequestMap, location);
	apr_thread_cond_broadcast(framework->condition);

	apr_thread_mutex_unlock(framework->installRequestLock);

	return CELIX_SUCCESS;
}

celix_status_t framework_setBundleStateAndNotify(FRAMEWORK framework, BUNDLE bundle, int state) {
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

celix_status_t framework_acquireBundleLock(FRAMEWORK framework, BUNDLE bundle, int desiredStates) {
	celix_status_t status = CELIX_SUCCESS;

	bool locked;
	apr_os_thread_t lockingThread = NULL;

	int err = apr_thread_mutex_lock(framework->bundleLock);
	if (err != APR_SUCCESS) {
		celix_log("Failed to lock");
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		bool lockable = false;
		bundle_isLockable(bundle, &lockable);
		while (!lockable
				|| ((framework->globalLockThread != NULL)
				&& (framework->globalLockThread != pthread_self()))) {
			BUNDLE_STATE state;
			bundle_getState(bundle, &state);
			if ((desiredStates & state) == 0) {
				status = CELIX_ILLEGAL_STATE;
				break;
			} else
				bundle_getLockingThread(bundle, &lockingThread);
				if (framework->globalLockThread == pthread_self()
					&& (lockingThread != NULL)
					&& arrayList_contains(framework->globalLockWaitersList, lockingThread)) {
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
			BUNDLE_STATE state;
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

bool framework_releaseBundleLock(FRAMEWORK framework, BUNDLE bundle) {
    bool unlocked;
    apr_os_thread_t lockingThread = NULL;

    apr_thread_mutex_lock(framework->bundleLock);

    bundle_unlock(bundle, &unlocked);
	if (!unlocked) {
	    apr_thread_mutex_unlock(framework->bundleLock);
		return false;
	}
	bundle_getLockingThread(bundle, &lockingThread);
	if (lockingThread == NULL) {
	    apr_thread_cond_broadcast(framework->condition);
	}

	apr_thread_mutex_unlock(framework->bundleLock);

	return true;
}

bool framework_acquireGlobalLock(FRAMEWORK framework) {
    apr_thread_mutex_lock(framework->bundleLock);

	bool interrupted = false;

	while (!interrupted
			&& (framework->globalLockThread != NULL)
			&& (framework->globalLockThread != pthread_self())) {
		pthread_t currentThread = pthread_self();
		arrayList_add(framework->globalLockWaitersList, currentThread);
		apr_thread_cond_broadcast(framework->condition);

		apr_thread_cond_wait(framework->condition, framework->bundleLock);
		if (framework->interrupted) {
			interrupted = true;
			framework->interrupted = false;
		}

		arrayList_removeElement(framework->globalLockWaitersList, currentThread);
	}

	if (!interrupted) {
		framework->globalLockCount++;
		framework->globalLockThread = pthread_self();
	}

	apr_thread_mutex_unlock(framework->bundleLock);

	return !interrupted;
}

celix_status_t framework_releaseGlobalLock(FRAMEWORK framework) {
	int ret = CELIX_SUCCESS;
	if (apr_thread_mutex_lock(framework->bundleLock) != 0) {
		celix_log("Error locking framework bundle lock");
		return CELIX_FRAMEWORK_EXCEPTION;
	}

	if (framework->globalLockThread == pthread_self()) {
		framework->globalLockCount--;
		if (framework->globalLockCount == 0) {
			framework->globalLockThread = NULL;
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

celix_status_t framework_waitForStop(FRAMEWORK framework) {
	if (apr_thread_mutex_lock(framework->mutex) != 0) {
		celix_log("Error locking the framework, shutdown gate not set.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	while (!framework->shutdown) {
		apr_status_t apr_status = apr_thread_cond_wait(framework->shutdownGate, framework->mutex);
		if (apr_status != 0) {
			celix_log("Error waiting for shutdown gate.");
			return CELIX_FRAMEWORK_EXCEPTION;
		}
		printf("Interrupted %d\n", framework->shutdown);
	}
	printf("waited for stop\n");
	if (apr_thread_mutex_unlock(framework->mutex) != 0) {
		celix_log("Error unlocking the framework.");
		return CELIX_FRAMEWORK_EXCEPTION;
	}
	printf("waited for stop finish\n");
	return CELIX_SUCCESS;
}

static void *APR_THREAD_FUNC framework_shutdown(apr_thread_t *thd, void *framework) {
//static void * framework_shutdown(void * framework) {
	FRAMEWORK fw = (FRAMEWORK) framework;

	HASH_MAP_ITERATOR iterator = hashMapIterator_create(fw->installedBundleMap);
	while (hashMapIterator_hasNext(iterator)) {
		BUNDLE bundle = hashMapIterator_nextValue(iterator);
		BUNDLE_STATE state;
		bundle_getState(bundle, &state);
		if (state == BUNDLE_ACTIVE || state == BUNDLE_STARTING) {
			char *location;
			bundleArchive_getLocation(bundle_getArchive(bundle), &location);
		    printf("stop bundle: %s\n", location);
			fw_stopBundle(fw, bundle, 0);
		}
	}
	hashMapIterator_destroy(iterator);

	int err = apr_thread_mutex_lock(fw->mutex);
	if (err != 0) {
		celix_log("Error locking the framework, cannot exit clean.");
		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	fw->shutdown = true;
	err = apr_thread_cond_broadcast(fw->shutdownGate);
	if (err != 0) {
		celix_log("Error waking the shutdown gate, cannot exit clean.");
		err = apr_thread_mutex_unlock(fw->mutex);
		if (err != 0) {
			celix_log("Error unlocking the framework, cannot exit clean.");
		}

		apr_thread_exit(thd, APR_ENOLOCK);
		return NULL;
	}
	err = apr_thread_mutex_unlock(fw->mutex);
	if (err != 0) {
		celix_log("Error unlocking the framework, cannot exit clean.");
	}

	apr_thread_exit(thd, APR_SUCCESS);

	return NULL;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	// nothing to do
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;

	apr_thread_t *shutdownThread;
	FRAMEWORK framework;

	if (bundleContext_getFramework(context, &framework) == CELIX_SUCCESS) {

	    if (apr_thread_create(&shutdownThread, NULL, framework_shutdown, framework, framework->mp) == APR_SUCCESS) {
            //int err = pthread_create(&shutdownThread, NULL, framework_shutdown, framework);
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

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
