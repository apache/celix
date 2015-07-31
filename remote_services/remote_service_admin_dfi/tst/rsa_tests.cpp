/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "launcher.h"
#include "framework.h"


static void testFindRsaService(void) {
    struct framework *fm = celixLauncher_getFramework();
    CHECK(fm != NULL);

    //TODO get framework bundle context. lookup service -> test service
}

}


TEST_GROUP(RsaDfiTests) {
    void setup() {
    }
};

TEST(RsaDfiTests, FindRsaService) {
    testFindRsaService();
}
