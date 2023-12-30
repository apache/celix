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


#ifndef SERVICE_REGISTRATION_PRIVATE_H_
#define SERVICE_REGISTRATION_PRIVATE_H_

#include "registry_callback_private.h"
#include "service_registration.h"
#include "celix_ref.h"

enum celix_service_type {
	CELIX_PLAIN_SERVICE,
	CELIX_FACTORY_SERVICE,
	CELIX_DEPRECATED_FACTORY_SERVICE
};

struct serviceRegistration {
    struct celix_ref refCount;
    registry_callback_t callback; // read-only

    char* className;          // read-only
    bundle_pt bundle;         // read-only
    properties_pt properties; // read-only
    long serviceId;           // read-only

    bool isUnregistering;

    enum celix_service_type svcType; // read-only
    union {
        const void* svcObj;
        service_factory_pt deprecatedFactory;
        celix_service_factory_t* factory;
    };

    celix_thread_rwlock_t lock; // protects the service object
};

service_registration_pt serviceRegistration_create(registry_callback_t callback, bundle_pt bundle, const char* serviceName, long serviceId, const void * serviceObject, properties_pt dictionary);
service_registration_pt serviceRegistration_createServiceFactory(registry_callback_t callback, bundle_pt bundle, const char* serviceName, long serviceId, const void * serviceObject, properties_pt dictionary);

void serviceRegistration_retain(service_registration_pt registration);
void serviceRegistration_release(service_registration_pt registration);

bool serviceRegistration_isValid(service_registration_pt registration);
void serviceRegistration_invalidate(service_registration_pt registration);

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, const void **service);
celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, const void **service);

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle);
celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, const char **serviceName);


service_registration_t* celix_serviceRegistration_createServiceFactory(
		registry_callback_t callback,
		const celix_bundle_t *bnd,
		const char *serviceName,
		long svcId,
		celix_service_factory_t* factory,
		celix_properties_t *props);

#endif /* SERVICE_REGISTRATION_PRIVATE_H_ */
