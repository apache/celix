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
/**
 * framework_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <CppUTestExt/MockSupport_c.h>
#include "CppUTestExt/MockSupport_c.h"

#include "framework_private.h"

celix_status_t framework_create(framework_pt *framework, properties_pt config) {
	mock_c()->actualCall("framework_create");
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_destroy(framework_pt framework) {
	mock_c()->actualCall("framework_destroy");
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_init(framework_pt framework) {
	mock_c()->actualCall("fw_init");
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_start(framework_pt framework) {
	mock_c()->actualCall("framework_start");
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_stop(framework_pt framework) {
	mock_c()->actualCall("framework_stop");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getProperty(framework_pt framework, const char* name, const char* defaultValue, const char** value) {
	mock_c()->actualCall("fw_getProperty")
			->withPointerParameters("framework", framework)
			->withStringParameters("name", name)
			->withStringParameters("defaultValue", defaultValue)
			->withOutputParameter("value", (const char **) value);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, const char * location, const char *inputFile) {
	mock_c()->actualCall("fw_installBundle")
			->withPointerParameters("framework", framework)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->withOutputParameter("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle) {
	mock_c()->actualCall("fw_uninstallBundle")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_getBundleEntry(framework_pt framework, const_bundle_pt bundle, const char *name, char **entry) {
	mock_c()->actualCall("framework_getBundleEntry")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", (bundle_pt)bundle)
			->withStringParameters("name", name)
			->withOutputParameter("entry", (const char **) entry);
		return mock_c()->returnValue().value.intValue;
}


celix_status_t fw_startBundle(framework_pt framework, long bundle, int options) {
	mock_c()->actualCall("fw_startBundle")
		->withPointerParameters("framework", framework)
		->withLongIntParameters("bundle", bundle)
		->withIntParameters("options", options);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, const char *inputFile) {
	mock_c()->actualCall("framework_updateBundle")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("inputFile", inputFile);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_stopBundle(framework_pt framework, long bundle, bool record) {
	mock_c()->actualCall("fw_stopBundle")
		->withPointerParameters("framework", framework)
		->withLongIntParameters("bundle", bundle)
		->withIntParameters("record", record);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, long bundle, const char* serviceName, const void* svcObj, properties_pt properties) {
	mock_c()->actualCall("fw_registerService")
		->withPointerParameters("framework", framework)
		->withLongIntParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withPointerParameters("service", (void*) svcObj)
		->withPointerParameters("properties", properties)
		->withOutputParameter("registration", (void **) registration);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, long bundle, const char* serviceName, service_factory_pt factory, properties_pt properties) {
	mock_c()->actualCall("fw_registerServiceFactory")
		->withPointerParameters("framework", framework)
		->withLongIntParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withPointerParameters("serviceFactory", factory)
		->withPointerParameters("properties", properties)
		->withOutputParameter("registration", (void **) registration);
		return mock_c()->returnValue().value.intValue;
}

void fw_unregisterService(service_registration_pt registration) {
	mock_c()->actualCall("fw_unregisterService");
}


celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char * serviceName, const char * filter) {
	mock_c()->actualCall("fw_getServiceReferences")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withStringParameters("filter", filter)
		->withOutputParameter("references", (void **) references);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_ungetServiceReference(framework_pt framework, bundle_pt bundle, service_reference_pt reference) {
    mock_c()->actualCall("framework_ungetServiceReference")
        ->withPointerParameters("framework", framework)
        ->withPointerParameters("bundle", bundle)
        ->withPointerParameters("reference", reference);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, const void **service) {
	mock_c()->actualCall("fw_getService")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("reference", reference)
		->withOutputParameter("service", service);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, bool *result) {
	mock_c()->actualCall("framework_ungetService")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("reference", reference)
		->withOutputParameter("result", (int *) result);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getBundleRegisteredServices(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("fw_getBundleRegisteredServices")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("services", (void **) services);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("fw_getBundleServicesInUse")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle)
			->withOutputParameter("services", (void **) services);
		return mock_c()->returnValue().value.intValue;
}


void fw_addServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener, const char* filter) {
	mock_c()->actualCall("fw_addServiceListener")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("listener", listener)
		->withStringParameters("filter", filter);
}

void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, celix_service_listener_t *listener) {
	mock_c()->actualCall("fw_removeServiceListener")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("listener", listener);
}


celix_status_t fw_addBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
	mock_c()->actualCall("fw_addBundleListener")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("listener", listener);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_removeBundleListener(framework_pt framework, bundle_pt bundle, bundle_listener_pt listener) {
	mock_c()->actualCall("fw_removeBundleListener")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("listener", listener);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_addFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
    mock_c()->actualCall("fw_addframeworkListener")
            ->withPointerParameters("framework", framework)
            ->withPointerParameters("bundle", bundle)
            ->withPointerParameters("listener", listener);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_removeFrameworkListener(framework_pt framework, bundle_pt bundle, framework_listener_pt listener) {
    mock_c()->actualCall("fw_removeframeworkListener")
            ->withPointerParameters("framework", framework)
            ->withPointerParameters("bundle", bundle)
            ->withPointerParameters("listener", listener);
        return mock_c()->returnValue().value.intValue;
}

void fw_serviceChanged(framework_pt framework, celix_service_event_type_t eventType, service_registration_pt registration, properties_pt oldprops) {
	mock_c()->actualCall("fw_serviceChanged");
}


celix_status_t fw_isServiceAssignable(framework_pt fw, bundle_pt requester, service_reference_pt reference, bool *assignable) {
	mock_c()->actualCall("fw_isServiceAssignable");
		return mock_c()->returnValue().value.intValue;
}


//bundle_archive_t fw_createArchive(long id, char * location) {

//void revise(bundle_archive_t archive, char * location) {

celix_status_t getManifest(bundle_archive_pt archive, manifest_pt *manifest) {
	mock_c()->actualCall("getManifest")
			->withPointerParameters("archive", archive)
			->withOutputParameter("manifest", (void **) manifest);
		return mock_c()->returnValue().value.intValue;
}


bundle_pt findBundle(bundle_context_pt context) {
	mock_c()->actualCall("findBundle");
		return mock_c()->returnValue().value.pointerValue;
}

service_registration_pt findRegistration(service_reference_pt reference) {
	mock_c()->actualCall("findRegistration");
		return mock_c()->returnValue().value.pointerValue;
}


service_reference_pt listToArray(array_list_pt list) {
	mock_c()->actualCall("listToArray")
			->withPointerParameters("list", list);
		return mock_c()->returnValue().value.pointerValue;
}

celix_status_t framework_markResolvedModules(framework_pt framework, linked_list_pt wires) {
	mock_c()->actualCall("framework_markResolvedModules");
		return mock_c()->returnValue().value.intValue;
}


celix_status_t framework_waitForStop(framework_pt framework) {
	mock_c()->actualCall("framework_waitForStop");
		return mock_c()->returnValue().value.intValue;
}


array_list_pt framework_getBundles(framework_pt framework) {
	mock_c()->actualCall("framework_getBundles")
			->withPointerParameters("framework", framework);
		return mock_c()->returnValue().value.pointerValue;
}

bundle_pt framework_getBundle(framework_pt framework, const char* location) {
	mock_c()->actualCall("framework_getBundle")
			->withPointerParameters("framework", framework)
			->withStringParameters("location", location);
		return mock_c()->returnValue().value.pointerValue;
}

bundle_pt framework_getBundleById(framework_pt framework, long id) {
	mock_c()->actualCall("framework_getBundleById")
		->withPointerParameters("framework", framework)
		->withIntParameters("id", id);
	return mock_c()->returnValue().value.pointerValue;
}

celix_status_t framework_getFrameworkBundle(const_framework_pt framework, bundle_pt *bundle) {
	mock_c()->actualCall("framework_getFrameworkBundle")
			->withPointerParameters("framework", (framework_pt)framework)
			->withOutputParameter("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}

void celix_framework_useBundles(framework_t *fw, bool includeFrameworkBundle, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
	mock_c()->actualCall("celix_framework_useBundles")
			->withPointerParameters("fw", fw)
			->withBoolParameters("includeFrameworkBundle", includeFrameworkBundle)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
}

bool celix_framework_useBundle(framework_t *fw, bool onlyActive, long bundleId, void *callbackHandle, void(*use)(void *handle, const bundle_t *bnd)) {
	mock_c()->actualCall("celix_framework_useBundle")
			->withPointerParameters("fw", fw)
			->withBoolParameters("onlyActive", onlyActive)
			->withLongIntParameters("bundleId", bundleId)
			->withPointerParameters("callbackHandle", callbackHandle)
			->withPointerParameters("use", use);
	return mock_c()->returnValue().value.boolValue;
}

service_registration_t* celix_framework_registerServiceFactory(framework_t *fw , const celix_bundle_t *bnd, const char* serviceName, celix_service_factory_t *factory, celix_properties_t *properties) {
	mock_c()->actualCall("celix_framework_registerServiceFactory")
			->withPointerParameters("fw", fw)
			->withConstPointerParameters("bnd", bnd)
			->withStringParameters("serviceName", serviceName)
			->withPointerParameters("factory", factory)
			->withPointerParameters("properties", properties);
	return mock_c()->returnValue().value.pointerValue;
}

bool celix_framework_isBundleInstalled(celix_framework_t *fw, long bndId) {
    mock_c()->actualCall("celix_framework_isBundleInstalled")
            ->withPointerParameters("fw", fw)
            ->withLongIntParameters("bndId", bndId);
    return mock_c()->returnValue().value.boolValue;
}

bool celix_framework_isBundleActive(celix_framework_t *fw, long bndId) {
    mock_c()->actualCall("celix_framework_isBundleActive")
            ->withPointerParameters("fw", fw)
            ->withLongIntParameters("bndId", bndId);
    return mock_c()->returnValue().value.boolValue;
}

long celix_framework_installBundle(celix_framework_t *fw, const char *bundleLoc, bool autoStart) {
    mock_c()->actualCall("celix_framework_installBundle")
            ->withPointerParameters("fw", fw)
            ->withStringParameters("bundleLoc", bundleLoc)
            ->withBoolParameters("autoStart", autoStart);
    return mock_c()->returnValue().value.longIntValue;
}

bool celix_framework_uninstallBundle(celix_framework_t *fw, long bndId) {
    mock_c()->actualCall("celix_framework_uninstallBundle")
            ->withPointerParameters("fw", fw)
            ->withLongIntParameters("bndId", bndId);
    return mock_c()->returnValue().value.boolValue;
}

bool celix_framework_stopBundle(celix_framework_t *fw, long bndId) {
    mock_c()->actualCall("celix_framework_stopBundle")
            ->withPointerParameters("fw", fw)
            ->withLongIntParameters("bndId", bndId);
    return mock_c()->returnValue().value.boolValue;
}

bool celix_framework_startBundle(celix_framework_t *fw, long bndId) {
    mock_c()->actualCall("celix_framework_startBundle")
            ->withPointerParameters("fw", fw)
            ->withLongIntParameters("bndId", bndId);
    return mock_c()->returnValue().value.boolValue;
}