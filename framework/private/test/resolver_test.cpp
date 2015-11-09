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
 * resolver_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "resolver.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s) {
	if (s == NULL) {
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc(len + 1, sizeof(char));

	if (d == NULL) {
		return NULL;
	}

	strncpy(d, s, len);
	return d;
}

TEST_GROUP(resolver) {
	void setup(void){
	}

	void teardown(){
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(resolver, resolve){
	module_pt module;
	module_pt module2;
	manifest_pt actual_manifest = (manifest_pt) 0x01;
	bundle_pt actual_bundle = (bundle_pt) 0x02;
	version_pt actual_version = (version_pt) 0x03;
	linked_list_pt actual_capabilities = NULL;
	linked_list_pt actual_requirements= NULL;
	char * actual_name = my_strdup("module");
	char * actual_id = my_strdup("42");
	char * actual_service_name = my_strdup("test_service_foo");
	char * actual_service_name2 = my_strdup("test_service_bar");
	requirement_pt req = (requirement_pt) 0x04;
	requirement_pt req2= (requirement_pt) 0x05;
	capability_pt cap = (capability_pt) 0x06;
	capability_pt cap2= (capability_pt) 0x07;
	linked_list_pt get;

	//creating module
	linkedList_create(&actual_capabilities);
	linkedList_create(&actual_requirements);

	linkedList_addElement(actual_requirements, req);
	linkedList_addElement(actual_requirements, req2);
	linkedList_addElement(actual_capabilities, cap);
	linkedList_addElement(actual_capabilities, cap2);


	mock().expectNCalls(2, "manifestParser_create");
	mock().expectNCalls(2,"manifestParser_getSymbolicName")
					.withOutputParameterReturning("symbolicName", &actual_name, sizeof(actual_name) );
	mock().expectNCalls(2, "manifestParser_getBundleVersion")
					.withOutputParameterReturning("version", &actual_version, sizeof(version_pt) );
	mock().expectNCalls(2, "manifestParser_getCapabilities")
					.withOutputParameterReturning("capabilities", &actual_capabilities, sizeof(linked_list_pt) );
	mock().expectNCalls(2, "manifestParser_getCurrentRequirements")
					.withOutputParameterReturning("requirements", &actual_requirements, sizeof(linked_list_pt) );
	mock().expectNCalls(2, "manifestParser_destroy");

	module = module_create(actual_manifest, actual_id, actual_bundle);
	module2 = module_create(actual_manifest, actual_id, actual_bundle);

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap)
			.withOutputParameterReturning("serviceName", &actual_service_name, sizeof(actual_service_name));

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap2)
			.withOutputParameterReturning("serviceName", &actual_service_name2, sizeof(actual_service_name2));

	resolver_addModule(module2);

	mock().expectOneCall("requirement_getTargetName")
			.withParameter("requirement", req)
			.withOutputParameterReturning("targetName", &actual_service_name, sizeof(actual_service_name));

	mock().expectOneCall("requirement_getTargetName")
			.withParameter("requirement", req2)
			.withOutputParameterReturning("targetName", &actual_service_name2, sizeof(actual_service_name2));

	bool out = true;
	mock().expectOneCall("requirement_isSatisfied")
			.withParameter("requirement", req)
			.withParameter("capability", cap)
			.withOutputParameterReturning("inRange", &out, sizeof(out));

	mock().expectOneCall("requirement_isSatisfied")
			.withParameter("requirement", req2)
			.withParameter("capability", cap2)
			.withOutputParameterReturning("inRange", &out, sizeof(out));

	mock().expectOneCall("capability_getModule")
			.withParameter("capability", cap)
			.withOutputParameterReturning("module", &module, sizeof(module));

	/*mock().expectOneCall("capability_getModule")
			.withParameter("capability", cap)
			.withOutputParameterReturning("module", &module, sizeof(module));*/

	get = resolver_resolve(module);
	//CHECK(linkedList_contains(get, wire));

	//test resolved module checking

	POINTERS_EQUAL(NULL, resolver_resolve(module));

	mock().expectOneCall("version_destroy");
	module_destroy(module);
	module_destroy(module2);


	free(actual_id);
	free(actual_name);
}
/*
TEST(resolver, moduleResolved){
	module_pt module;
  resolver_moduleResolved(module);
}

TEST(resolver, addModule){
	module_pt module;
	resolver_addModule(module);
}

TEST(resolver, removeModule){
	module_pt module ;

	module = module_create(actual_manifest, actual_id, actual_bundle);

	resolver_removeModule(module);
}
*/




