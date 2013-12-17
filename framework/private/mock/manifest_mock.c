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
 * manifest_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "manifest.h"

celix_status_t manifest_create(apr_pool_t *pool, manifest_pt *manifest) {
	mock_c()->actualCall("manifest_create")
        ->withPointerParameters("pool", pool)
        ->_andPointerOutputParameters("manifest", (void **) manifest);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifest_createFromFile(apr_pool_t *pool, char *filename, manifest_pt *manifest) {
    mock_c()->actualCall("manifest_createFromFile")
        ->withPointerParameters("pool", pool)
        ->withStringParameters("filename", filename)
        ->_andPointerOutputParameters("manifest", (void **) manifest);
	return mock_c()->returnValue().value.intValue;
}

void manifest_clear(manifest_pt manifest) {
	mock_c()->actualCall("manifest_clear");
}

properties_pt manifest_getMainAttributes(manifest_pt manifest) {
	mock_c()->actualCall("manifest_getMainAttributes");
	return mock_c()->returnValue().value.pointerValue;
}

celix_status_t manifest_getEntries(manifest_pt manifest, hash_map_pt *map) {
	mock_c()->actualCall("manifest_getEntries");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t manifest_read(manifest_pt manifest, char *filename) {
	mock_c()->actualCall("manifest_read");
	return mock_c()->returnValue().value.intValue;
}

void manifest_write(manifest_pt manifest, char * filename) {
	mock_c()->actualCall("bundle_getCurrentModule");
}

char * manifest_getValue(manifest_pt manifest, const char * name) {
	mock_c()->actualCall("bundle_getCurrentModule");
	return (char *) mock_c()->returnValue().value.stringValue;
}


