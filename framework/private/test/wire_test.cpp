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
 * version_test.cpp
 *
 *  \date       Dec 18, 2012
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "celix_log.h"
#include "celix_errno.h"

#include "wire.h"
#include "module.h"
#include "requirement.h"
#include "capability.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(wire) {

	void setup(void) {
	    logger = (framework_logger_pt) malloc(sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
	    free(logger);
	}
};


TEST(wire, create) {
    module_pt module = (module_pt) 0x01;
    capability_pt cap = (capability_pt) 0x02;
    requirement_pt req = (requirement_pt) 0x03;

    wire_pt wire = NULL;

    wire_create(module, req, module, cap, &wire);

	LONGS_EQUAL(1, 1);

	wire_destroy(wire);
}




