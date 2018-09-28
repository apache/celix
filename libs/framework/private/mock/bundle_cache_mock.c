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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "bundle_cache.h"

celix_status_t bundleCache_create(const char *fwUUID, properties_pt configurationMap, bundle_cache_pt *bundle_cache) {
	mock_c()->actualCall("bundleCache_create")
		->withPointerParameters("configurationMap", configurationMap)
		->withOutputParameter("bundle_cache", bundle_cache);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_destroy(bundle_cache_pt *cache) {
	mock_c()->actualCall("bundleCache_destroy");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_getArchives(bundle_cache_pt cache, array_list_pt *archives) {
	mock_c()->actualCall("bundleCache_getArchives")
			->withPointerParameters("cache", cache)
			->withOutputParameter("archives", archives);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_createArchive(bundle_cache_pt cache, long id, const char * location, const char *inputFile, bundle_archive_pt *archive) {
	mock_c()->actualCall("bundleCache_createArchive")
			->withPointerParameters("cache", cache)
			->withLongIntParameters("id", id)
			->withStringParameters("location", location)
			->withStringParameters("inputFile", inputFile)
			->withOutputParameter("archive", archive);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t bundleCache_delete(bundle_cache_pt cache) {
	mock_c()->actualCall("bundleCache_delete");
	return mock_c()->returnValue().value.intValue;
}




