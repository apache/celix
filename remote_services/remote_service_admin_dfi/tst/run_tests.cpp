/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"

#include <stdio.h>

#include "launcher.h"


int main(int argc, char** argv) {
    celixLauncher_launch("config.properties");
    int rc = RUN_ALL_TESTS(argc, argv);
    framework_stop((celixLauncher_getFramework()));
    celixLauncher_waitForShutdown();
    return rc;
}