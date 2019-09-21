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
 * version_range_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTestExt/MockSupport_c.h"

#include "version_range.h"

celix_status_t versionRange_createVersionRange(version_pt low, bool isLowInclusive, version_pt high, bool isHighInclusive, version_range_pt *versionRange) {
	mock_c()->actualCall("versionRange_createVersionRange")
		->withPointerParameters("low", low)
		->withIntParameters("isLowInclusive", isLowInclusive)
		->withPointerParameters("high", high)
		->withIntParameters("isHighInclusive", isHighInclusive)
		->withOutputParameter("versionRange", versionRange);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t versionRange_destroy(version_range_pt range) {
	mock_c()->actualCall("versionRange_destroy")
		->withPointerParameters("range", range);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t versionRange_createInfiniteVersionRange(version_range_pt *range) {
	mock_c()->actualCall("versionRange_createInfiniteVersionRange")
	        ->withOutputParameter("range", (void **) range);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t versionRange_isInRange(version_range_pt versionRange, version_pt version, bool *inRange) {
	mock_c()->actualCall("versionRange_isInRange")
			->withPointerParameters("versionRange", versionRange)
			->withPointerParameters("version", version)
			->withOutputParameter("inRange", (int *) inRange);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t versionRange_parse(const char * rangeStr, version_range_pt *range) {
	mock_c()->actualCall("versionRange_parse")
        ->withStringParameters("rangeStr", rangeStr)
        ->withOutputParameter("range", (void **) range);
	return mock_c()->returnValue().value.intValue;
}



