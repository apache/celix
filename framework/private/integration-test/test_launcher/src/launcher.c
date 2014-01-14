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
#include <apr_general.h>
#include <apr_strings.h>
#include <unistd.h>

#include "framework_private.h"
#include "properties.h"
#include "bundle_context.h"
#include "bundle.h"
#include "linked_list_iterator.h"
#include "celix_log.h"

int main(void) {
	apr_status_t rv = APR_SUCCESS;
	apr_status_t s = APR_SUCCESS;
	properties_pt config = NULL;
	char *autoStart = NULL;
    apr_pool_t *pool = NULL;
    bundle_pt fwBundle = NULL;

	rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }
    apr_pool_t *memoryPool;
    s = apr_pool_create(&memoryPool, NULL);
    if (s != APR_SUCCESS) {
        return CELIX_START_ERROR;
    }

    struct framework * framework = NULL;
    celix_status_t status = CELIX_SUCCESS;
    status = framework_create(&framework, memoryPool, config);
    if (status == CELIX_SUCCESS) {
		status = fw_init(framework);
		if (status == CELIX_SUCCESS) {
            // Start the system bundle
            framework_getFrameworkBundle(framework, &fwBundle);
            bundle_start(fwBundle);
            bundle_context_pt context = NULL;
            bundle_getContext(fwBundle, &context);

            // do some stuff
            bundle_pt bundle = NULL;
            status = CELIX_DO_IF(status, bundleContext_installBundle(context, "../test_bundle1/test_bundle1.zip", &bundle));
            status = CELIX_DO_IF(status, bundle_start(bundle));

            // Stop the system bundle, sleep a bit to let stuff settle down
            sleep(5);
            bundle_stop(fwBundle);

            framework_destroy(framework);
		}
    }

    if (status != CELIX_SUCCESS) {
        printf("Problem creating framework\n");
    }

	apr_pool_destroy(memoryPool);
	apr_terminate();

	printf("LAUNCHER: Exit\n");

    return 0;
}

