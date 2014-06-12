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
 * manifest_parser_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "manifest_parser.h"

celix_status_t manifestParser_create(module_pt owner, manifest_pt manifest, manifest_parser_pt *manifest_parser) {
	mock_c()->actualCall("manifestParser_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifestParser_destroy(manifest_parser_pt manifest_parser) {
    mock_c()->actualCall("manifestParser_destroy");
    return mock_c()->returnValue().value.intValue;
}

celix_status_t manifestParser_getSymbolicName(manifest_parser_pt parser, char **symbolicName) {
	mock_c()->actualCall("manifestParser_getSymbolicName");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifestParser_getBundleVersion(manifest_parser_pt parser, version_pt *version) {
	mock_c()->actualCall("manifestParser_getBundleVersion");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifestParser_getCapabilities(manifest_parser_pt parser, linked_list_pt *capabilities) {
	mock_c()->actualCall("manifestParser_getCapabilities");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifestParser_getRequirements(manifest_parser_pt parser, linked_list_pt *requirements) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return mock_c()->returnValue().value.intValue;
}




