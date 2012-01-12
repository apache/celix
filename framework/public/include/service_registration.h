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
 * service_registration.h
 *
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_REGISTRATION_H_
#define SERVICE_REGISTRATION_H_

#include "celixbool.h"

typedef struct serviceRegistration * SERVICE_REGISTRATION;

#include "service_registry.h"
#include "array_list.h"
#include "bundle.h"

SERVICE_REGISTRATION serviceRegistration_create(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary);
SERVICE_REGISTRATION serviceRegistration_createServiceFactory(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary);
void serviceRegistration_destroy(SERVICE_REGISTRATION registration);

bool serviceRegistration_isValid(SERVICE_REGISTRATION registration);
void serviceRegistration_invalidate(SERVICE_REGISTRATION registration);
celix_status_t serviceRegistration_unregister(SERVICE_REGISTRATION registration);

celix_status_t serviceRegistration_getService(SERVICE_REGISTRATION registration, BUNDLE bundle, void **service);

celix_status_t serviceRegistration_getProperties(SERVICE_REGISTRATION registration, PROPERTIES *properties);
celix_status_t serviceRegistration_getRegistry(SERVICE_REGISTRATION registration, SERVICE_REGISTRY *registry);
celix_status_t serviceRegistration_getServiceReferences(SERVICE_REGISTRATION registration, ARRAY_LIST *references);
celix_status_t serviceRegistration_getBundle(SERVICE_REGISTRATION registration, BUNDLE *bundle);
celix_status_t serviceRegistration_getServiceName(SERVICE_REGISTRATION registration, char **serviceName);

#endif /* SERVICE_REGISTRATION_H_ */
