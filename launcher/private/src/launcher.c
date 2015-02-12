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
 * launcher.c
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <libgen.h>
#include <apr_general.h>
#include <apr_strings.h>
#include <curl/curl.h>

#include "framework.h"
#include "properties.h"
#include "bundle_context.h"
#include "bundle.h"
#include "linked_list_iterator.h"
#include "celix_log.h"

#define DEFAULT_CONFIG_FILE "config.properties"

void launcher_shutdown(int signal);

int running = 0;

struct framework * framework;
apr_pool_t *memoryPool;

void show_usage(char* prog_name);

int main(int argc, char *argv[]) {
	// Set signal handler
	apr_status_t rv = APR_SUCCESS;
	apr_status_t s = APR_SUCCESS;
	properties_pt config = NULL;
	char *autoStart = NULL;
    apr_pool_t *pool = NULL;
    bundle_pt fwBundle = NULL;

	(void) signal(SIGINT, launcher_shutdown);

	// Before doing anything else, let's setup Curl
	curl_global_init(CURL_GLOBAL_NOTHING);

	rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

    s = apr_pool_create(&memoryPool, NULL);
    if (s != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

	// Perform some minimal command-line option parsing...
	char* opt = NULL;
	if (argc > 1) {
		opt = argv[1];
	}

	char* config_file = NULL;

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

    config = properties_load(config_file);
	// Make sure we've read it and that nothing went wrong with the file access...
	if (config == NULL) {
		printf("Error: invalid or non-existing configuration file: \"%s\"!\n", config_file);
		perror(NULL);
		show_usage(argv[0]);
		return CELIX_START_ERROR;
	}

    autoStart = properties_get(config, "cosgi.auto.start.1");
    framework = NULL;
    celix_status_t status = CELIX_SUCCESS;
    status = framework_create(&framework, memoryPool, config);
    if (status == CELIX_SUCCESS) {
		status = fw_init(framework);
		if (status == CELIX_SUCCESS) {
            // Start the system bundle
            framework_getFrameworkBundle(framework, &fwBundle);
            bundle_start(fwBundle);

            if (apr_pool_create(&pool, memoryPool) == APR_SUCCESS) {
                char delims[] = " ";
                char *result = NULL;
                char *save_ptr = NULL;
                linked_list_pt bundles;
                array_list_pt installed = NULL;
                bundle_pt bundle = NULL;
                bundle_context_pt context = NULL;
                linked_list_iterator_pt iter = NULL;
                unsigned int i;

                linkedList_create(&bundles);
                result = strtok_r(autoStart, delims, &save_ptr);
                while (result != NULL) {
                    char * location = apr_pstrdup(memoryPool, result);
                    linkedList_addElement(bundles, location);
                    result = strtok_r(NULL, delims, &save_ptr);
                }
                // First install all bundles
                // Afterwards start them
                arrayList_create(&installed);
                framework_getFrameworkBundle(framework, &bundle);
                bundle_getContext(bundle, &context);
                iter = linkedListIterator_create(bundles, 0);
                while (linkedListIterator_hasNext(iter)) {
                    bundle_pt current = NULL;
                    char * location = (char *) linkedListIterator_next(iter);
                    if (bundleContext_installBundle(context, location, &current) == CELIX_SUCCESS) {
                        // Only add bundle if it is installed correctly
                        arrayList_add(installed, current);
                    } else {
                        printf("Could not install bundle from %s\n", location);
                    }
                    linkedListIterator_remove(iter);
                }
                linkedListIterator_destroy(iter);
                linkedList_destroy(bundles);

                for (i = 0; i < arrayList_size(installed); i++) {
                    bundle_pt bundle = (bundle_pt) arrayList_get(installed, i);
                    bundle_startWithOptions(bundle, 0);
                }

                arrayList_destroy(installed);
                apr_pool_destroy(pool);
            }

            framework_waitForStop(framework);
            framework_destroy(framework);
            properties_destroy(config);
		}
    }

    if (status != CELIX_SUCCESS) {
        printf("Problem creating framework\n");
    }

	apr_pool_destroy(memoryPool);
	apr_terminate();

	// Cleanup Curl
	curl_global_cleanup();

	printf("Launcher: Exit\n");

    return 0;
}

void launcher_shutdown(int signal) {
	bundle_pt fwBundle = NULL;
	framework_getFrameworkBundle(framework, &fwBundle);
	bundle_stop(fwBundle);
//	if (framework_waitForStop(framework) != CELIX_SUCCESS) {
//		celix_log("Error waiting for stop.");
//	}
//	framework_destroy(framework);
}

void show_usage(char* prog_name) {
	printf("Usage:\n  %s [path/to/config.properties]\n\n", basename(prog_name));
}
