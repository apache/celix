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
 * framework.h
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

typedef struct activator * activator_pt;
typedef struct framework * framework_pt;

#include "manifest.h"
#include "wire.h"
#include "hash_map.h"
#include "array_list.h"
#include "celix_errno.h"
#include "service_factory.h"
#include "bundle_archive.h"
#include "service_listener.h"
#include "bundle_listener.h"
#include "service_registration.h"
#include "bundle_context.h"
#include "framework_exports.h"

FRAMEWORK_EXPORT celix_status_t framework_create(framework_pt *framework, apr_pool_t *memoryPool, properties_pt config);
FRAMEWORK_EXPORT celix_status_t framework_destroy(framework_pt framework);

FRAMEWORK_EXPORT celix_status_t fw_init(framework_pt framework);
FRAMEWORK_EXPORT celix_status_t framework_start(framework_pt framework);
FRAMEWORK_EXPORT void framework_stop(framework_pt framework);

FRAMEWORK_EXPORT celix_status_t fw_getProperty(framework_pt framework, const char *name, char **value);

FRAMEWORK_EXPORT celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle);

FRAMEWORK_EXPORT celix_status_t framework_getBundleEntry(framework_pt framework, bundle_pt bundle, char *name, apr_pool_t *pool, char **entry);

FRAMEWORK_EXPORT celix_status_t fw_startBundle(framework_pt framework, bundle_pt bundle, int options);
FRAMEWORK_EXPORT celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_stopBundle(framework_pt framework, bundle_pt bundle, bool record);

FRAMEWORK_EXPORT celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, char * serviceName, void * svcObj, properties_pt properties);
FRAMEWORK_EXPORT celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, char * serviceName, service_factory_pt factory, properties_pt properties);
FRAMEWORK_EXPORT void fw_unregisterService(service_registration_pt registration);

FRAMEWORK_EXPORT celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char * serviceName, char * filter);
FRAMEWORK_EXPORT void * fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference);
FRAMEWORK_EXPORT bool framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference);
FRAMEWORK_EXPORT celix_status_t fw_getBundleRegisteredServices(framework_pt framework, apr_pool_t *pool, bundle_pt bundle, array_list_pt *services);
FRAMEWORK_EXPORT celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services);

FRAMEWORK_EXPORT void fw_addServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener, char * filter);
FRAMEWORK_EXPORT void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);
FRAMEWORK_EXPORT celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener);

FRAMEWORK_EXPORT void fw_serviceChanged(framework_pt framework, service_event_type_e eventType, service_registration_pt registration, properties_pt oldprops);

FRAMEWORK_EXPORT celix_status_t fw_isServiceAssignable(framework_pt fw, bundle_pt requester, service_reference_pt reference, bool *assignable);

//bundle_archive_pt fw_createArchive(long id, char * location);
//void revise(bundle_archive_pt archive, char * location);
FRAMEWORK_EXPORT celix_status_t getManifest(bundle_archive_pt archive, apr_pool_t *pool, manifest_pt *manifest);

FRAMEWORK_EXPORT bundle_pt findBundle(bundle_context_pt context);
FRAMEWORK_EXPORT service_registration_pt findRegistration(service_reference_pt reference);

FRAMEWORK_EXPORT service_reference_pt listToArray(array_list_pt list);
FRAMEWORK_EXPORT celix_status_t framework_markResolvedModules(framework_pt framework, hash_map_pt wires);

FRAMEWORK_EXPORT celix_status_t framework_waitForStop(framework_pt framework);

FRAMEWORK_EXPORT array_list_pt framework_getBundles(framework_pt framework);
FRAMEWORK_EXPORT bundle_pt framework_getBundle(framework_pt framework, char * location);
FRAMEWORK_EXPORT bundle_pt framework_getBundleById(framework_pt framework, long id);

FRAMEWORK_EXPORT celix_status_t framework_getMemoryPool(framework_pt framework, apr_pool_t **pool);
FRAMEWORK_EXPORT celix_status_t framework_getFrameworkBundle(framework_pt framework, bundle_pt *bundle);

#endif /* FRAMEWORK_H_ */
