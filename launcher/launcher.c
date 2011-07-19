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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <apr_general.h>
#include <apr_strings.h>

#include "framework.h"
#include "properties.h"
#include "headers.h"
#include "bundle_context.h"
#include "bundle.h"
#include "linked_list_iterator.h"

void launcher_shutdown(int signal);

int running = 0;

struct framework * framework;
apr_pool_t *memoryPool;

int main(void) {
	// Set signal handler
	(void) signal(SIGINT, launcher_shutdown);

	apr_status_t rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

    apr_status_t s = apr_pool_create(&memoryPool, NULL);
    if (s != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

    PROPERTIES config = properties_load("config.properties");
    char * autoStart = properties_get(config, "cosgi.auto.start.1");
    framework = NULL;
    framework_create(&framework, memoryPool);
    fw_init(framework);

    // Start the system bundle
    framework_start(framework);

    char delims[] = " ";
    char * result;
    apr_pool_t *pool;
    if (apr_pool_create(&pool, memoryPool) == APR_SUCCESS) {
        LINKED_LIST bundles;
        linkedList_create(pool, &bundles);
        result = strtok(autoStart, delims);
        while (result != NULL) {
            char * location = apr_pstrdup(memoryPool, result);
            linkedList_addElement(bundles, location);
            result = strtok(NULL, delims);
        }
        // First install all bundles
        // Afterwards start them
        ARRAY_LIST installed = arrayList_create();
        BUNDLE_CONTEXT context = bundle_getContext(framework->bundle);
        LINKED_LIST_ITERATOR iter = linkedListIterator_create(bundles, 0);
        while (linkedListIterator_hasNext(iter)) {
            BUNDLE current = NULL;
            char * location = linkedListIterator_next(iter);
            if (bundleContext_installBundle(context, location, &current) == CELIX_SUCCESS) {
                // Only add bundle if it is installed correctly
                arrayList_add(installed, current);
            } else {
                char error[256];
                sprintf(error, "Could not install bundle from %s", location);
                celix_log(error);
            }
            linkedListIterator_remove(iter);
        }
        linkedListIterator_destroy(iter);
        apr_pool_destroy(pool);

        int i;
        for (i = 0; i < arrayList_size(installed); i++) {
            BUNDLE bundle = (BUNDLE) arrayList_get(installed, i);
            startBundle(bundle, 0);
        }

        arrayList_destroy(installed);
    }

    framework_waitForStop(framework);
    framework_destroy(framework);
    properties_destroy(config);

    apr_pool_destroy(memoryPool);
    apr_terminate();

    return 0;
}

void launcher_shutdown(int signal) {
	framework_stop(framework);
//	if (framework_waitForStop(framework) != CELIX_SUCCESS) {
//		celix_log("Error waiting for stop.");
//	}
//	framework_destroy(framework);
}
