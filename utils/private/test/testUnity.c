/*
 * testUnity.c
 *
 *  Created on: Jan 30, 2012
 *      Author: alexander
 */
#include <stdlib.h>

#include "unity.h"
#include "tomock.h"
#include "Mocktomock.h"

void setUp(void) {
}

void tearDown(void) {
}

int FunctionName() {
	FunctionToMock(1, 2);
	return 8;
}

void testVerifyThatUnityIsAwesomeAndWillMakeYourLifeEasier(void) {
	TEST_ASSERT_TRUE(1);
}

void test_FunctionName_WorksProperlyAndAlwaysReturns8(void) {

	FunctionToMock_ExpectAndReturn(1,3, 3);

	TEST_ASSERT_EQUAL(8, FunctionName());
}
