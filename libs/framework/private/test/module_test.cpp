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
 * module_test.cpp
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
#include "module.h"

#include "manifest_parser.h"
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

TEST_GROUP(module) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(module, create){
	module_pt module = NULL;
	manifest_pt actual_manifest = (manifest_pt) 0x01;
	manifest_parser_pt parser = (manifest_parser_pt) 0x02;
	bundle_pt actual_bundle = (bundle_pt) 0x03;
	version_pt actual_version = (version_pt) 0x04;
	linked_list_pt actual_capabilities = NULL;
	linked_list_pt actual_requirements= NULL;
	char * actual_name = my_strdup("module");
	char * actual_id = my_strdup("42");

	linkedList_create(&actual_capabilities);
	linkedList_create(&actual_requirements);
	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", actual_manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &actual_name, sizeof(actual_name) );
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &actual_version, sizeof(actual_version) );
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &actual_capabilities, sizeof(actual_capabilities) );
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &actual_requirements, sizeof(actual_requirements) );
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);

	module = module_create(actual_manifest, actual_id, actual_bundle);
	CHECK(module != NULL);

	mock().expectOneCall("version_destroy")
			.withParameter("version", actual_version);
	module_destroy(module);

	free(actual_id);
}

TEST(module, createFrameworkModule){
	module_pt module = NULL;
	bundle_pt actual_bundle = (bundle_pt) 0x01;
	version_pt actual_version = (version_pt) 0x02;

	mock().expectOneCall("version_createVersion")
			.withParameter("major", 1)
			.withParameter("minor", 0)
			.withParameter("micro", 0)
			.withParameter("qualifier", "")
			.withOutputParameterReturning("version", &actual_version, sizeof(actual_version));

	module = module_createFrameworkModule(actual_bundle);

	CHECK(module != NULL);

	mock().expectOneCall("version_destroy")
			.withParameter("version", actual_version);

	module_destroy(module);
}

TEST(module, resolved){
	module_pt module = NULL;
	manifest_pt actual_manifest = (manifest_pt) 0x01;
	manifest_parser_pt parser = (manifest_parser_pt) 0x02;
	bundle_pt actual_bundle = (bundle_pt) 0x03;
	version_pt actual_version = (version_pt) 0x04;
	linked_list_pt actual_capabilities = NULL;
	linked_list_pt actual_requirements= NULL;
	char * actual_name = my_strdup("module");
	char * actual_id = my_strdup("42");

	linkedList_create(&actual_capabilities);
	linkedList_create(&actual_requirements);
	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", actual_manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &actual_name, sizeof(actual_name) );
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &actual_version, sizeof(actual_version) );
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &actual_capabilities, sizeof(actual_capabilities) );
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &actual_requirements, sizeof(actual_requirements) );
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);
	module = module_create(actual_manifest, actual_id, actual_bundle);

	CHECK_FALSE(module_isResolved(module));
	module_setResolved(module);
	CHECK(module_isResolved(module));

	mock().expectOneCall("version_destroy")
			.withParameter("version", actual_version);
	module_destroy(module);

	free(actual_id);
}

TEST(module, wires){
	manifest_pt manifest = (manifest_pt) 0x01;
	manifest_parser_pt parser = (manifest_parser_pt) 0x02;
	bundle_pt bundle = (bundle_pt) 0x03;
	version_pt version = (version_pt) 0x04;
	wire_pt wire = (wire_pt) 0x05;
	wire_pt wire_new = (wire_pt) 0x06;
	capability_pt cap = (capability_pt) 0x07;
	requirement_pt req = (requirement_pt) 0x08;
	char * service_name = my_strdup("foobar");

	//test var declarations
	array_list_pt get_list = NULL;

	//module related declares
	module_pt module = NULL;
	char * name = my_strdup("module_1");
	char * id = my_strdup("42");
	linked_list_pt capabilities = NULL;
	linked_list_pt requirements = NULL;
	linked_list_pt wires = NULL;
	linked_list_pt wires_new = NULL;

	//module2 related declares
	module_pt module2 = NULL;
	char * name2 = my_strdup("module_2");
	char * id2 = my_strdup("43");
	linked_list_pt capabilities2 = NULL;
	linked_list_pt requirements2 = NULL;

	//create module
	linkedList_create(&capabilities);
	linkedList_create(&requirements);
	linkedList_create(&wires);
	linkedList_addElement(capabilities, cap);
	linkedList_addElement(requirements, req);
	linkedList_addElement(wires, wire);

	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &name, sizeof(name) );
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &version, sizeof(version) );
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &capabilities, sizeof(capabilities) );
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &requirements, sizeof(requirements) );
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);
	module = module_create(manifest, id, bundle);

	//create module2
	linkedList_create(&capabilities2);
	linkedList_create(&requirements2);
	linkedList_create(&wires_new);
	linkedList_addElement(wires_new, wire_new);

	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &name2, sizeof(name2) );
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &version, sizeof(version) );
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &capabilities2, sizeof(capabilities2) );
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &requirements2, sizeof(requirements2) );
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);
	module2 = module_create(manifest, id2, bundle);

	//test empty wires handling
	POINTERS_EQUAL(NULL, module_getWire(module, service_name));

	//expect the adding of wire
	mock().expectOneCall("wire_getExporter")
					.withParameter("wire", wire)
					.withOutputParameterReturning("exporter", &module2, sizeof(module2));

	//set modules->wires = actual_wires, and register module dependent at module2
	module_setWires(module, wires);

	//expect getting of wire vars
	mock().expectOneCall("wire_getCapability")
			.withParameter("wire", wire)
			.withOutputParameterReturning("capability", &cap, sizeof(cap));

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap)
			.withOutputParameterReturning("serviceName", &service_name, sizeof(service_name));

	//test for added wire
	POINTERS_EQUAL(wire, module_getWire(module, service_name));

	//test added dependent module (method 1 of 2)
	get_list = module_getDependents(module2);
	CHECK(arrayList_contains(get_list, module));

	//expect the re-adding of wire
	//expect the adding of wire
	mock().expectOneCall("wire_getExporter")
					.withParameter("wire", wire)
					.withOutputParameterReturning("exporter", &module2, sizeof(module2));

	mock().expectOneCall("wire_destroy")
			.withParameter("wire", wire);

	mock().expectOneCall("wire_getExporter")
					.withParameter("wire", wire_new)
					.withOutputParameterReturning("exporter", &module2, sizeof(module2));

	//test clearing of the wires before adding back wire
	module_setWires(module, wires_new);

	//test added dependent module (method 2 of 2)
	CHECK(arrayList_contains(module_getDependentImporters(module2),module));

	//check getwires
	POINTERS_EQUAL(wires_new, module_getWires(module));

	//TODO make tests for possible implementations of the following functions
	module_addDependentRequirer(module, module2);
	module_getDependentRequirers(module);
	module_removeDependentRequirer(module, module2);

	//clean up
	mock().expectNCalls(2, "version_destroy")
			.withParameter("version", version);

	mock().expectOneCall("wire_destroy")
			.withParameter("wire", wire_new);

	mock().expectOneCall("capability_destroy")
			.withParameter("capability", cap);

	mock().expectOneCall("requirement_destroy")
			.withParameter("requirement", req);
	module_destroy(module);
	module_destroy(module2);

	arrayList_destroy(get_list);
	free(id);
	free(id2);
	free(service_name);
}

TEST(module, get){
	module_pt module = NULL;
	manifest_pt actual_manifest = (manifest_pt) 0x01;
	manifest_parser_pt parser = (manifest_parser_pt) 0x02;
	bundle_pt actual_bundle = (bundle_pt) 0x03;
	version_pt actual_version = (version_pt) 0x04;
	linked_list_pt actual_capabilities = NULL;
	linked_list_pt actual_requirements= NULL;
	char * actual_name = my_strdup("module");
	char * actual_id = my_strdup("42");
	char * get = NULL;

	linkedList_create(&actual_capabilities);
	linkedList_create(&actual_requirements);

	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", actual_manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &actual_name, sizeof(actual_name) );
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &actual_version, sizeof(actual_version) );
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &actual_capabilities, sizeof(actual_capabilities) );
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &actual_requirements, sizeof(actual_requirements) );
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);


	module = module_create(actual_manifest, actual_id, actual_bundle);

	CHECK(module != NULL);

	POINTERS_EQUAL(actual_bundle, module_getBundle(module));
	POINTERS_EQUAL(actual_requirements, module_getRequirements(module));
	POINTERS_EQUAL(actual_capabilities, module_getCapabilities(module));
	POINTERS_EQUAL(actual_version, module_getVersion(module));

	STRCMP_EQUAL(actual_id, module_getId(module));

	LONGS_EQUAL(CELIX_SUCCESS, module_getSymbolicName(module, &get));
	STRCMP_EQUAL(actual_name, get);

	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, module_getSymbolicName(NULL, &get));

	mock().expectOneCall("version_destroy")
			.withParameter("version", actual_version);

	module_destroy(module);

	free(actual_id);
}
