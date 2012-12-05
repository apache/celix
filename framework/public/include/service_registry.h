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
 * service_registry.h
 *
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_REGISTRY_H_
#define SERVICE_REGISTRY_H_

#include <apr_general.h>

typedef struct serviceRegistry * service_registry_t;

#include "properties.h"
#include "filter.h"
#include "service_factory.h"
#include "service_event.h"
#include "array_list.h"
#include "service_registration.h"

service_registry_t serviceRegistry_create(framework_t framework, void (*serviceChanged)(framework_t, service_event_type_e, service_registration_t, properties_t));
celix_status_t serviceRegistry_destroy(service_registry_t registry);
celix_status_t serviceRegistry_getRegisteredServices(service_registry_t registry, apr_pool_t *pool, bundle_t bundle, array_list_t *services);
array_list_t serviceRegistry_getServicesInUse(service_registry_t registry, bundle_t bundle);
service_registration_t serviceRegistry_registerService(service_registry_t registry, bundle_t bundle, char * serviceName, void * serviceObject, properties_t dictionary);
service_registration_t serviceRegistry_registerServiceFactory(service_registry_t registry, bundle_t bundle, char * serviceName, service_factory_t factory, properties_t dictionary);
void serviceRegistry_unregisterService(service_registry_t registry, bundle_t bundle, service_registration_t registration);
void serviceRegistry_unregisterServices(service_registry_t registry, bundle_t bundle);
celix_status_t serviceRegistry_getServiceReferences(service_registry_t registry, apr_pool_t *pool, const char *serviceName, filter_t filter, array_list_t *references);
void * serviceRegistry_getService(service_registry_t registry, bundle_t bundle, service_reference_t reference);
bool serviceRegistry_ungetService(service_registry_t registry, bundle_t bundle, service_reference_t reference);
void serviceRegistry_ungetServices(service_registry_t registry, bundle_t bundle);
array_list_t serviceRegistry_getUsingBundles(service_registry_t registry, apr_pool_t *pool, service_reference_t reference);
service_registration_t serviceRegistry_findRegistration(service_registry_t registry, service_reference_t reference);
celix_status_t serviceRegistry_createServiceReference(service_registry_t registry, apr_pool_t *pool, service_registration_t registration, service_reference_t *reference);

celix_status_t serviceRegistry_getListenerHooks(service_registry_t registry, apr_pool_t *pool, array_list_t *hooks);

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_t registry, service_registration_t registration, properties_t oldprops);

#endif /* SERVICE_REGISTRY_H_ */
