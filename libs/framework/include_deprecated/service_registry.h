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

#include "celix_properties.h"
#include "filter.h"
#include "service_factory.h"
#include "celix_service_event.h"
#include "array_list.h"
#include "service_registration.h"
#include "celix_service_factory.h"
#include "celix_framework_export.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_serviceRegistry * service_registry_pt;
typedef struct celix_serviceRegistry celix_service_registry_t;

typedef void (*serviceChanged_function_pt)(celix_framework_t*, celix_service_event_type_t, service_registration_pt, celix_properties_t*);

CELIX_FRAMEWORK_EXPORT celix_service_registry_t* celix_serviceRegistry_create(celix_framework_t *framework);

CELIX_FRAMEWORK_EXPORT void celix_serviceRegistry_destroy(celix_service_registry_t* registry);

CELIX_FRAMEWORK_EXPORT celix_status_t
serviceRegistry_getRegisteredServices(celix_service_registry_t* registry, celix_bundle_t *bundle, celix_array_list_t **services);

CELIX_FRAMEWORK_EXPORT celix_status_t
serviceRegistry_getServicesInUse(celix_service_registry_t* registry, celix_bundle_t *bundle, celix_array_list_t **services);

CELIX_FRAMEWORK_EXPORT celix_status_t serviceRegistry_registerService(celix_service_registry_t* registry, celix_bundle_t *bundle, const char *serviceName,
                                               const void *serviceObject, celix_properties_t *dictionary,
                                               service_registration_pt *registration);

CELIX_FRAMEWORK_EXPORT celix_status_t
serviceRegistry_registerServiceFactory(celix_service_registry_t* registry, celix_bundle_t *bundle, const char *serviceName,
                                       service_factory_pt factory, celix_properties_t *dictionary,
                                       service_registration_pt *registration);

CELIX_FRAMEWORK_EXPORT celix_status_t serviceRegistry_getServiceReference(celix_service_registry_t* registry, celix_bundle_t *bundle,
                                                   service_registration_pt registration,
                                                   service_reference_pt *reference);

CELIX_FRAMEWORK_EXPORT celix_status_t
serviceRegistry_getServiceReferences(celix_service_registry_t* registry, celix_bundle_t *bundle, const char *serviceName,
                                     filter_pt filter, celix_array_list_t **references);

CELIX_FRAMEWORK_EXPORT celix_status_t serviceRegistry_clearReferencesFor(celix_service_registry_t* registry, celix_bundle_t *bundle);

CELIX_FRAMEWORK_EXPORT size_t serviceRegistry_nrOfHooks(celix_service_registry_t* registry);

/**
 * Register a service listener. Will also retroactively call the listener with register events for already registered services.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_serviceRegistry_addServiceListener(celix_service_registry_t *reg, celix_bundle_t *bundle, const char *filter, celix_service_listener_t *listener);

CELIX_FRAMEWORK_EXPORT celix_status_t celix_serviceRegistry_removeServiceListener(celix_service_registry_t *reg, celix_service_listener_t *listener);

CELIX_FRAMEWORK_EXPORT bool celix_serviceRegistry_isServiceRegistered(celix_service_registry_t* reg, long serviceId);

CELIX_FRAMEWORK_EXPORT celix_status_t
celix_serviceRegistry_registerService(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        void* service,
        celix_properties_t* props,
        long reserveId,
        service_registration_t **registration);

CELIX_FRAMEWORK_EXPORT celix_status_t
celix_serviceRegistry_registerServiceFactory(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        celix_service_factory_t *factory,
        celix_properties_t* props,
        long reserveId,
        service_registration_t **registration);

/**
 * List the registered service for the provided bundle.
 * @return A list of service ids. Caller is owner of the array list.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_serviceRegistry_listServiceIdsForOwner(celix_service_registry_t* registry, long bndId);

/**
 * Get service information for the provided svc id and bnd id.
 *
 * If the output pointers for serviceName and/or serviceProperties are provided these will get a copy of the registry
 * value. The caller is owner of the serviceName/serviceProperties.
 *
 * Returns true if the bundle is found.
 */
CELIX_FRAMEWORK_EXPORT bool celix_serviceRegistry_getServiceInfo(
        celix_service_registry_t* registry,
        long svcId,
        long bndId,
        char **serviceName,
        celix_properties_t **serviceProperties,
        bool *factory);


/**
 * Returns the next svc id.
 */
CELIX_FRAMEWORK_EXPORT long celix_serviceRegistry_nextSvcId(celix_service_registry_t* registry);


/**
 * Unregister service for the provided service id (owned by bnd).
 * Will print an error if the service id is invalid
 */
CELIX_FRAMEWORK_EXPORT void celix_serviceRegistry_unregisterService(celix_service_registry_t* registry, celix_bundle_t* bnd, long serviceId);


/**
 * Create a LDAP filter for the provided filter parts.
 * @param serviceName       The optional service name
 * @param versionRange      The optional version range
 * @param filter            The optional filter
 * @return a LDAP filter. Caller is responsible for freeing the filter.
 */
CELIX_FRAMEWORK_EXPORT char* celix_serviceRegistry_createFilterFor(
        celix_service_registry_t* registry,
        const char* serviceName,
        const char* versionRange,
        const char* additionalFilter);

/**
 * Find services and return a array list of service ids (long).
 * Caller is responsible for freeing the returned array list.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_serviceRegisrty_findServices(celix_service_registry_t* registry, const char* filter);


#ifdef __cplusplus
}
#endif

#endif /* SERVICE_REGISTRY_H_ */
