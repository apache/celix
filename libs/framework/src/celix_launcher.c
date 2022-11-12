/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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

#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_framework_utils.h"

static void show_usage(char* prog_name);
static void show_properties(celix_properties_t *embeddedProps, const char *configFile);
static void printEmbeddedBundles();
static void shutdown_framework(int signal);
static void ignore(int signal);

static int celixLauncher_launchWithConfigAndProps(const char *configFile, celix_framework_t* *framework, celix_properties_t* packedConfig);

static void combine_properties(celix_properties_t *original, const celix_properties_t *append);

#define DEFAULT_CONFIG_FILE "config.properties"

static framework_t *g_fw = NULL;

int celixLauncher_launchAndWaitForShutdown(int argc, char *argv[], celix_properties_t* packedConfig) {
	celix_framework_t* framework = NULL;


	// Perform some minimal command-line option parsing...
	char *opt = NULL;
	if (argc > 1) {
		opt = argv[1];
	}

	char *config_file = NULL;
	bool showProps = false;
    bool showEmbeddedBundles = false;
	for (int i = 1; i < argc; ++i) {
		opt = argv[i];
		// Check whether the user wants some help...
		if (strncmp("-?", opt, strlen("-?")) == 0 || strncmp("-h", opt, strlen("-h")) == 0 || strncmp("--help", opt, strlen("--help")) == 0) {
			show_usage(argv[0]);
			celix_properties_destroy(packedConfig);
			return 0;
		} else if (strncmp("-p", opt, strlen("-p")) == 0 || strncmp("--props", opt, strlen("--props")) == 0) {
            showProps = true;
        } else if (strncmp("--embedded_bundles", opt, strlen("--embedded_bundles")) == 0) {
            showEmbeddedBundles = true;
		} else {
			config_file = opt;
		}
	}

	if (config_file == NULL) {
		config_file = DEFAULT_CONFIG_FILE;
	}

	if (packedConfig == NULL) {
		packedConfig = celix_properties_create();
	}

	if (showProps) {
		show_properties(packedConfig, config_file);
		celix_properties_destroy(packedConfig);
		return 0;
	}

    if (showEmbeddedBundles) {
        printEmbeddedBundles();
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
	printf("\t-p | --props: Show the embedded and runtime properties for this Celix container\n");
    printf("\t--embedded_bundles: Show the embedded bundles for this Celix container\n");
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

int celixLauncher_launch(const char *configFile, celix_framework_t* *framework) {
	return celixLauncher_launchWithConfigAndProps(configFile, framework, NULL);
}

static int celixLauncher_launchWithConfigAndProps(const char *configFile, celix_framework_t* *framework, celix_properties_t* packedConfig) {
	if (packedConfig == NULL) {
		packedConfig = celix_properties_create();
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


int celixLauncher_launchWithProperties(celix_properties_t* config, celix_framework_t* *framework) {
#ifndef CELIX_NO_CURLINIT
	// Before doing anything else, let's setup Curl
	curl_global_init(CURL_GLOBAL_NOTHING);
#endif
	*framework = celix_frameworkFactory_createFramework(config);
	return *framework != NULL ? CELIX_SUCCESS : CELIX_ILLEGAL_STATE;
}

void celixLauncher_waitForShutdown(celix_framework_t* framework) {
    celix_framework_waitForStop(framework);
}

void celixLauncher_destroy(celix_framework_t* framework) {
    celix_frameworkFactory_destroyFramework(framework);
#ifndef CELIX_NO_CURLINIT
	// Cleanup Curl
	curl_global_cleanup();
#endif
}

void celixLauncher_stop(celix_framework_t* framework) {
    celix_framework_stopBundle(framework, CELIX_FRAMEWORK_BUNDLE_ID);
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

static void printEmbeddedBundles() {
    celix_array_list_t* embeddedBundles = celix_framework_utils_listEmbeddedBundles();
    printf("Embedded bundles:\n");
    for (int i = 0; i < celix_arrayList_size(embeddedBundles); ++i) {
        char* bundle = celix_arrayList_get(embeddedBundles, i);
        printf("|- %02i: %s\n", (i+1), bundle);
        free(bundle);
    }
    if (celix_arrayList_size(embeddedBundles) == 0) {
        printf("|- no embedded bundles\n");
    }
    celix_arrayList_destroy(embeddedBundles);
}

static void combine_properties(celix_properties_t *original, const celix_properties_t *append) {
	if (original != NULL && append != NULL) {
		const char *key = NULL;
		CELIX_PROPERTIES_FOR_EACH(append, key) {
			celix_properties_set(original, key, celix_properties_get(append, key, NULL));
		}
	}
}
