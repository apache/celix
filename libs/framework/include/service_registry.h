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


#ifndef SERVICE_REGISTRY_H_
#define SERVICE_REGISTRY_H_

typedef struct celix_serviceRegistry * service_registry_pt;
typedef struct celix_serviceRegistry celix_service_registry_t;

#include "properties.h"
#include "filter.h"
#include "service_factory.h"
#include "celix_service_event.h"
#include "array_list.h"
#include "service_registration.h"
#include "celix_service_factory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*serviceChanged_function_pt)(celix_framework_t*, celix_service_event_type_t, service_registration_pt, celix_properties_t*);

celix_status_t serviceRegistry_create(celix_framework_t *framework, serviceChanged_function_pt serviceChanged,
                                      service_registry_pt *registry);

celix_status_t serviceRegistry_destroy(service_registry_pt registry);

celix_status_t
serviceRegistry_getRegisteredServices(service_registry_pt registry, celix_bundle_t *bundle, celix_array_list_t **services);

celix_status_t
serviceRegistry_getServicesInUse(service_registry_pt registry, celix_bundle_t *bundle, celix_array_list_t **services);

celix_status_t serviceRegistry_registerService(service_registry_pt registry, celix_bundle_t *bundle, const char *serviceName,
                                               const void *serviceObject, celix_properties_t *dictionary,
                                               service_registration_pt *registration);

celix_status_t
serviceRegistry_registerServiceFactory(service_registry_pt registry, celix_bundle_t *bundle, const char *serviceName,
                                       service_factory_pt factory, celix_properties_t *dictionary,
                                       service_registration_pt *registration);

celix_status_t
serviceRegistry_unregisterService(service_registry_pt registry, celix_bundle_t *bundle, service_registration_pt registration);

celix_status_t serviceRegistry_clearServiceRegistrations(service_registry_pt registry, celix_bundle_t *bundle);

celix_status_t serviceRegistry_getServiceReference(service_registry_pt registry, celix_bundle_t *bundle,
                                                   service_registration_pt registration,
                                                   service_reference_pt *reference);

celix_status_t
serviceRegistry_getServiceReferences(service_registry_pt registry, celix_bundle_t *bundle, const char *serviceName,
                                     filter_pt filter, celix_array_list_t **references);

celix_status_t
serviceRegistry_retainServiceReference(service_registry_pt registry, celix_bundle_t *bundle, service_reference_pt reference);

celix_status_t
serviceRegistry_ungetServiceReference(service_registry_pt registry, celix_bundle_t *bundle, service_reference_pt reference);

celix_status_t
serviceRegistry_getService(service_registry_pt registry, celix_bundle_t *bundle, service_reference_pt reference,
                           const void **service);

celix_status_t
serviceRegistry_ungetService(service_registry_pt registry, celix_bundle_t *bundle, service_reference_pt reference,
                             bool *result);

celix_status_t serviceRegistry_clearReferencesFor(service_registry_pt registry, celix_bundle_t *bundle);

void serviceRegistry_callHooksForListenerFilter(service_registry_pt registry, celix_bundle_t *owner, const char *filter, bool removed);

size_t serviceRegistry_nrOfHooks(service_registry_pt registry);

celix_status_t
serviceRegistry_servicePropertiesModified(service_registry_pt registry, service_registration_pt registration,
                                          celix_properties_t *oldprops);

celix_status_t
celix_serviceRegistry_registerServiceFactory(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        celix_service_factory_t *factory,
        celix_properties_t* props,
        service_registration_t **registration);

/**
 * List the registered service for the provided bundle.
 * @return A list of service ids. Caller is owner of the array list.
 */
celix_array_list_t* celix_serviceRegistry_listServiceIdsForOwner(celix_service_registry_t* registry, long bndId);

/**
 * Get service information for the provided svc id and bnd id.
 *
 * If the output pointers for serviceName and/or serviceProperties are provided these will get a copy of the registry
 * value. The caller is owner of the serviceName/serviceProperties.
 *
 * Returns true if the bundle is found.
 */
bool celix_serviceRegistry_getServiceInfo(
        celix_service_registry_t* registry,
        long svcId,
        long bndId,
        char **serviceName,
        celix_properties_t **serviceProperties,
        bool *factory);


#ifdef __cplusplus
}
#endif

#endif /* SERVICE_REGISTRY_H_ */
