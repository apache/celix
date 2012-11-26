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

typedef struct serviceRegistry * SERVICE_REGISTRY;

#include "properties.h"
#include "filter.h"
#include "service_factory.h"
#include "service_event.h"
#include "array_list.h"
#include "service_registration.h"

SERVICE_REGISTRY serviceRegistry_create(FRAMEWORK framework, void (*serviceChanged)(FRAMEWORK, SERVICE_EVENT_TYPE, SERVICE_REGISTRATION, PROPERTIES));
celix_status_t serviceRegistry_destroy(SERVICE_REGISTRY registry);
celix_status_t serviceRegistry_getRegisteredServices(SERVICE_REGISTRY registry, apr_pool_t *pool, BUNDLE bundle, ARRAY_LIST *services);
ARRAY_LIST serviceRegistry_getServicesInUse(SERVICE_REGISTRY registry, BUNDLE bundle);
SERVICE_REGISTRATION serviceRegistry_registerService(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary);
SERVICE_REGISTRATION serviceRegistry_registerServiceFactory(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, service_factory_t factory, PROPERTIES dictionary);
void serviceRegistry_unregisterService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REGISTRATION registration);
void serviceRegistry_unregisterServices(SERVICE_REGISTRY registry, BUNDLE bundle);
celix_status_t serviceRegistry_getServiceReferences(SERVICE_REGISTRY registry, apr_pool_t *pool, const char *serviceName, filter_t filter, ARRAY_LIST *references);
void * serviceRegistry_getService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference);
bool serviceRegistry_ungetService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference);
void serviceRegistry_ungetServices(SERVICE_REGISTRY registry, BUNDLE bundle);
ARRAY_LIST serviceRegistry_getUsingBundles(SERVICE_REGISTRY registry, apr_pool_t *pool, SERVICE_REFERENCE reference);
SERVICE_REGISTRATION serviceRegistry_findRegistration(SERVICE_REGISTRY registry, SERVICE_REFERENCE reference);
celix_status_t serviceRegistry_createServiceReference(SERVICE_REGISTRY registry, apr_pool_t *pool, SERVICE_REGISTRATION registration, SERVICE_REFERENCE *reference);

celix_status_t serviceRegistry_getListenerHooks(SERVICE_REGISTRY registry, apr_pool_t *pool, ARRAY_LIST *hooks);

celix_status_t serviceRegistry_servicePropertiesModified(SERVICE_REGISTRY registry, SERVICE_REGISTRATION registration, PROPERTIES oldprops);

#endif /* SERVICE_REGISTRY_H_ */
