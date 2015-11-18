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
 * manifest_parser_test.cpp
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
#include "constants.h"
#include "manifest_parser.h"
#include "attribute.h"
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

TEST_GROUP(manifest_parser) {
	void setup(void) {
	}

	void teardown() {
	}
};

TEST(manifest_parser, create){
	module_pt owner = (module_pt) 0x01;
	manifest_pt manifest = (manifest_pt) 0x02;
	manifest_parser_pt parser;
	char * service_name_key = my_strdup("service");
	char * service_version_key = my_strdup("version");

	//get generic bundle data
	char * version_str = my_strdup("1.2.3");
	char * bundle_name = my_strdup("Test_Bundle");
	version_pt version = (version_pt) 0x03;

	mock().expectOneCall("manifest_getValue")
			.withParameter("name", OSGI_FRAMEWORK_BUNDLE_VERSION)
			.andReturnValue(version_str);

	mock().expectOneCall("version_createVersionFromString")
			.withParameter("versionStr", version_str)
			.withOutputParameterReturning("version", &version, sizeof(version));

	mock().expectOneCall("manifest_getValue")
			.withParameter("name", OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME)
			.andReturnValue(bundle_name);

	//create capabilities
	char * export_header = my_strdup("export_service_name;version=\"[4.5,6)\",export_service_name2;version=\"[6.5,4)\"");
	char * cap_name	= my_strdup("export_service_name");
	char * cap_name2	= my_strdup("export_service_name2");
	capability_pt cap = (capability_pt) 0x04;
	capability_pt cap2 = (capability_pt) 0x05;
	attribute_pt attribute_cap_name = (attribute_pt) 0x06;
	attribute_pt attribute_cap_version = (attribute_pt) 0x07;
	attribute_pt attribute_cap_name2 = (attribute_pt) 0x08;
	attribute_pt attribute_cap_version2 = (attribute_pt) 0x09;

	mock().expectOneCall("manifest_getValue")
			.withParameter("name", OSGI_FRAMEWORK_EXPORT_LIBRARY)
			.andReturnValue(export_header);

	//make cap 1
	mock().expectOneCall("attribute_create")
			.withParameter("key", service_name_key)
			.withParameter("value", cap_name)
			.withOutputParameterReturning("attribute", &attribute_cap_name, sizeof(attribute_cap_name));

	mock().expectOneCall("attribute_create")
			.withParameter("key", service_version_key)
			.withParameter("value", "[4.5,6)")
			.withOutputParameterReturning("attribute", &attribute_cap_version, sizeof(attribute_cap_version));

	mock().expectOneCall("attribute_getKey")
			.withParameter("attribute", attribute_cap_name)
			.withOutputParameterReturning("key", &service_name_key, sizeof(service_name_key));

	//make cap2
	mock().expectOneCall("attribute_create")
			.withParameter("key", service_name_key)
			.withParameter("value", cap_name2)
			.withOutputParameterReturning("attribute", &attribute_cap_name2, sizeof(attribute_cap_name2));

	mock().expectOneCall("attribute_create")
			.withParameter("key", service_version_key)
			.withParameter("value", "[6.5,4)")
			.withOutputParameterReturning("attribute", &attribute_cap_version2, sizeof(attribute_cap_version2));

	mock().expectOneCall("attribute_getKey")
			.withParameter("attribute", attribute_cap_name2)
			.withOutputParameterReturning("key", &service_name_key, sizeof(service_name_key));

	//create requirements
	char * import_header = my_strdup("import_service_name;version=\"[7.8,9)\",import_service_name2;version=\"[9.8,7)\"");
	char * req_name	= my_strdup("import_service_name");
	char * req_name2 = my_strdup("import_service_name2");
	requirement_pt req = (requirement_pt) 0x0A;
	requirement_pt req2 = (requirement_pt) 0x0B;
	attribute_pt attribute_req_name = (attribute_pt) 0x0C;
	attribute_pt attribute_req_name2 = (attribute_pt) 0x0D;
	attribute_pt attribute_req_version = (attribute_pt) 0x0E;
	attribute_pt attribute_req_version2 = (attribute_pt) 0x0F;

	mock().expectOneCall("manifest_getValue")
			.withParameter("name", OSGI_FRAMEWORK_IMPORT_LIBRARY)
			.andReturnValue(import_header);

	//make req 1
	mock().expectOneCall("attribute_create")
			.withParameter("key", service_name_key)
			.withParameter("value", req_name)
			.withOutputParameterReturning("attribute", &attribute_req_name, sizeof(attribute_req_name));

	mock().expectOneCall("attribute_create")
			.withParameter("key", service_version_key)
			.withParameter("value", "[7.8,9)")
			.withOutputParameterReturning("attribute", &attribute_req_version, sizeof(attribute_req_version));

	mock().expectOneCall("attribute_getKey")
			.withParameter("attribute", attribute_req_name)
			.withOutputParameterReturning("key", &service_name_key, sizeof(service_name_key));

	//make req 2
	mock().expectOneCall("attribute_create")
			.withParameter("key", service_name_key)
			.withParameter("value", req_name2)
			.withOutputParameterReturning("attribute", &attribute_req_name2, sizeof(attribute_req_name2));

	mock().expectOneCall("attribute_create")
			.withParameter("key", service_version_key)
			.withParameter("value", "[9.8,7)")
			.withOutputParameterReturning("attribute", &attribute_req_version2, sizeof(attribute_req_version2));

	mock().expectOneCall("attribute_getKey")
			.withParameter("attribute", attribute_req_name2)
			.withOutputParameterReturning("key", &service_name_key, sizeof(service_name_key));


	//actual call to create
	manifestParser_create(owner, manifest, &parser);

	//test getters
	version_pt version_clone = (version_pt) 0x06;
	version_pt get_version;
	linked_list_pt get_caps;
	linked_list_pt get_reqs;
	char * get_bundle_name;
	celix_status_t status;

	mock().expectOneCall("version_clone")
			.withParameter("version", version)
			.withOutputParameterReturning("clone", &version_clone, sizeof(version_clone));

	status = manifestParser_getBundleVersion(parser, &get_version);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(version_clone, get_version);

	status = manifestParser_getCapabilities(parser, &get_caps);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(linkedList_contains(get_caps, cap));
	CHECK(linkedList_contains(get_caps, cap2));
	LONGS_EQUAL(2, linkedList_size(get_caps));

	status = manifestParser_getRequirements(parser, &get_reqs);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(linkedList_contains(get_reqs, req));
	CHECK(linkedList_contains(get_reqs, req2));
	LONGS_EQUAL(2, linkedList_size(get_reqs));

	status = manifestParser_getSymbolicName(parser, &get_bundle_name);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL(bundle_name, get_bundle_name);


	//cleanup
	mock().expectOneCall("version_destroy")
			.withParameter("version", version);

	manifestParser_destroy(parser);

	capability_destroy((capability_pt) linkedList_get(get_caps, 0));
	capability_destroy((capability_pt) linkedList_get(get_caps, 1));

	requirement_destroy((requirement_pt) linkedList_get(get_reqs, 0));
	requirement_destroy((requirement_pt) linkedList_get(get_reqs, 1));

	linkedList_destroy(get_caps);
	linkedList_clear(get_reqs);
	linkedList_destroy(get_reqs);

	free(service_name_key);
	free(service_version_key);

	free(version_str);
	free(bundle_name);

	free(export_header);
	free(cap_name);
	free(cap_name2);

	free(import_header);
	free(req_name);
	free(req_name2);

	free(get_bundle_name);

	mock().checkExpectations();
	mock().clear();
}

