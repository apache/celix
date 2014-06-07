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
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "service_reference_private.h"

celix_status_t serviceReference_create(bundle_pt bundle, service_registration_pt registration, service_reference_pt *reference) {
	mock_c()->actualCall("serviceReference_create")
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("registration", registration)
			->_andPointerOutputParameters("reference", (void **) reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_destroy(service_reference_pt reference) {
	mock_c()->actualCall("serviceReference_destroy")
			->withPointerParameters("reference", reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_invalidate(service_reference_pt reference) {
	mock_c()->actualCall("serviceReference_invalidate")
			->withPointerParameters("reference", reference);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getServiceRegistration(service_reference_pt reference, service_registration_pt *registration) {
	mock_c()->actualCall("serviceReference_getServiceRegistration")
			->withPointerParameters("reference", reference)
			->_andPointerOutputParameters("registration", (void **) registration);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getBundle(service_reference_pt reference, bundle_pt *bundle) {
	mock_c()->actualCall("serviceReference_getBundle");
	return mock_c()->returnValue().value.intValue;
}

bool serviceReference_isAssignableTo(service_reference_pt reference, bundle_pt requester, char * serviceName) {
	mock_c()->actualCall("serviceReference_isAssignableTo");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_getUsingBundles(service_reference_pt reference, array_list_pt *bundles) {
	mock_c()->actualCall("serviceReference_getUsingBundles");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal) {
	mock_c()->actualCall("serviceReference_equals")
		->withPointerParameters("reference", reference)
		->withPointerParameters("compareTo", compareTo)
		->_andIntOutputParameters("equal", (int *) equal);
	return mock_c()->returnValue().value.intValue;
}

unsigned int serviceReference_hashCode(void *referenceP) {
	mock_c()->actualCall("serviceReference_hashCode");
	return mock_c()->returnValue().value.intValue;
}

int serviceReference_equals2(void *reference1, void *reference2) {
	mock_c()->actualCall("serviceReference_equals2");
	return mock_c()->returnValue().value.intValue;
}


