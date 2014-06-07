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
 * bundle_cache_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_cache.h"

celix_status_t bundleCache_create(properties_pt configurationMap, framework_logger_pt logger, bundle_cache_pt *bundle_cache) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_getArchives(bundle_cache_pt cache, array_list_pt *archives) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_createArchive(bundle_cache_pt cache, long id, char * location, char *inputFile, bundle_archive_pt *archive) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_delete(bundle_cache_pt cache) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return mock_c()->returnValue().value.intValue;
}




