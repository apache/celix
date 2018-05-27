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
 * version_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "version.h"

celix_status_t version_createVersion(int major, int minor, int micro, char * qualifier, version_pt *version) {
	mock_c()->actualCall("version_createVersion")
		->withIntParameters("major", major)
		->withIntParameters("minor", minor)
		->withIntParameters("micro", micro)
		->withStringParameters("qualifier", qualifier)
		->withOutputParameter("version", version);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_destroy(version_pt version) {
    mock_c()->actualCall("version_destroy")
    		->withPointerParameters("version", version);
    return mock_c()->returnValue().value.intValue;
}

celix_status_t version_clone(version_pt version, version_pt *clone) {
	mock_c()->actualCall("version_clone")
		->withPointerParameters("version", version)
		->withOutputParameter("clone", clone);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_createVersionFromString(const char * versionStr, version_pt *version) {
	mock_c()->actualCall("version_createVersionFromString")
			->withStringParameters("versionStr", versionStr)
	      ->withOutputParameter("version", (void **) version);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_createEmptyVersion(version_pt *version) {
	mock_c()->actualCall("version_createEmptyVersion")
			->withOutputParameter("version", (void **) version);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_getMajor(version_pt version, int *major) {
	mock_c()->actualCall("version_getMajor")
			->withPointerParameters("version", version)
			->withOutputParameter("major", major);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_getMinor(version_pt version, int *minor) {
	mock_c()->actualCall("version_getMinor")
			->withPointerParameters("version", version)
			->withOutputParameter("minor", minor);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_getMicro(version_pt version, int *micro) {
	mock_c()->actualCall("version_getMicro")
			->withPointerParameters("version", version)
			->withOutputParameter("micro", micro);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_getQualifier(version_pt version, const char **qualifier) {
	mock_c()->actualCall("version_getQualifier")
			->withPointerParameters("version", version)
			->withOutputParameter("qualifier", qualifier);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t version_compareTo(version_pt version, version_pt compare, int *result) {
    mock_c()->actualCall("version_compareTo")
    		->withPointerParameters("version", version)
    		->withPointerParameters("compare", compare)
    		->withOutputParameter("result", result);
    return CELIX_SUCCESS;
}


celix_status_t version_toString(version_pt version, char **string) {
	mock_c()->actualCall("version_toString")
			->withPointerParameters("version", version)
			->withOutputParameter("string", string);
	return mock_c()->returnValue().value.intValue;
}



