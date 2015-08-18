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
 * main.c
 *
 *  \date       Jul 30, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <string.h>
#include <curl/curl.h>
#include <signal.h>
#include <libgen.h>
#include "celix_launcher.h"

static void show_usage(char* prog_name);
static void shutdown_framework(int signal);

#define DEFAULT_CONFIG_FILE "config.properties"

static framework_pt framework = NULL;

int main(int argc, char *argv[]) {

    // Perform some minimal command-line option parsing...
    char *opt = NULL;
    if (argc > 1) {
        opt = argv[1];
    }

    char *config_file = NULL;

    if (opt) {
        // Check whether the user wants some help...
        if (strcmp("-h", opt) == 0 || strcmp("-help", opt) == 0) {
            show_usage(argv[0]);
            return 0;
        } else {
            config_file = opt;
        }
    } else {
        config_file = DEFAULT_CONFIG_FILE;
    }


    // Set signal handler
    (void) signal(SIGINT, shutdown_framework);

    int rc = celixLauncher_launch(config_file, &framework);
    if (rc == 0) {
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);
    }
}

static void show_usage(char* prog_name) {
    printf("Usage:\n  %s [path/to/config.properties]\n\n", basename(prog_name));
}

static void shutdown_framework(int signal) {
    if (framework != NULL) {
        celixLauncher_stop(framework); //NOTE main thread will destroy
    }
}