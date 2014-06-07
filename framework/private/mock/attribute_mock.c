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
 * attribute_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "attribute.h"

celix_status_t attribute_create(char * key, char * value, attribute_pt *attribute) {
	mock_c()->actualCall("attribute_create")
			->withStringParameters("key", key)
			->withStringParameters("value", value)
			->_andPointerOutputParameters("attribute", (void **) attribute);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t attribute_getKey(attribute_pt attribute, char **key) {
	mock_c()->actualCall("attribute_getKey")
			->withPointerParameters("attribute", attribute)
			->_andStringOutputParameters("key", (const char **) key);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t attribute_getValue(attribute_pt attribute, char **value) {
	mock_c()->actualCall("attribute_getValue")
			->withPointerParameters("attribute", attribute)
			->_andStringOutputParameters("value", (const char **) value);
	return mock_c()->returnValue().value.intValue;
}



