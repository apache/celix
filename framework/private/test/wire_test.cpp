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
 * wire_test.cpp
 *
 *  \date       Sep 25, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
#include "celix_log.h"
#include "celix_errno.h"

#include "wire.h"
#include "module.h"
#include "requirement.h"
#include "capability.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(wire) {

	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};


TEST(wire, create) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

    module_pt module = (module_pt) 0x01;
    capability_pt cap = (capability_pt) 0x02;
    requirement_pt req = (requirement_pt) 0x03;
    wire_pt wire = NULL;

    LONGS_EQUAL(CELIX_SUCCESS, wire_create(module, req, module, cap, &wire));

    LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, wire_create(module, req, module, cap, &wire));

    LONGS_EQUAL(CELIX_SUCCESS, wire_destroy(wire));
}

TEST(wire, get){
    module_pt importer = (module_pt) 0x01;
    module_pt exporter = (module_pt) 0x02;
    capability_pt cap = (capability_pt) 0x03;
    requirement_pt req = (requirement_pt) 0x04;
    void * get;

    wire_pt wire = NULL;

    wire_create(importer, req, exporter, cap, &wire);

    wire_getImporter(wire, (module_pt*)&get);
	POINTERS_EQUAL(importer, get);

    wire_getExporter(wire, (module_pt*)&get);
	POINTERS_EQUAL(exporter, get);

    wire_getCapability(wire, (capability_pt*)&get);
	POINTERS_EQUAL(cap, get);

    wire_getRequirement(wire, (requirement_pt*)&get);
	POINTERS_EQUAL(req, get);

	wire_destroy(wire);
}
