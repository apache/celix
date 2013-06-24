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
 * framework_private.h
 *
 *  \date       May 22, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef FRAMEWORK_PRIVATE_H_
#define FRAMEWORK_PRIVATE_H_

#include "framework.h"

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

celix_status_t framework_start(framework_t framework);
void framework_stop(framework_t framework);

FRAMEWORK_EXPORT celix_status_t fw_getProperty(framework_t framework, const char *name, char **value);

FRAMEWORK_EXPORT celix_status_t fw_installBundle(framework_t framework, bundle_t * bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_uninstallBundle(framework_t framework, bundle_t bundle);

FRAMEWORK_EXPORT celix_status_t framework_getBundleEntry(framework_t framework, bundle_t bundle, char *name, apr_pool_t *pool, char **entry);

FRAMEWORK_EXPORT celix_status_t fw_startBundle(framework_t framework, bundle_t bundle, int options);
FRAMEWORK_EXPORT celix_status_t framework_updateBundle(framework_t framework, bundle_t bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_stopBundle(framework_t framework, bundle_t bundle, bool record);

FRAMEWORK_EXPORT celix_status_t fw_registerService(framework_t framework, service_registration_t * registration, bundle_t bundle, char * serviceName, void * svcObj, properties_t properties);
FRAMEWORK_EXPORT celix_status_t fw_registerServiceFactory(framework_t framework, service_registration_t * registration, bundle_t bundle, char * serviceName, service_factory_t factory, properties_t properties);
FRAMEWORK_EXPORT void fw_unregisterService(service_registration_t registration);

FRAMEWORK_EXPORT celix_status_t fw_getServiceReferences(framework_t framework, array_list_t *references, bundle_t bundle, const char * serviceName, char * filter);
FRAMEWORK_EXPORT void * fw_getService(framework_t framework, bundle_t bundle, service_reference_t reference);
FRAMEWORK_EXPORT bool framework_ungetService(framework_t framework, bundle_t bundle, service_reference_t reference);
FRAMEWORK_EXPORT celix_status_t fw_getBundleRegisteredServices(framework_t framework, apr_pool_t *pool, bundle_t bundle, array_list_t *services);
FRAMEWORK_EXPORT celix_status_t fw_getBundleServicesInUse(framework_t framework, bundle_t bundle, array_list_t *services);

FRAMEWORK_EXPORT void fw_addServiceListener(framework_t framework, bundle_t bundle, service_listener_t listener, char * filter);
FRAMEWORK_EXPORT void fw_removeServiceListener(framework_t framework, bundle_t bundle, service_listener_t listener);

FRAMEWORK_EXPORT celix_status_t fw_addBundleListener(framework_t framework, bundle_t bundle, bundle_listener_t listener);
FRAMEWORK_EXPORT celix_status_t fw_removeBundleListener(framework_t framework, bundle_t bundle, bundle_listener_t listener);

FRAMEWORK_EXPORT void fw_serviceChanged(framework_t framework, service_event_type_e eventType, service_registration_t registration, properties_t oldprops);

FRAMEWORK_EXPORT celix_status_t fw_isServiceAssignable(framework_t fw, bundle_t requester, service_reference_t reference, bool *assignable);

//bundle_archive_t fw_createArchive(long id, char * location);
//void revise(bundle_archive_t archive, char * location);
FRAMEWORK_EXPORT celix_status_t getManifest(bundle_archive_t archive, apr_pool_t *pool, MANIFEST *manifest);

FRAMEWORK_EXPORT bundle_t findBundle(bundle_context_t context);
FRAMEWORK_EXPORT service_registration_t findRegistration(service_reference_t reference);

FRAMEWORK_EXPORT service_reference_t listToArray(array_list_t list);
FRAMEWORK_EXPORT celix_status_t framework_markResolvedModules(framework_t framework, hash_map_t wires);

FRAMEWORK_EXPORT array_list_t framework_getBundles(framework_t framework);
FRAMEWORK_EXPORT bundle_t framework_getBundle(framework_t framework, char * location);
FRAMEWORK_EXPORT bundle_t framework_getBundleById(framework_t framework, long id);

#endif /* FRAMEWORK_PRIVATE_H_ */
