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
 * properties_mock.c
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "properties.h"

properties_pt properties_create(void) {
	mock_c()->actualCall("properties_create");
	return mock_c()->returnValue().value.pointerValue;
}

void properties_destroy(properties_pt properties) {
	mock_c()->actualCall("properties_destroy")
			->withPointerParameters("properties", properties);
}

properties_pt properties_load(const char * filename) {
	mock_c()->actualCall("properties_load");
	return mock_c()->returnValue().value.pointerValue;
}

void properties_store(properties_pt properties, const char * file, const char * header) {
	mock_c()->actualCall("properties_store");
}

const char * properties_get(properties_pt properties, const char * key) {
	mock_c()->actualCall("properties_get")
			->withPointerParameters("properties", properties)
			->withStringParameters("key", key);
	return (char *) mock_c()->returnValue().value.stringValue;
}

const char * properties_getWithDefault(properties_pt properties, const char * key, const char * defaultValue) {
	mock_c()->actualCall("properties_get")
			->withPointerParameters("properties", properties)
			->withStringParameters("key", key)
			->withStringParameters("defaultValue", defaultValue);
	return mock_c()->returnValue().value.pointerValue;
}

void properties_set(properties_pt properties, const char * key, const char * value) {
	mock_c()->actualCall("properties_set")
		->withPointerParameters("properties", properties)
		->withStringParameters("key", key)
		->withStringParameters("value", value);
}




