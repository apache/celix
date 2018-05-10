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
 * service_registry_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "service_registry.h"

celix_status_t serviceRegistry_create(framework_pt framework, serviceChanged_function_pt serviceChanged, service_registry_pt *registry) {
	mock_c()->actualCall("serviceRegistry_create")
			->withPointerParameters("framework", framework)
			->withPointerParameters("serviceChanged", serviceChanged)
			->withOutputParameter("registry", registry);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_destroy(service_registry_pt registry) {
	mock_c()->actualCall("serviceRegistry_destroy");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_pt registry, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("serviceRegistry_getRegisteredServices")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("services", services);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getServicesInUse(service_registry_pt registry, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("serviceRegistry_getServicesInUse")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("services", services);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_registerService(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void * serviceObject, properties_pt dictionary, service_registration_pt *registration) {
	mock_c()->actualCall("serviceRegistry_registerService")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("serviceObject", (void*)serviceObject)
			->withPointerParameters("dictionary", dictionary)
			->withOutputParameter("registration", registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_registerServiceFactory(service_registry_pt registry, bundle_pt bundle, const char* serviceName, service_factory_pt factory, properties_pt dictionary, service_registration_pt *registration) {
	mock_c()->actualCall("serviceRegistry_registerServiceFactory")
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withPointerParameters("factory", factory)
		->withPointerParameters("dictionary", dictionary)
		->withOutputParameter("registration", registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_unregisterService(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration) {
	mock_c()->actualCall("serviceRegistry_unregisterService")
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("registration", registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_pt registry, bundle_pt bundle, const char *serviceName, filter_pt filter, array_list_pt *references) {
	mock_c()->actualCall("serviceRegistry_getServiceReferences")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("filter", filter)
			->withOutputParameter("references", references);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_retainServiceReference(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
	mock_c()->actualCall("serviceRegistry_retainServiceReference");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_ungetServiceReference(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
	mock_c()->actualCall("serviceRegistry_ungetServiceReference")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("reference", reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, const void **service) {
	mock_c()->actualCall("serviceRegistry_getService")
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("reference", reference)
		->withOutputParameter("service", service);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_ungetService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, bool *result) {
	mock_c()->actualCall("serviceRegistry_ungetService")
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withOutputParameter("result", result);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getListenerHooks(service_registry_pt registry, bundle_pt bundle, array_list_pt *hooks) {
	mock_c()->actualCall("serviceRegistry_getListenerHooks")
		->withPointerParameters("registry", registry)
		->withPointerParameters("bundle", bundle)
		->withOutputParameter("hooks", hooks);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_pt registry, service_registration_pt registration, properties_pt oldprops) {
	mock_c()->actualCall("serviceRegistry_servicePropertiesModified")
			->withPointerParameters("registry", registry)
			->withPointerParameters("registration", registration)
			->withPointerParameters("oldprops", oldprops);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_getServiceReference(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration, service_reference_pt *reference) {
	mock_c()->actualCall("serviceRegistry_getServiceReference")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("registration", registration)
			->withOutputParameter("reference", reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_clearReferencesFor(service_registry_pt registry, bundle_pt bundle) {
	mock_c()->actualCall("serviceRegistry_clearReferencesFor")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistry_clearServiceRegistrations(service_registry_pt registry, bundle_pt bundle) {
	mock_c()->actualCall("serviceRegistry_clearReferencesFor")
			->withPointerParameters("registry", registry)
			->withPointerParameters("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t
celix_serviceRegistry_registerServiceFactory(
		celix_service_registry_t *reg,
		const celix_bundle_t *bnd,
		const char *serviceName,
		celix_service_factory_t *factory,
		celix_properties_t* props,
		service_registration_t **registration) {
	mock_c()->actualCall("celix_serviceRegistry_registerServiceFactory")
			->withPointerParameters("reg", reg)
			->withConstPointerParameters("bnd", bnd)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("factory", factory)
			->withPointerParameters("props", props)
			->withOutputParameter("registration", registration);
	return mock_c()->returnValue().value.intValue;
}