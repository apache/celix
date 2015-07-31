/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"


extern "C" {
    #include <stdio.h>

    #include "launcher.h"
    #include "framework.h"

    static int startCelix(void) {
        celixLauncher_launch("config.properties");
    }

    static int stopCelix(void) {
        celixLauncher_stop();
        celixLauncher_waitForShutdown();
    }
}

int main(int argc, char** argv) {
    startCelix();
    int rc = RUN_ALL_TESTS(argc, argv);
    stopCelix();
    return rc;
}