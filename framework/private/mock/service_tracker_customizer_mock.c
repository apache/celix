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
 * service_tracker_customizer_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "service_tracker_customizer_private.h"

celix_status_t serviceTrackerCustomizer_create(void *handle,
		adding_callback_pt addingFunction, added_callback_pt addedFunction,
		modified_callback_pt modifiedFunction, removed_callback_pt removedFunction,
		service_tracker_customizer_pt *customizer) {
	mock_c()->actualCall("serviceTrackerCustomizer_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceTrackerCustomizer_getHandle(service_tracker_customizer_pt customizer, void **handle) {
	mock_c()->actualCall("serviceTrackerCustomizer_getHandle")
			->withPointerParameters("customizer", customizer)
			->_andPointerOutputParameters("handle", handle);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceTrackerCustomizer_getAddingFunction(service_tracker_customizer_pt customizer, adding_callback_pt *function) {
	mock_c()->actualCall("serviceTrackerCustomizer_getAddingFunction")
		->withPointerParameters("customizer", customizer)
		->_andPointerOutputParameters("function", (void **) function);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceTrackerCustomizer_getAddedFunction(service_tracker_customizer_pt customizer, added_callback_pt *function) {
	mock_c()->actualCall("serviceTrackerCustomizer_getAddedFunction")
		->withPointerParameters("customizer", customizer)
		->_andPointerOutputParameters("function", (void **) function);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceTrackerCustomizer_getModifiedFunction(service_tracker_customizer_pt customizer, modified_callback_pt *function) {
	mock_c()->actualCall("serviceTrackerCustomizer_getModifiedFunction")
			->withPointerParameters("customizer", customizer)
			->_andPointerOutputParameters("function", (void **) function);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceTrackerCustomizer_getRemovedFunction(service_tracker_customizer_pt customizer, removed_callback_pt *function) {
	mock_c()->actualCall("serviceTrackerCustomizer_getRemovedFunction")
			->withPointerParameters("customizer", customizer)
			->_andPointerOutputParameters("function", (void **) function);
	return mock_c()->returnValue().value.intValue;
}



