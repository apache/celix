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
 * service_reference_mock.c
 *
 *  \date       Feb 6, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "service_reference_private.h"

celix_status_t serviceReference_create(registry_callback_t callback, bundle_pt referenceOwner, service_registration_pt registration, service_reference_pt *reference) {
	mock_c()->actualCall("serviceReference_create")
			->withParameterOfType("registry_callback_t", "callback", &callback)
			->withPointerParameters("referenceOwner", referenceOwner)
			->withPointerParameters("registration", registration)
			->withOutputParameter("reference", (void **) reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_retain(service_reference_pt ref) {
    mock_c()->actualCall("serviceReference_retain")
            ->withPointerParameters("ref", ref);
    return mock_c()->returnValue().value.intValue;
}
celix_status_t serviceReference_release(service_reference_pt ref, bool *destroyed) {
    mock_c()->actualCall("serviceReference_release")
            ->withPointerParameters("ref", ref)
            ->withOutputParameter("destroyed", destroyed);
    return mock_c()->returnValue().value.intValue;
}


celix_status_t serviceReference_increaseUsage(service_reference_pt ref, size_t *updatedCount){
	mock_c()->actualCall("serviceReference_increaseUsage")
			->withPointerParameters("reference", ref)
			->withOutputParameter("updatedCount", updatedCount);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_decreaseUsage(service_reference_pt ref, size_t *updatedCount){
	mock_c()->actualCall("serviceReference_decreaseUsage")
			->withPointerParameters("ref", ref)
			->withOutputParameter("updatedCount", updatedCount);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getUsageCount(service_reference_pt reference, size_t *count){
	mock_c()->actualCall("serviceReference_getUsageCount")
			->withPointerParameters("reference", reference)
			->withOutputParameter("count", count);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getReferenceCount(service_reference_pt reference, size_t *count){
	mock_c()->actualCall("serviceReference_getReferenceCount")
			->withPointerParameters("reference", reference)
			->withOutputParameter("count", count);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_setService(service_reference_pt ref, const void *service){
	mock_c()->actualCall("serviceReference_setService")
			->withPointerParameters("ref", ref)
			->withPointerParameters("service", (void *)service);
	return mock_c()->returnValue().value.intValue;
}
celix_status_t serviceReference_getService(service_reference_pt reference, void **service){
	mock_c()->actualCall("serviceReference_getService")
			->withPointerParameters("reference", reference)
			->withOutputParameter("service", service);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getBundle(service_reference_pt reference, bundle_pt *bundle) {
	mock_c()->actualCall("serviceReference_getBundle")
		->withPointerParameters("reference", reference)
		->withOutputParameter("bundle", bundle);
	return mock_c()->returnValue().value.intValue;
}


celix_status_t serviceReference_getOwner(service_reference_pt reference, bundle_pt *owner) {
    mock_c()->actualCall("serviceReference_getOwner")
        ->withPointerParameters("reference", reference)
        ->withOutputParameter("owner", owner);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getServiceRegistration(service_reference_pt reference, service_registration_pt *registration) {
	mock_c()->actualCall("serviceReference_getServiceRegistration")
			->withPointerParameters("reference", reference)
			->withOutputParameter("registration", (void **) registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getProperty(service_reference_pt reference, const char *key, const char** value){
	mock_c()->actualCall("serviceReference_getProperty")
			->withPointerParameters("reference", reference)
			->withStringParameters("key", key)
			->withOutputParameter("value", value);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getPropertyKeys(service_reference_pt reference, char **keys[], unsigned int *size){
	mock_c()->actualCall("serviceReference_getPropertyKeys")
			->withPointerParameters("reference", reference)
			->withOutputParameter("keys", keys)
			->withOutputParameter("size", size);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_invalidate(service_reference_pt reference) {
	mock_c()->actualCall("serviceReference_invalidate")
			->withPointerParameters("reference", reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_isValid(service_reference_pt reference, bool *result) {
	mock_c()->actualCall("serviceReference_isValid")
			->withPointerParameters("reference", reference)
			->withOutputParameter("result", result);
	return mock_c()->returnValue().value.intValue;
}

bool serviceReference_isAssignableTo(service_reference_pt reference, bundle_pt requester, const char * serviceName) {
	mock_c()->actualCall("serviceReference_isAssignableTo")
			->withPointerParameters("reference", reference)
			->withPointerParameters("requester", requester)
			->withStringParameters("serviceName", serviceName);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getUsingBundles(service_reference_pt reference, array_list_pt *bundles) {
	mock_c()->actualCall("serviceReference_getUsingBundles")
		->withPointerParameters("reference", reference)
		->withOutputParameter("bundles", bundles);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal) {
	mock_c()->actualCall("serviceReference_equals")
		->withPointerParameters("reference", reference)
		->withPointerParameters("compareTo", compareTo)
		->withOutputParameter("equal", equal);
	return mock_c()->returnValue().value.intValue;
}

int serviceReference_equals2(const void *reference1, const void *reference2) {
	mock_c()->actualCall("serviceReference_equals2")
			->withPointerParameters("reference1", (void*)reference1)
			->withPointerParameters("reference2", (void*)reference2);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_compareTo(service_reference_pt reference, service_reference_pt compareTo, int *compare){
	mock_c()->actualCall("serviceReference_compareTo")
			->withPointerParameters("reference", reference)
			->withPointerParameters("compareTo", compareTo)
			->withOutputParameter("compare", compare);
	return mock_c()->returnValue().value.intValue;
}

unsigned int serviceReference_hashCode(const void *referenceP) {
	mock_c()->actualCall("serviceReference_hashCode");
	return mock_c()->returnValue().value.intValue;
}
