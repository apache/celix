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


#include <signal.h>

#include "celix_launcher.h"

#include <CppUTest/CommandLineTestRunner.h>

static void shutdown_framework(int signal);
static void ignore(int signal);

#define DEFAULT_CONFIG_FILE "config.properties"

static framework_pt framework = NULL;

int main(int argc, char *argv[]) {
    // Perform some minimal command-line option parsing...
    const char *cfg = DEFAULT_CONFIG_FILE;

    // Set signal handler
    (void) signal(SIGINT, shutdown_framework);
    (void) signal(SIGUSR1, ignore);
    (void) signal(SIGUSR2, ignore);

    int rc = celixLauncher_launch(cfg, &framework);
    if (rc != 0) {
        printf("Error starting Celix\n");
    }

    if (rc == 0 && framework != NULL) {
        rc = RUN_ALL_TESTS(argc, argv);

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);
    }

    if (rc != 0) {
        printf("*** FAILURE ***\n");
    } else {
        printf("*** SUCCESS ***\n");
    }

    return rc;
}

static void shutdown_framework(int __attribute__((unused)) signal) {
    if (framework != NULL) {
        celixLauncher_stop(framework); //NOTE main thread will destroy
    }
}

static void ignore(int __attribute__((unused)) signal) {
    //ignoring for signal SIGUSR1, SIGUSR2. Can be used to interrupt sleep, etc
}
