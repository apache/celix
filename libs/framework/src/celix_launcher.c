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
#include "celix_array_list.h"
#include "celix_launcher.h"
#include "framework.h"
#include "linked_list_iterator.h"

static void show_usage(char* prog_name);
static void show_properties(celix_properties_t *embeddedProps, const char *configFile);
static void shutdown_framework(int signal);
static void ignore(int signal);

static int celixLauncher_launchWithConfigAndProps(const char *configFile, framework_pt *framework, properties_pt packedConfig);

static void combine_properties(celix_properties_t *orignal, const celix_properties_t *append);

#define DEFAULT_CONFIG_FILE "config.properties"

static framework_t *g_fw = NULL;

int celixLauncher_launchAndWaitForShutdown(int argc, char *argv[], properties_pt packedConfig) {
	framework_pt framework = NULL;


	// Perform some minimal command-line option parsing...
	char *opt = NULL;
	if (argc > 1) {
		opt = argv[1];
	}

	char *config_file = NULL;
	bool showProps = false;
	for (int i = 1; i < argc; ++i) {
		opt = argv[i];
		// Check whether the user wants some help...
		if (strncmp("-?", opt, strlen("-?")) == 0 || strncmp("-h", opt, strlen("-h")) == 0 || strncmp("--help", opt, strlen("--help")) == 0) {
			show_usage(argv[0]);
			celix_properties_destroy(packedConfig);
			return 0;
		} else if (strncmp("-p", opt, strlen("-p")) == 0 || strncmp("--props", opt, strlen("--props")) == 0) {
			showProps = true;
		} else {
			config_file = opt;
		}
	}

	if (config_file == NULL) {
		config_file = DEFAULT_CONFIG_FILE;
	}

	if (packedConfig == NULL) {
		packedConfig = properties_create();
	}

	if (showProps) {
		show_properties(packedConfig, config_file);
		celix_properties_destroy(packedConfig);
		return 0;
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
		g_fw = framework;
		celixLauncher_waitForShutdown(framework);
		celixLauncher_destroy(framework);
	}
	return rc;
}

static void show_usage(char* prog_name) {
	printf("Usage:\n  %s [-h|-p] [path/to/runtime/config.properties]\n", basename(prog_name));
	printf("Options:\n");
	printf("\t-h | --help: Show this message\n");
	printf("\t-p | --props: Show the embedded and runtime properties for this celix container\n");
	printf("\n");
}

static void shutdown_framework(int signal) {
	if (g_fw != NULL) {
		celixLauncher_stop(g_fw); //NOTE main thread will destroy
	}
}

static void ignore(int signal) {
	//ignoring for signal SIGUSR1, SIGUSR2. Can be used on threads
}

int celixLauncher_launch(const char *configFile, framework_pt *framework) {
	return celixLauncher_launchWithConfigAndProps(configFile, framework, NULL);
}

static int celixLauncher_launchWithConfigAndProps(const char *configFile, framework_pt *framework, properties_pt packedConfig) {
	if (packedConfig == NULL) {
		packedConfig = properties_create();
	}

	FILE *config = fopen(configFile, "r");
	if (config != NULL) {
		celix_properties_t *configProps = celix_properties_loadWithStream(config);
		fclose(config);
		combine_properties(packedConfig, configProps);
		celix_properties_destroy(configProps);
	}

	return celixLauncher_launchWithProperties(packedConfig, framework);
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
}

void celixLauncher_stop(framework_pt framework) {
	bundle_pt fwBundle = NULL;
	if( framework_getFrameworkBundle(framework, &fwBundle) == CELIX_SUCCESS){
		bundle_stop(fwBundle);
	}
}

static void show_properties(celix_properties_t *embeddedProps, const char *configFile) {
	const char *key = NULL;
	celix_properties_t *keys = celix_properties_create(); //only to store the keys

	printf("Embedded properties:\n");
	if (embeddedProps == NULL || celix_properties_size(embeddedProps) == 0) {
		printf("|- Empty!\n");
	} else {
		CELIX_PROPERTIES_FOR_EACH(embeddedProps, key) {
			const char *val = celix_properties_get(embeddedProps, key, "!Error!");
			printf("|- %s=%s\n", key, val);
            celix_properties_set(keys, key, NULL);
        }
	}
	printf("\n");

	celix_properties_t *runtimeProps = NULL;
	if (configFile != NULL) {
        runtimeProps = celix_properties_load(configFile);
	}
	printf("Runtime properties (input %s):\n", configFile);
	if (runtimeProps == NULL || celix_properties_size(runtimeProps) == 0) {
		printf("|- Empty!\n");
	} else {
		CELIX_PROPERTIES_FOR_EACH(runtimeProps, key) {
			const char *val = celix_properties_get(runtimeProps, key, "!Error!");
			printf("|- %s=%s\n", key, val);
            celix_properties_set(keys, key, NULL);
		}
	}
    printf("\n");

	//combined result
	combine_properties(embeddedProps, runtimeProps);
	printf("Resolved (env, runtime and embedded) properties:\n");
	if (celix_properties_size(keys) == 0) {
		printf("|- Empty!\n");
	} else {
		CELIX_PROPERTIES_FOR_EACH(keys, key) {
			const char *valEm = celix_properties_get(embeddedProps, key, NULL);
            const char *valRt = celix_properties_get(runtimeProps, key, NULL);
            const char *envVal = getenv(key);
            const char *val = envVal != NULL ? envVal : valRt != NULL ? valRt : valEm;
            const char *source = envVal != NULL ? "environment" : valRt != NULL ? "runtime" : "embedded";
			printf("|- %s=%s (source %s)\n", key, val, source);
		}
	}
    printf("\n");

	if (runtimeProps != NULL) {
		celix_properties_destroy(runtimeProps);
	}
	celix_properties_destroy(keys);
}

static void combine_properties(celix_properties_t *orignal, const celix_properties_t *append) {
	if (orignal != NULL && append != NULL) {
		const char *key = NULL;
		CELIX_PROPERTIES_FOR_EACH(append, key) {
			celix_properties_set(orignal, key, celix_properties_get(append, key, NULL));
		}
	}
}