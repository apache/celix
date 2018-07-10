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

#include <CppUTestExt/MockSupport_c.h>
#include "CppUTestExt/MockSupport_c.h"

#include "service_registration.h"
#include "service_registration_private.h"
#include "service_registry.h"

service_registration_pt serviceRegistration_create(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId, const void* serviceObject, properties_pt dictionary) {
	mock_c()->actualCall("serviceRegistration_create")
		->withParameterOfType("registry_callback_t", "callback", &callback)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withUnsignedLongIntParameters("serviceId", serviceId)
		->withPointerParameters("serviceObject", (void*)serviceObject)
		->withPointerParameters("dictionary", dictionary);
	return mock_c()->returnValue().value.pointerValue;
}

service_registration_pt serviceRegistration_createServiceFactory(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId, const void* serviceObject, properties_pt dictionary) {
	mock_c()->actualCall("serviceRegistration_createServiceFactory")
		->withParameterOfType("registry_callback_t", "callback", &callback)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withUnsignedLongIntParameters("serviceId", serviceId)
		->withPointerParameters("serviceObject", (void*) serviceObject)
		->withPointerParameters("dictionary", dictionary);
	return mock_c()->returnValue().value.pointerValue;
}

celix_status_t serviceRegistration_destroy(service_registration_pt registration) {
	mock_c()->actualCall("serviceRegistration_destroy")
			->withPointerParameters("registration", registration);
	return mock_c()->returnValue().value.intValue;
}


bool serviceRegistration_isValid(service_registration_pt registration) {
	mock_c()->actualCall("serviceRegistration_isValid")
			->withPointerParameters("registration", registration);
	return mock_c()->returnValue().value.intValue;
}

void serviceRegistration_invalidate(service_registration_pt registration) {
	mock_c()->actualCall("serviceRegistration_invalidate")
			->withPointerParameters("registration", registration);
}

celix_status_t serviceRegistration_unregister(service_registration_pt registration) {
	mock_c()->actualCall("serviceRegistration_unregister")
		->withPointerParameters("registration", registration);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, const void **service) {
	mock_c()->actualCall("serviceRegistration_getService")
		->withPointerParameters("registration", registration)
		->withPointerParameters("bundle", bundle)
		->withOutputParameter("service", (void **) service);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, const void **service) {
	mock_c()->actualCall("serviceRegistration_ungetService")
			->withPointerParameters("registration", registration)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("service", service);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties) {
	mock_c()->actualCall("serviceRegistration_getProperties")
			->withPointerParameters("registration", registration)
			->withOutputParameter("properties", (void **) properties);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getRegistry(service_registration_pt registration, service_registry_pt *registry) {
	mock_c()->actualCall("serviceRegistration_getRegistry")
			->withPointerParameters("registration", registration)
			->withOutputParameter("registry", (void **) registry);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getServiceReferences(service_registration_pt registration, array_list_pt *references) {
	mock_c()->actualCall("serviceRegistration_getServiceReferences")
			->withPointerParameters("registration", registration)
			->withOutputParameter("references", references);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle) {
	mock_c()->actualCall("serviceRegistration_getBundle")
			->withPointerParameters("registration", registration)
			->withOutputParameter("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, const char **serviceName) {
	mock_c()->actualCall("serviceRegistration_getServiceName")
			->withPointerParameters("registration", registration)
			->withOutputParameter("serviceName", (const char **) serviceName);
	return mock_c()->returnValue().value.intValue;
}

void serviceRegistration_retain(service_registration_pt registration) {
    mock_c()->actualCall("serviceRegistration_retain")
            ->withPointerParameters("registration", registration);
}

void serviceRegistration_release(service_registration_pt registration) {
    mock_c()->actualCall("serviceRegistration_release")
            ->withPointerParameters("registration", registration);
}


long serviceRegistration_getServiceId(service_registration_t *registration) {
	mock_c()->actualCall("serviceRegistration_getServiceId")
			->withPointerParameters("registration", registration);
	return mock_c()->returnValue().value.longIntValue;
}

service_registration_t* celix_serviceRegistration_createServiceFactory(
		registry_callback_t callback,
		const celix_bundle_t *bnd,
		const char *serviceName,
		long svcId,
		celix_service_factory_t* factory,
		celix_properties_t *props) {
	mock_c()->actualCall("celix_serviceRegistration_createServiceFactory")
			->withParameterOfType("registry_callback_t", "callback", &callback)
			->withConstPointerParameters("bnd", bnd)
			->withStringParameters("serviceName", serviceName)
			->withUnsignedLongIntParameters("svcId", svcId)
			->withPointerParameters("factory", (void*) factory)
			->withPointerParameters("props", props);
	return mock_c()->returnValue().value.pointerValue;
}

