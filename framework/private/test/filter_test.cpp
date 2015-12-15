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
 * filter_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "filter_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(filter) {
	void setup(void) {
		logger = (framework_logger_pt) malloc(sizeof(*logger));
		logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();

		free(logger);
	}
};

TEST(filter, create) {
	char filterStr[] = "(key=value)";
	filter_pt filter = filter_create(filterStr);

	STRCMP_EQUAL(filterStr, filter->filterStr);
}


TEST(filter, create1) {
    char filterStr[] = "(key<value)";
    filter_pt filter = filter_create(filterStr);

    STRCMP_EQUAL(filterStr, filter->filterStr);
}

TEST(filter, create2) {
    char filterStr[] = "(key>value)";
    filter_pt filter = filter_create(filterStr);

    STRCMP_EQUAL(filterStr, filter->filterStr);
}

TEST(filter, create3) {
    char filterStr[] = "(key<=value)";
    filter_pt filter = filter_create(filterStr);

    STRCMP_EQUAL(filterStr, filter->filterStr);
}

TEST(filter, create4) {
    char filterStr[] = "(key>=value)";
    filter_pt filter = filter_create(filterStr);

    STRCMP_EQUAL(filterStr, filter->filterStr);
}


