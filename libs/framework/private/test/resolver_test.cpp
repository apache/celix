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
#include "manifest_parser.h"
#include "requirement_private.h"
#include "capability_private.h"
#include "version_private.h"

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
	manifest_pt manifest = (manifest_pt) 0x01;
	manifest_pt manifest2 = (manifest_pt) 0x02;
	manifest_parser_pt parser = (manifest_parser_pt) 0x03;
	manifest_parser_pt parser2 = (manifest_parser_pt) 0x04;
	bundle_pt bundle = (bundle_pt) 0x05;
	version_pt version = (version_pt) malloc(sizeof(*version));
	linked_list_pt capabilities = NULL;
	linked_list_pt requirements = NULL;
	linked_list_pt empty_capabilities = NULL;
	linked_list_pt empty_requirements = NULL;
	char * name = my_strdup("module_one");
	char * name2 = my_strdup("module_two");
	char * id = my_strdup("42");
	char * id2 = my_strdup("43");
	char * service_name = my_strdup("test_service_foo");
	char * service_name2 = my_strdup("test_service_bar");
	requirement_pt req = (requirement_pt) 0x06;
	requirement_pt req2= (requirement_pt) 0x07;
	capability_pt cap = (capability_pt) 0x08;
	capability_pt cap2= (capability_pt) 0x09;

	importer_wires_pt get_importer_wires;
	linked_list_pt get_wire_list;
	linked_list_pt get_wire_list2;

	//creating modules
	linkedList_create(&capabilities);
	linkedList_create(&empty_capabilities);
	linkedList_create(&requirements);
	linkedList_create(&empty_requirements);

	linkedList_addElement(requirements, req);
	linkedList_addElement(requirements, req2);
	linkedList_addElement(capabilities, cap);
	linkedList_addElement(capabilities, cap2);


	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &name, sizeof(name));
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &version, sizeof(version_pt));
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &empty_capabilities, sizeof(empty_capabilities));
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &requirements, sizeof(requirements));
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);

	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", manifest2)
			.withOutputParameterReturning("manifest_parser", &parser2, sizeof(parser2))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser2)
			.withOutputParameterReturning("symbolicName", &name2, sizeof(name2));
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser2)
			.withOutputParameterReturning("version", &version, sizeof(version_pt));
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser2)
			.withOutputParameterReturning("capabilities", &capabilities, sizeof(linked_list_pt));
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser2)
			.withOutputParameterReturning("requirements", &empty_requirements, sizeof(linked_list_pt));
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser2);


	module = module_create(manifest, id, bundle);
	module2 = module_create(manifest2, id2, bundle);

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap)
			.withOutputParameterReturning("serviceName", &service_name, sizeof(service_name));

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap2)
			.withOutputParameterReturning("serviceName", &service_name2, sizeof(service_name2));

	resolver_addModule(module2);

	mock().expectOneCall( "requirement_getTargetName")
			.withParameter("requirement", req)
			.withOutputParameterReturning("targetName", &service_name, sizeof(service_name));

	mock().expectOneCall("requirement_getTargetName")
			.withParameter("requirement", req2)
			.withOutputParameterReturning("targetName", &service_name2, sizeof(service_name2));

	bool out = true;
	mock().expectOneCall("requirement_isSatisfied")
			.withParameter("requirement", req)
			.withParameter("capability", cap)
			.withOutputParameterReturning("inRange", &out, sizeof(out));

	mock().expectOneCall("requirement_isSatisfied")
			.withParameter("requirement", req2)
			.withParameter("capability", cap2)
			.withOutputParameterReturning("inRange", &out, sizeof(out));

	mock().expectNCalls(2, "capability_getModule")
			.withParameter("capability", cap)
			.withOutputParameterReturning("module", &module2, sizeof(module2));

	mock().expectNCalls(2, "capability_getModule")
			.withParameter("capability", cap2)
			.withOutputParameterReturning("module", &module2, sizeof(module2));


	get_wire_list = resolver_resolve(module);
	LONGS_EQUAL(2, linkedList_size(get_wire_list));
	get_wire_list2 = resolver_resolve(module2);
	LONGS_EQUAL(1, linkedList_size(get_wire_list2)); //creates one empty importer wires struct

	get_importer_wires = (importer_wires_pt) linkedList_removeLast(get_wire_list2);
	LONGS_EQUAL(0, linkedList_size(get_importer_wires->wires));
	linkedList_destroy(get_importer_wires->wires);
	free(get_importer_wires);
	linkedList_destroy(get_wire_list2);

	get_importer_wires = (importer_wires_pt) linkedList_removeLast(get_wire_list);
	if ( get_importer_wires->importer == module ) {
		module_setWires(module, get_importer_wires->wires);
		free(get_importer_wires);
		get_importer_wires = (importer_wires_pt) linkedList_removeLast(get_wire_list);
		POINTERS_EQUAL(get_importer_wires->importer, module2);
		module_setWires(module2, get_importer_wires->wires);
		free(get_importer_wires);
	} else {
		POINTERS_EQUAL(get_importer_wires->importer, module2);
		module_setWires(module2, get_importer_wires->wires);
		free(get_importer_wires);
		get_importer_wires = (importer_wires_pt) linkedList_removeLast(get_wire_list);
		POINTERS_EQUAL(get_importer_wires->importer, module);
		module_setWires(module, get_importer_wires->wires);
		free(get_importer_wires);
	}

	//register as resolved
	module_setResolved(module);
	module_setResolved(module2);

	mock().expectNCalls(2, "capability_getServiceName")
			.withParameter("capability", cap)
			.withOutputParameterReturning("serviceName", &service_name, sizeof(service_name));

	mock().expectNCalls(2, "capability_getServiceName")
			.withParameter("capability", cap2)
			.withOutputParameterReturning("serviceName", &service_name2, sizeof(service_name2));

	resolver_moduleResolved(module2);

	//test resolved module checking
	POINTERS_EQUAL(NULL, resolver_resolve(module));

	//CLEAN UP
	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap)
			.withOutputParameterReturning("serviceName", &service_name, sizeof(service_name));

	mock().expectOneCall("capability_getServiceName")
			.withParameter("capability", cap2)
			.withOutputParameterReturning("serviceName", &service_name2, sizeof(service_name2));

	resolver_removeModule(module2);

	mock().expectOneCall("requirement_destroy")
			.withParameter("requirement", req);

	mock().expectOneCall("requirement_destroy")
			.withParameter("requirement", req2);

	mock().expectOneCall("capability_destroy")
			.withParameter("capability", cap);

	mock().expectOneCall("capability_destroy")
			.withParameter("capability", cap2);

	mock().expectNCalls(2, "version_destroy")
			.withParameter("version", version);

	module_destroy(module);
	module_destroy(module2);

	linkedList_destroy(get_wire_list);
	free(id);
	free(id2);
	free(service_name);
	free(service_name2);
	free(version);
}

TEST(resolver, resolve_fail){
	module_pt module;
	manifest_pt manifest = (manifest_pt) 0x01;
	manifest_parser_pt parser = (manifest_parser_pt) 0x03;
	bundle_pt bundle = (bundle_pt) 0x05;
	version_pt version = (version_pt) malloc(sizeof(*version));
	linked_list_pt requirements = NULL;
	linked_list_pt empty_capabilities = NULL;
	char * name = my_strdup("module_one");
	char * id = my_strdup("42");
	char * service_name = my_strdup("test_service_foo");
	requirement_pt req = (requirement_pt) 0x06;
	requirement_pt req2= (requirement_pt) 0x07;
	linked_list_pt get_wire_map;

	//creating modules
	linkedList_create(&empty_capabilities);
	linkedList_create(&requirements);

	linkedList_addElement(requirements, req);
	linkedList_addElement(requirements, req2);


	mock().expectOneCall("manifestParser_create")
			.withParameter("manifest", manifest)
			.withOutputParameterReturning("manifest_parser", &parser, sizeof(parser))
			.ignoreOtherParameters();
	mock().expectOneCall("manifestParser_getSymbolicName")
			.withParameter("parser", parser)
			.withOutputParameterReturning("symbolicName", &name, sizeof(name));
	mock().expectOneCall("manifestParser_getBundleVersion")
			.withParameter("parser", parser)
			.withOutputParameterReturning("version", &version, sizeof(version_pt));
	mock().expectOneCall("manifestParser_getCapabilities")
			.withParameter("parser", parser)
			.withOutputParameterReturning("capabilities", &empty_capabilities, sizeof(empty_capabilities));
	mock().expectOneCall("manifestParser_getCurrentRequirements")
			.withParameter("parser", parser)
			.withOutputParameterReturning("requirements", &requirements, sizeof(requirements));
	mock().expectOneCall("manifestParser_destroy")
			.withParameter("manifest_parser", parser);

	module = module_create(manifest, id, bundle);

	resolver_addModule(module);

	mock().expectOneCall( "requirement_getTargetName")
			.withParameter("requirement", req)
			.withOutputParameterReturning("targetName", &service_name, sizeof(service_name));

	mock().expectOneCall("framework_log");

	get_wire_map = resolver_resolve(module);
	POINTERS_EQUAL(NULL, get_wire_map);

	//cleanup
	resolver_removeModule(module);

	mock().expectOneCall("requirement_destroy")
			.withParameter("requirement", req);

	mock().expectOneCall("requirement_destroy")
			.withParameter("requirement", req2);

	mock().expectOneCall("version_destroy")
			.withParameter("version", version);

	module_destroy(module);

	free(service_name);
	free(version);
	free(id);
}
