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
 * service_registration_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "service_registration.h"

service_registration_pt serviceRegistration_create(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary) {
	mock_c()->actualCall("serviceRegistration_create")
		->withPointerParameters("pool", pool)
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withIntParameters("serviceId", serviceId)
		->withPointerParameters("serviceObject", serviceObject)
		->withPointerParameters("dictionary", dictionary);
	return mock_c()->returnValue().value.pointerValue;
}

service_registration_pt serviceRegistration_createServiceFactory(apr_pool_t *pool, service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary) {
	mock_c()->actualCall("serviceRegistration_createServiceFactory")
		->withPointerParameters("pool", pool)
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withIntParameters("serviceId", serviceId)
		->withPointerParameters("serviceObject", serviceObject)
		->withPointerParameters("dictionary", dictionary);
	return mock_c()->returnValue().value.pointerValue;
}

void serviceRegistration_destroy(service_registration_pt registration) {
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


celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, void **service) {
	mock_c()->actualCall("serviceRegistration_getService")
		->withPointerParameters("registration", registration)
		->withPointerParameters("bundle", bundle)
		->_andPointerOutputParameters("service", (void **) service);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties) {
	mock_c()->actualCall("serviceRegistration_getProperties")
			->withPointerParameters("registration", registration)
			->_andPointerOutputParameters("properties", (void **) properties);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getRegistry(service_registration_pt registration, service_registry_pt *registry) {
	mock_c()->actualCall("serviceRegistration_getRegistry")
			->withPointerParameters("registration", registration)
			->_andPointerOutputParameters("registry", (void **) registry);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getServiceReferences(service_registration_pt registration, array_list_pt *references) {
	mock_c()->actualCall("serviceRegistration_getServiceReferences")
			->withPointerParameters("registration", registration)
			->_andPointerOutputParameters("references", (void **) references);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle) {
	mock_c()->actualCall("serviceRegistration_getBundle")
			->withPointerParameters("registration", registration)
			->_andPointerOutputParameters("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, char **serviceName) {
	mock_c()->actualCall("serviceRegistration_getServiceName")
			->withPointerParameters("registration", registration)
			->_andStringOutputParameters("serviceName", (const char **) serviceName);
	return mock_c()->returnValue().value.intValue;
}



