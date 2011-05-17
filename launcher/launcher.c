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

#include "framework.h"
#include "properties.h"
#include "headers.h"
#include "bundle_context.h"
#include "bundle.h"
#include "linked_list_iterator.h"

static void launcher_load_custom_bundles(void);
void launcher_shutdown(int signal);

int running = 0;

#include <stdio.h>

struct framework * framework;

int main(void) {
	// Set signal handler
	(void) signal(SIGINT, launcher_shutdown);
    PROPERTIES config = properties_load("config.properties");
    char * autoStart = properties_get(config, "cosgi.auto.start.1");
    framework = NULL;
    framework_create(&framework);
    fw_init(framework);

    // Start the system bundle
    framework_start(framework);

    char delims[] = " ";
    char * result;
    LINKED_LIST bundles = linkedList_create();
    result = strtok(autoStart, delims);
    while (result != NULL) {
    	char * location = strdup(result);
    	linkedList_addElement(bundles, location);
    	result = strtok(NULL, delims);
    }
    // Update according to Felix Main
    // First install all bundles
    // Afterwards start them
    ARRAY_LIST installed = arrayList_create();
    BUNDLE_CONTEXT context = bundle_getContext(framework->bundle);
    LINKED_LIST_ITERATOR iter = linkedListIterator_create(bundles, 0);
    while (linkedListIterator_hasNext(iter)) {
    	BUNDLE current = NULL;
    	char * location = linkedListIterator_next(iter);
    	bundleContext_installBundle(context, location, &current);
    	arrayList_add(installed, current);
    	linkedListIterator_remove(iter);
    }
    linkedListIterator_destroy(iter);
    linkedList_destroy(bundles);

    int i;
    for (i = 0; i < arrayList_size(installed); i++) {
    	BUNDLE bundle = (BUNDLE) arrayList_get(installed, i);
		startBundle(bundle, 0);
    }

    arrayList_destroy(installed);


    framework_waitForStop(framework);
    framework_destroy(framework);
    properties_destroy(config);
    return 0;
}

void launcher_shutdown(int signal) {
	framework_stop(framework);
	if (framework_waitForStop(framework) != CELIX_SUCCESS) {
		celix_log("Error waiting for stop.");
	}
	framework_destroy(framework);
}
