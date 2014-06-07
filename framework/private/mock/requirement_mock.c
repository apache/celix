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
 * requirement_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "requirement.h"

celix_status_t requirement_create(hash_map_pt directives, hash_map_pt attributes, requirement_pt *requirement) {
	mock_c()->actualCall("requirement_create")
			->withPointerParameters("directives", directives)
			->withPointerParameters("attributes", attributes)
			->_andPointerOutputParameters("requirement", (void **) requirement);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t requirement_getVersionRange(requirement_pt requirement, version_range_pt *range) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t requirement_getTargetName(requirement_pt requirement, char **targetName) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t requirement_isSatisfied(requirement_pt requirement, capability_pt capability, bool *inRange) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

