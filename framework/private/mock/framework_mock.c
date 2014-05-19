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
 * framework_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "framework_private.h"

celix_status_t framework_create(framework_pt *framework, apr_pool_t *memoryPool, properties_pt config) {
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

void framework_stop(framework_pt framework) {
	mock_c()->actualCall("framework_stop");
}

celix_status_t fw_getProperty(framework_pt framework, const char *name, char **value) {
	mock_c()->actualCall("fw_getProperty")
			->withPointerParameters("framework", framework)
			->withStringParameters("name", name)
			->_andStringOutputParameters("value", (const char **) value);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_installBundle(framework_pt framework, bundle_pt * bundle, char * location, char *inputFile) {
	mock_c()->actualCall("fw_installBundle")
			->withPointerParameters("framework", framework)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->_andPointerOutputParameters("bundle", (void **) bundle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_uninstallBundle(framework_pt framework, bundle_pt bundle) {
	mock_c()->actualCall("fw_uninstallBundle")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_getBundleEntry(framework_pt framework, bundle_pt bundle, char *name, apr_pool_t *pool, char **entry) {
	mock_c()->actualCall("framework_getBundleEntry")
			->withPointerParameters("framework", framework)
			->withPointerParameters("bundle", bundle)
			->withStringParameters("name", name)
			->withPointerParameters("pool", pool)
			->_andStringOutputParameters("entry", (const char **) entry);
		return mock_c()->returnValue().value.intValue;
}


celix_status_t fw_startBundle(framework_pt framework, bundle_pt bundle, int options) {
	mock_c()->actualCall("fw_startBundle")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withIntParameters("options", options);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_updateBundle(framework_pt framework, bundle_pt bundle, char *inputFile) {
	mock_c()->actualCall("framework_updateBundle")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("inputFile", inputFile);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_stopBundle(framework_pt framework, bundle_pt bundle, bool record) {
	mock_c()->actualCall("fw_stopBundle")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withIntParameters("record", record);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t fw_registerService(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, char * serviceName, void * svcObj, properties_pt properties) {
	mock_c()->actualCall("fw_registerService")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withPointerParameters("service", svcObj)
		->withPointerParameters("properties", properties)
		->_andPointerOutputParameters("registration", (void **) registration);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_registerServiceFactory(framework_pt framework, service_registration_pt * registration, bundle_pt bundle, char * serviceName, service_factory_pt factory, properties_pt properties) {
	mock_c()->actualCall("fw_registerServiceFactory")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withPointerParameters("serviceFactory", factory)
		->withPointerParameters("properties", properties)
		->_andPointerOutputParameters("registration", (void **) registration);
		return mock_c()->returnValue().value.intValue;
}

void fw_unregisterService(service_registration_pt registration) {
	mock_c()->actualCall("fw_unregisterService");
}


celix_status_t fw_getServiceReferences(framework_pt framework, array_list_pt *references, bundle_pt bundle, const char * serviceName, char * filter) {
	mock_c()->actualCall("fw_getServiceReferences")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withStringParameters("serviceName", serviceName)
		->withStringParameters("filter", filter)
		->_andPointerOutputParameters("references", (void **) references);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, void **service) {
	mock_c()->actualCall("fw_getService")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("reference", reference)
		->_andPointerOutputParameters("service", service);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_ungetService(framework_pt framework, bundle_pt bundle, service_reference_pt reference, bool *result) {
	mock_c()->actualCall("framework_ungetService")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("reference", reference)
		->_andIntOutputParameters("result", (int *) result);
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getBundleRegisteredServices(framework_pt framework, apr_pool_t *pool, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("fw_getBundleRegisteredServices");
		return mock_c()->returnValue().value.intValue;
}

celix_status_t fw_getBundleServicesInUse(framework_pt framework, bundle_pt bundle, array_list_pt *services) {
	mock_c()->actualCall("fw_getBundleServicesInUse");
		return mock_c()->returnValue().value.intValue;
}


void fw_addServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener, char * filter) {
	mock_c()->actualCall("fw_addServiceListener")
		->withPointerParameters("framework", framework)
		->withPointerParameters("bundle", bundle)
		->withPointerParameters("listener", listener)
		->withStringParameters("filter", filter);
}

void fw_removeServiceListener(framework_pt framework, bundle_pt bundle, service_listener_pt listener) {
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

void fw_serviceChanged(framework_pt framework, service_event_type_e eventType, service_registration_pt registration, properties_pt oldprops) {
	mock_c()->actualCall("fw_serviceChanged");
}


celix_status_t fw_isServiceAssignable(framework_pt fw, bundle_pt requester, service_reference_pt reference, bool *assignable) {
	mock_c()->actualCall("fw_isServiceAssignable");
		return mock_c()->returnValue().value.intValue;
}


//bundle_archive_t fw_createArchive(long id, char * location) {

//void revise(bundle_archive_t archive, char * location) {

celix_status_t getManifest(bundle_archive_pt archive, apr_pool_t *pool, manifest_pt *manifest) {
	mock_c()->actualCall("getManifest")
			->withPointerParameters("archive", archive)
			->withPointerParameters("pool", pool)
			->_andPointerOutputParameters("manifest", (void **) manifest);
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
	mock_c()->actualCall("listToArray");
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

bundle_pt framework_getBundle(framework_pt framework, char * location) {
	mock_c()->actualCall("framework_getBundle");
		return mock_c()->returnValue().value.pointerValue;
}

bundle_pt framework_getBundleById(framework_pt framework, long id) {
	mock_c()->actualCall("framework_getBundleById")
		->withPointerParameters("framework", framework)
		->withIntParameters("id", id);
	return mock_c()->returnValue().value.pointerValue;
}


celix_status_t framework_getMemoryPool(framework_pt framework, apr_pool_t **pool) {
	mock_c()->actualCall("framework_getMemoryPool")
			->withPointerParameters("framework", framework)
			->_andPointerOutputParameters("pool", (void **) pool);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t framework_getFrameworkBundle(framework_pt framework, bundle_pt *bundle) {
	mock_c()->actualCall("framework_getFrameworkBundle");
	return mock_c()->returnValue().value.intValue;
}




