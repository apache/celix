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
 * filter_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTestExt/MockSupport_c.h"

#include "filter.h"

filter_pt filter_create(const char * filterString) {
	mock_c()->actualCall("filter_create")
			->withStringParameters("filterString", filterString);
	return mock_c()->returnValue().value.pointerValue;
}

void filter_destroy(filter_pt filter) {
	mock_c()->actualCall("filter_destroy")
			->withPointerParameters("filter", filter);
}

celix_status_t filter_match(filter_pt filter, properties_pt properties, bool *result) {
	mock_c()->actualCall("filter_match")
			->withPointerParameters("filter", filter)
			->withPointerParameters("properties", properties)
			->withOutputParameter("result", result);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t filter_getString(filter_pt filter, const char **filterStr) {
	mock_c()->actualCall("filter_getString")
			->withPointerParameters("filter", filter)
			->withOutputParameter("filterStr", filterStr);
	return mock_c()->returnValue().value.intValue;
}
