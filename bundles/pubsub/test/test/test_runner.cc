#include "celix_api.h"


#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

int main(int argc, char **argv) {
    celix_framework_t *fw = NULL;
    celixLauncher_launch("config.properties", &fw);

    int rc = RUN_ALL_TESTS(argc, argv);

    celixLauncher_stop(fw);
    celixLauncher_waitForShutdown(fw);
    celixLauncher_destroy(fw);

    return rc;
}