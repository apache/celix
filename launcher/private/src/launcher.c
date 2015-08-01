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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "launcher.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <signal.h>

#include <curl/curl.h>

#include "framework.h"
#include "linked_list_iterator.h"

static struct framework * framework;
static properties_pt config;

#ifdef WITH_APR
static apr_pool_t *memoryPool;
#endif

int celixLauncher_launch(const char *configFile) {
    int status = 0;
    FILE *config = fopen(configFile, "r");
    if (config != NULL) {
        status = celixLauncher_launchWithStream(config);
    } else {
        fprintf(stderr, "Error: invalid or non-existing configuration file: '%s'.", configFile);
        perror("");
        status = 1;
    }
    return status;
}

int celixLauncher_launchWithStream(FILE *stream) {
    int status = 0;

    config = properties_loadWithStream(stream);
    fclose(stream);
    // Make sure we've read it and that nothing went wrong with the file access...
    if (config == NULL) {
        fprintf(stderr, "Error: invalid configuration file");
        perror(NULL);
        status = 1;
    }

    // Set signal handler
#ifdef WITH_APR
    apr_status_t rv;
    apr_status_t s;
#endif

    // Before doing anything else, let's setup Curl
    curl_global_init(CURL_GLOBAL_NOTHING);

#ifdef WITH_APR
	rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

    s = apr_pool_create(&memoryPool, NULL);
    if (s != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }
#endif


    if (status == 0) {
        char *autoStart = properties_get(config, "cosgi.auto.start.1");
        framework = NULL;
        celix_status_t status;
#ifdef WITH_APR
        status = framework_create(&framework, memoryPool, config);
#else
        status = framework_create(&framework, config);
#endif
        bundle_pt fwBundle = NULL;
        if (status == CELIX_SUCCESS) {
            status = fw_init(framework);
            if (status == CELIX_SUCCESS) {
                // Start the system bundle
                framework_getFrameworkBundle(framework, &fwBundle);
                bundle_start(fwBundle);

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
                    char *location = strdup(result);
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
                    char *location = (char *) linkedListIterator_next(iter);
                    if (bundleContext_installBundle(context, location, &current) == CELIX_SUCCESS) {
                        // Only add bundle if it is installed correctly
                        arrayList_add(installed, current);
                    } else {
                        printf("Could not install bundle from %s\n", location);
                    }
                    linkedListIterator_remove(iter);
                    free(location);
                }
                linkedListIterator_destroy(iter);
                linkedList_destroy(bundles);

                for (i = 0; i < arrayList_size(installed); i++) {
                    bundle_pt installedBundle = (bundle_pt) arrayList_get(installed, i);
                    bundle_startWithOptions(installedBundle, 0);
                }

                arrayList_destroy(installed);
            }
        }

        if (status != CELIX_SUCCESS) {
            printf("Problem creating framework\n");
        }

#ifdef WITH_APR
	apr_pool_destroy(memoryPool);
	apr_terminate();
#endif

        // Cleanup Curl
        curl_global_cleanup();

        printf("Launcher: Exit\n");
    }

    return status;
}

void celixLauncher_waitForShutdown(void) {
    framework_waitForStop(framework);
    framework_destroy(framework);
    properties_destroy(config);
}

void celixLauncher_stop(void) {
    bundle_pt fwBundle = NULL;
    framework_getFrameworkBundle(framework, &fwBundle);
    bundle_stop(fwBundle);
}

struct framework *celixLauncher_getFramework(void) {
    return framework;
}

