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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <CppUTestExt/MockSupport_c.h>
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_context.h"

celix_status_t bundleContext_create(framework_pt framework, framework_logger_pt logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
	mock_c()->actualCall("bundleContext_create")
			->withPointerParameters("framework", framework)
			->withPointerParameters("logger", logger)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("bundle_context", (void **) bundle_context);
	return mock_c()->returnValue().value.intValue;
}
celix_status_t bundleContext_destroy(bundle_context_pt context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_getBundle")
			->withPointerParameters("context", context)
			->withOutputParameter("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
	mock_c()->actualCall("bundleContext_getFramework")
			->withPointerParameters("context", context)
			->withOutputParameter("framework", (void **) framework);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_installBundle(bundle_context_pt context, const char * location, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_installBundle")
			->withPointerParameters("context", context)
			->withStringParameters("location", location)
			->withOutputParameter("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context, const char * location, const char *inputFile, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_installBundle2")
			->withPointerParameters("context", context)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->withOutputParameter("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_registerService(bundle_context_pt context, const char * serviceName, const void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	mock_c()->actualCall("bundleContext_registerService")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("svcObj", (void*)svcObj)
			->withPointerParameters("properties", properties)
			->withOutputParameter("service_registration", (void **) service_registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, const char * serviceName, service_factory_pt factory,
        properties_pt properties, service_registration_pt *service_registration) {
	mock_c()->actualCall("bundleContext_registerServiceFactory")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("factory", factory)
			->withPointerParameters("properties", properties)
			->withOutputParameter("service_registration", (void **) service_registration);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, const char * filter, array_list_pt *service_references) {
	mock_c()->actualCall("bundleContext_getServiceReferences")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withStringParameters("filter", filter)
			->withOutputParameter("service_references", (void **) service_references);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, const char * serviceName, service_reference_pt *service_reference) {
	mock_c()->actualCall("bundleContext_getServiceReference")
			->withPointerParameters("context", context)
			->withStringParameters("serviceName", serviceName)
			->withOutputParameter("service_reference", (void **) service_reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt reference) {
    mock_c()->actualCall("bundleContext_retainServiceReference")
            ->withPointerParameters("context", context)
            ->withPointerParameters("reference", reference);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference) {
    mock_c()->actualCall("bundleContext_ungetServiceReference")
            ->withPointerParameters("context", context)
            ->withPointerParameters("reference", reference);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void **service_instance) {
	mock_c()->actualCall("bundleContext_getService")
			->withPointerParameters("context", context)
			->withPointerParameters("reference", reference)
			->withOutputParameter("service_instance", (void **) service_instance);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
	mock_c()->actualCall("bundleContext_ungetService")
			->withPointerParameters("context", context)
			->withPointerParameters("reference", reference)
			->withOutputParameter("result", (int *) result);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles) {
	mock_c()->actualCall("bundleContext_getBundles")
			->withPointerParameters("context", context)
			->withOutputParameter("bundles", bundles);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle) {
	mock_c()->actualCall("bundleContext_getBundleById")
			->withPointerParameters("context", context)
			->withLongIntParameters("id", id)
			->withOutputParameter("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t bundleContext_addServiceListener(bundle_context_pt context, service_listener_pt listener, const char * filter) {
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


celix_status_t bundleContext_getProperty(bundle_context_pt context, const char *name, const char** value) {
	mock_c()->actualCall("bundleContext_getProperty")
			->withPointerParameters("context", context)
			->withStringParameters("name", name)
			->withOutputParameter("value", value);
	return mock_c()->returnValue().value.intValue;
}

long bundleContext_registerCService(bundle_context_t *ctx, const char *serviceName, void *svc, properties_t *properties, const char *serviceVersion) {
	mock_c()->actualCall("bundleContext_registerCService")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("svc", svc)
			->withPointerParameters("properties", properties)
			->withStringParameters("serviceName", serviceVersion);
	return mock_c()->returnValue().value.longIntValue;
}

long bundleContext_registerServiceForLang(bundle_context_t *ctx, const char *serviceName, void *svc, properties_t *properties, const char *serviceVersion, const char* lang) {
	mock_c()->actualCall("bundleContext_registerServiceForLang")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName)
			->withConstPointerParameters("svc", svc)
			->withPointerParameters("properties", properties)
			->withStringParameters("serviceName", serviceVersion)
			->withStringParameters("lang", lang);
	return mock_c()->returnValue().value.longIntValue;
}

void bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
	mock_c()->actualCall("bundleContext_unregisterService")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("serviceId", serviceId);
}

bool bundleContext_useServiceWithId(
		bundle_context_t *ctx,
		long serviceId,
		const char *serviceName,
		void *callbackHandle,
		void (*use)(void *handle, void* svc, const properties_t *props, const bundle_t *owner)
) {
	mock_c()->actualCall("bundleContext_registerServiceForLang")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("serviceId", serviceId)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
	return mock_c()->returnValue().value.boolValue;
}