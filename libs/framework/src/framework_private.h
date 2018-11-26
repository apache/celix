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
#include "bundle_cache.h"
#include "celix_log.h"

#include "celix_threads.h"
#include "service_registry.h"

struct celix_framework {
#ifdef WITH_APR
    apr_pool_t *pool;
#endif
    celix_bundle_t *bundle;
    long bundleId; //the bundle id of the framework (normally 0)
    hash_map_pt installRequestMap;

    celix_thread_mutex_t serviceListenersLock;
    array_list_pt serviceListeners;

    array_list_pt frameworkListeners;
    celix_thread_mutex_t frameworkListenersLock;

    array_list_pt bundleListeners;
    celix_thread_mutex_t bundleListenerLock;

    long nextBundleId;
    celix_service_registry_t *registry;
    bundle_cache_pt cache;

    struct {
        celix_thread_mutex_t mutex;
        celix_thread_cond_t cond;
        bool done; //true is shutdown is done
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
        celix_thread_mutex_t mutex; //protect active and requests
        bool active;
        celix_array_list_t *requests;
    } dispatcher;

    framework_logger_pt logger;
};

FRAMEWORK_EXPORT celix_status_t fw_getProperty(framework_pt framework, const char* name, const char* defaultValue, const char** value);

FRAMEWORK_EXPORT celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, const char * location, const char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t framework_getBundleEntry(framework_pt framework, bundle_pt bundle, const char* name, char** entry);

FRAMEWORK_EXPORT celix_status_t fw_startBundle(framework_pt framework, bundle_pt bundle, int options);
FRAMEWORK_EXPORT celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, const char* inputFile);
FRAMEWORK_EXPORT celix_status_t fw_stopBundle(framework_pt framework, bundle_pt bundle, bool record);

FRAMEWORK_EXPORT celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, const char* serviceName, const void* svcObj, properties_pt properties);
FRAMEWORK_EXPORT celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, const char* serviceName, service_factory_pt factory, properties_pt properties);
FRAMEWORK_EXPORT void fw_unregisterService(service_registration_pt registration);

FRAMEWORK_EXPORT celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char* serviceName, const char* filter);
FRAMEWORK_EXPORT celix_status_t framework_ungetServiceReference(framework_pt framework, bundle_pt bundle, service_reference_pt reference);
FRAMEWORK_EXPORT celix_status_t fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, const void** service);
FRAMEWORK_EXPORT celix_status_t framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, bool *result);
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

FRAMEWORK_EXPORT array_list_pt framework_getBundles(framework_pt framework);
FRAMEWORK_EXPORT bundle_pt framework_getBundle(framework_pt framework, const char* location);
FRAMEWORK_EXPORT bundle_pt framework_getBundleById(framework_pt framework, long id);








/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

void celix_framework_useBundles(framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd));
void celix_framework_useBundle(framework_t *fw, bool onlyActive, long bundleId, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd));
service_registration_t* celix_framework_registerServiceFactory(framework_t *fw , const celix_bundle_t *bnd, const char* serviceName, celix_service_factory_t *factory, celix_properties_t *properties);

#endif /* FRAMEWORK_PRIVATE_H_ */
