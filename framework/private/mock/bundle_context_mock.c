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
 * bundle_context_mock.c
 *
 *  \date       Feb 6, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_context.h"

celix_status_t bundleContext_create(framework_pt framework, framework_logger_pt logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
	mock_c()->actualCall("bundleContext_create")
			->withPointerParameters("framework", framework)
			->withPointerParameters("logger", logger)
			->withPointerParameters("bundle", bundle)
			->_andPointerOutputParameters("bundle_context", (void **) bundle_context);
	return mock_c()->returnValue().value.intValue;
}
celix_status_t bundleContext_destroy(bundle_context_pt context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_getBundle")
			->withPointerParameters("context", context)
			->_andPointerOutputParameters("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
	mock_c()->actualCall("bundleContext_getFramework")
			->withPointerParameters("context", context)
			->_andPointerOutputParameters("framework", (void **) framework);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getMemoryPool(bundle_context_pt context, apr_pool_t **memory_pool) {
	mock_c()->actualCall("bundleContext_getMemoryPool")
			->withPointerParameters("context", context)
			->_andPointerOutputParameters("memory_pool", (void **) memory_pool);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_installBundle(bundle_context_pt context, char * location, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_installBundle")
			->withPointerParameters("context", context)
			->withStringParameters("location", location)
			->_andPointerOutputParameters("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context, char * location, char *inputFile, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_installBundle2")
			->withPointerParameters("context", context)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->_andPointerOutputParameters("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_registerService(bundle_context_pt context, char * serviceName, void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	mock_c()->actualCall("bundleContext_registerService")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("svcObj", svcObj)
			->withPointerParameters("properties", properties)
			->_andPointerOutputParameters("service_registration", (void **) service_registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, char * serviceName, service_factory_pt factory,
        properties_pt properties, service_registration_pt *service_registration) {
	mock_c()->actualCall("bundleContext_registerServiceFactory")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("factory", factory)
			->withPointerParameters("properties", properties)
			->_andPointerOutputParameters("service_registration", (void **) service_registration);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, char * filter, array_list_pt *service_references) {
	mock_c()->actualCall("bundleContext_getServiceReferences")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withStringParameters("filter", filter)
			->_andPointerOutputParameters("service_references", (void **) service_references);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, char * serviceName, service_reference_pt *service_reference) {
	mock_c()->actualCall("bundleContext_getServiceReference")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->_andPointerOutputParameters("service_reference", (void **) service_reference);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void **service_instance) {
	mock_c()->actualCall("bundleContext_getService")
			->withPointerParameters("context", context)
			->withPointerParameters("reference", reference)
			->_andPointerOutputParameters("service_instance", (void **) service_instance);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
	mock_c()->actualCall("bundleContext_ungetService")
			->withPointerParameters("context", context)
			->withPointerParameters("reference", reference)
			->_andIntOutputParameters("result", (int *) result);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles) {
	mock_c()->actualCall("bundleContext_getBundles");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_getBundleById");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_addServiceListener(bundle_context_pt context, service_listener_pt listener, char * filter) {
	mock_c()->actualCall("bundleContext_addServiceListener")
		->withPointerParameters("context", context)
		->withPointerParameters("listener", listener)
		->withStringParameters("filter", filter);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, service_listener_pt listener) {
	mock_c()->actualCall("bundleContext_removeServiceListener")
		->withPointerParameters("context", context)
		->withPointerParameters("listener", listener);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
	mock_c()->actualCall("bundleContext_addBundleListener");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
	mock_c()->actualCall("bundleContext_removeBundleListener");
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getProperty(bundle_context_pt context, const char *name, char **value) {
	mock_c()->actualCall("bundleContext_getProperty");
	return mock_c()->returnValue().value.intValue;
}
