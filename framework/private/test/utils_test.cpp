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
 * utils_test.cpp
 *
 *  \date       Feb 6, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "utils.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(utils) {

	void setup(void) {
	}

	void teardown() {
	}
};

TEST(utils, stringHash) {
	std::string toHash = "abc";
	unsigned int hash;
	hash = utils_stringHash((void *) toHash.c_str());
	LONGS_EQUAL(446371745, hash);

	toHash = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";
	hash = utils_stringHash((void *) toHash.c_str());
	LONGS_EQUAL(1508668412, hash);

	toHash = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";
	hash = utils_stringHash((void *) toHash.c_str());
	LONGS_EQUAL(829630780, hash);
}

TEST(utils, stringEquals) {
	// Compare with equal strings
	std::string org = "abc";
	std::string cmp = "abc";

	int result = utils_stringEquals((void *) org.c_str(), (void *) cmp.c_str());
	CHECK(result);

	// Compare with no equal strings
	cmp = "abcd";

	result = utils_stringEquals((void *) org.c_str(), (void *) cmp.c_str());
	CHECK(!result);

	// Compare with numeric strings
	org = "123";
	cmp = "123";

	result = utils_stringEquals((void *) org.c_str(), (void *) cmp.c_str());
	CHECK(result);

	// Compare with long strings
	org = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";
	cmp = "abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz"
			"abc123def456ghi789jkl012mno345pqr678stu901vwx234yz";

	result = utils_stringEquals((void *) org.c_str(), (void *) cmp.c_str());
	CHECK(result);
}

TEST(utils, stringTrim) {
	// Multiple whitespaces, before, after and in between
	std::string toTrim = " a b c ";
	char *result = utils_stringTrim((char*) toTrim.c_str());

	STRCMP_EQUAL("a b c", result);

	// No whitespaces
	toTrim = "abc";
	result = utils_stringTrim((char*) toTrim.c_str());

	STRCMP_EQUAL("abc", result);

	// Only whitespace before
	toTrim = "               abc";
	result = utils_stringTrim((char*) toTrim.c_str());

	STRCMP_EQUAL("abc", result);

	// Only whitespace after
	toTrim = "abc         ";
	result = utils_stringTrim((char*) toTrim.c_str());

	STRCMP_EQUAL("abc", result);

	// Whitespace other then space (tab, cr..).
	toTrim = "\tabc  \n asdf  \n";
	result = utils_stringTrim((char*) toTrim.c_str());

	STRCMP_EQUAL("abc  \n asdf", result);
}

TEST(utils, isNumeric) {
	// Check numeric string
	std::string toCheck = "42";

	bool result;
	celix_status_t status = utils_isNumeric((char *) toCheck.c_str(), &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(result);

	// Check non numeric string
	toCheck = "42b";
	status = utils_isNumeric((char *) toCheck.c_str(), &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_C(!result);
}


