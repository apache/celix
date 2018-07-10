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
#include "celix_bundle_context.h"

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


celix_status_t bundleContext_addServiceListener(bundle_context_pt context, celix_service_listener_t *listener, const char * filter) {
	mock_c()->actualCall("bundleContext_addServiceListener")
		->withPointerParameters("context", context)
		->withPointerParameters("listener", listener)
		->withStringParameters("filter", filter);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, celix_service_listener_t *listener) {
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

long celix_bundleContext_registerService(bundle_context_t *ctx, void *svc, const char *serviceName, properties_t *properties) {
	mock_c()->actualCall("celix_bundleContext_registerCService")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("svc", svc)
			->withPointerParameters("properties", properties);
	return mock_c()->returnValue().value.longIntValue;
}

long celix_bundleContext_registerServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_registerServiceWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
	return mock_c()->returnValue().value.longIntValue;
}

void celix_bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
	mock_c()->actualCall("celix_bundleContext_unregisterService")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("serviceId", serviceId);
}

bool celix_bundleContext_useServiceWithId(
		bundle_context_t *ctx,
		long serviceId,
		const char *serviceName,
		void *callbackHandle,
		void (*use)(void *handle, void* svc)
) {
	mock_c()->actualCall("celix_bundleContext_registerServiceForLang")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("serviceId", serviceId)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
	return mock_c()->returnValue().value.boolValue;
}

dm_dependency_manager_t* celix_bundleContext_getDependencyManager(bundle_context_t *ctx) {
	mock_c()->actualCall("celix_bundleContext_getDependencyManager");
	return mock_c()->returnValue().value.pointerValue;
}

long celix_bundleContext_trackBundlesWithOptions(
		bundle_context_t* ctx,
		const celix_bundle_tracking_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_trackBundlesWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
	return mock_c()->returnValue().value.longIntValue;
}

long celix_bundleContext_trackBundles(
		bundle_context_t* ctx,
		void* callbackHandle,
		void (*onStarted)(void* handle, const bundle_t *bundle),
		void (*onStopped)(void *handle, const bundle_t *bundle)) {
	mock_c()->actualCall("celix_bundleContext_trackBundles")
			->withPointerParameters("ctx", ctx)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("onStarted", onStarted)
			->withPointerParameters("onStopped", onStopped);
	return mock_c()->returnValue().value.longIntValue;
}


void celix_bundleContext_useBundles(
		bundle_context_t *ctx,
		void *callbackHandle,
		void (*use)(void *handle, const bundle_t *bundle)) {
	mock_c()->actualCall("celix_bundleContext_useBundles")
			->withPointerParameters("ctx", ctx)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
}


void celix_bundleContext_useBundle(
		bundle_context_t *ctx,
		long bundleId,
		void *callbackHandle,
		void (*use)(void *handle, const bundle_t *bundle)) {
	mock_c()->actualCall("celix_bundleContext_useBundle")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("bundleId", bundleId)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
}

void celix_bundleContext_stopTracker(bundle_context_t *ctx, long trackerId) {
	mock_c()->actualCall("celix_bundleContext_stopTracker")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("trackerId", trackerId);
}

long celix_bundleContext_installBundle(bundle_context_t *ctx, const char *bundleLoc, bool autoStart) {
	mock_c()->actualCall("celix_bundleContext_installBundle")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("bundleLoc", bundleLoc)
			->withBoolParameters("autoStart", autoStart);
	return mock_c()->returnValue().value.longIntValue;
}

bool celix_bundleContext_uninstallBundle(bundle_context_t *ctx, long bundleId) {
	mock_c()->actualCall("celix_bundleContext_uninstallBundle")
			->withPointerParameters("ctx", ctx)
			->withLongIntParameters("bundleId", bundleId);
	return mock_c()->returnValue().value.boolValue;
}

bool celix_bundleContext_useServiceWithOptions(
		celix_bundle_context_t *ctx,
		const celix_service_use_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_useServiceWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
	return mock_c()->returnValue().value.boolValue;
}


void celix_bundleContext_useServicesWithOptions(
		celix_bundle_context_t *ctx,
		const celix_service_use_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_useServicesWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
}

long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props) {
	mock_c()->actualCall("celix_bundleContext_registerServiceFactory")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("factory", factory)
			->withPointerParameters("props", props);
	return mock_c()->returnValue().value.longIntValue;
}

long celix_bundleContext_findService(celix_bundle_context_t *ctx, const char *serviceName) {
	mock_c()->actualCall("celix_bundleContext_findService")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName);
	return mock_c()->returnValue().value.longIntValue;
}


long celix_bundleContext_findServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_findServiceWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
	return mock_c()->returnValue().value.longIntValue;
}


celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName) {
	mock_c()->actualCall("celix_bundleContext_findServices")
			->withPointerParameters("ctx", ctx)
			->withStringParameters("serviceName", serviceName);
	return mock_c()->returnValue().value.pointerValue;
}

celix_array_list_t* celix_bundleContext_findServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
	mock_c()->actualCall("celix_bundleContext_findServicesWithOptions")
			->withPointerParameters("ctx", ctx)
			->withConstPointerParameters("opts", opts);
	return mock_c()->returnValue().value.pointerValue;
}