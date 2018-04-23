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
 * celix_launcher.c
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "celix_launcher.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <signal.h>

#ifndef CELIX_NO_CURLINIT
#include <curl/curl.h>
#endif

#include <string.h>
#include <curl/curl.h>
#include <signal.h>
#include <libgen.h>
#include "celix_launcher.h"
#include "framework.h"
#include "linked_list_iterator.h"

static void show_usage(char* prog_name);
static void shutdown_framework(int signal);
static void ignore(int signal);

static int celixLauncher_launchWithConfigAndProps(const char *configFile, framework_pt *framework, properties_pt packedConfig);
static int celixLauncher_launchWithStreamAndProps(FILE *stream, framework_pt *framework, properties_pt packedConfig);

#define DEFAULT_CONFIG_FILE "config.properties"

static framework_pt framework = NULL;

int celixLauncher_launchAndWaitForShutdown(int argc, char *argv[], properties_pt packedConfig) {
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

	struct sigaction sigact;
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = shutdown_framework;
	sigaction(SIGINT,  &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);

	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = ignore;
	sigaction(SIGUSR1,  &sigact, NULL);
	sigaction(SIGUSR2,  &sigact, NULL);

	int rc = celixLauncher_launchWithConfigAndProps(config_file, &framework, packedConfig);
	if (rc == 0) {
		celixLauncher_waitForShutdown(framework);
		celixLauncher_destroy(framework);
	}
	return rc;
}

static void show_usage(char* prog_name) {
	printf("Usage:\n  %s [path/to/config.properties]\n\n", basename(prog_name));
}

static void shutdown_framework(int signal) {
	if (framework != NULL) {
		celixLauncher_stop(framework); //NOTE main thread will destroy
	}
}

static void ignore(int signal) {
	//ignoring for signal SIGUSR1, SIGUSR2. Can be used on threads
}

int celixLauncher_launch(const char *configFile, framework_pt *framework) {
	return celixLauncher_launchWithConfigAndProps(configFile, framework, NULL);
}

static int celixLauncher_launchWithConfigAndProps(const char *configFile, framework_pt *framework, properties_pt packedConfig){
	int status = 0;
	FILE *config = fopen(configFile, "r");
	if (packedConfig == NULL) {
		packedConfig = properties_create();
	}

	if (config != NULL) {
		status = celixLauncher_launchWithStreamAndProps(config, framework, packedConfig);
	} else {
		status = celixLauncher_launchWithProperties(packedConfig, framework);
	}

	return status;
}

static int celixLauncher_launchWithStreamAndProps(FILE *stream, framework_pt *framework, properties_pt packedConfig){
	int status = 0;

	properties_pt runtimeConfig = properties_loadWithStream(stream);
	fclose(stream);

	// Make sure we've read it and that nothing went wrong with the file access...
	// If there is no runtimeConfig, the packedConfig can be stored as global config
	if (runtimeConfig == NULL){
		runtimeConfig = packedConfig;
	}

	if (runtimeConfig == NULL) {
		fprintf(stderr, "Error: invalid configuration file");
		perror(NULL);
		status = 1;
	} else {
		// Check if there's a pre-compiled config available
		if (packedConfig != NULL){
			// runtimeConfig and packedConfig must be merged
			// when a duplicate of a key is available, the runtimeConfig must be prioritized

			hash_map_iterator_t iter = hashMapIterator_construct(packedConfig);

			hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);

			while (entry != NULL) {
				const char * key = (const char *) hashMapEntry_getKey(entry);
				const char * value = (const char *) hashMapEntry_getValue(entry);

				// Check existence of key in runtimeConfig
				if (!hashMap_containsKey(runtimeConfig, key)) {
					properties_set(runtimeConfig, key, value);
				}

				entry = hashMapIterator_nextEntry(&iter);
				if (entry != NULL) {
					key = (const char *) hashMapEntry_getKey(entry);
					value = (const char *) hashMapEntry_getValue(entry);
				}
			}

			// normally, the framework_destroy will clean up the properties_pt
			// since there are 2 properties_pt available (runtimeConfig and packedConfig)
			// the packedConfig must be destroyed
			properties_destroy(packedConfig);
		}

		status = celixLauncher_launchWithProperties(runtimeConfig, framework);
	}

	return status;
}

int celixLauncher_launchWithProperties(properties_pt config, framework_pt *framework) {
	celix_status_t status;
	if (config == NULL) {
		config = properties_create();
	}
#ifndef CELIX_NO_CURLINIT
	// Before doing anything else, let's setup Curl
	curl_global_init(CURL_GLOBAL_NOTHING);
#endif


	status = framework_create(framework, config);
	if (status == CELIX_SUCCESS) {
		status = framework_start(*framework);
	}

	if (status == CELIX_SUCCESS) {
		printf("Launcher: Framework Started\n");
	}
	
	return status;
}

void celixLauncher_waitForShutdown(framework_pt framework) {
	framework_waitForStop(framework);
}

void celixLauncher_destroy(framework_pt framework) {
	framework_destroy(framework);

#ifndef CELIX_NO_CURLINIT
	// Cleanup Curl
	curl_global_cleanup();
#endif

	printf("Launcher: Exit\n");
}

void celixLauncher_stop(framework_pt framework) {
	bundle_pt fwBundle = NULL;
	if( framework_getFrameworkBundle(framework, &fwBundle) == CELIX_SUCCESS){
		bundle_stop(fwBundle);
	}
}
