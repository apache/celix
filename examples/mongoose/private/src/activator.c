#include <sys/cdefs.h>/**
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
 * activator.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "bundle_activator.h"
#include "mongoose.h"

struct userData {
	struct mg_context *ctx;
    char* entry;
};

celix_status_t bundleActivator_create(bundle_context_pt  __attribute__((unused)) context, void **userData) {
	*userData = calloc(1, sizeof(struct userData));
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	bundle_pt bundle;
	celix_status_t status = CELIX_SUCCESS;
	struct userData * data = (struct userData *) userData;

	if (bundleContext_getBundle(context, &bundle) == CELIX_SUCCESS) {
		bundle_getEntry(bundle, "root", &data->entry);
		const char *options[] = {
			"document_root", data->entry,
			"listening_ports", "8081",
			NULL
		};

		data->ctx = mg_start(NULL, options);

		if (data->ctx) {
			printf("Mongoose started on: %s\n", mg_get_option(data->ctx, "listening_ports"));
		}
	} else {
		status = CELIX_START_ERROR;
	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	struct userData * data = (struct userData *) userData;
	mg_stop(data->ctx);
	printf("Mongoose stopped\n");
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	struct userData * data = (struct userData *) userData;
    free(data->entry);
	free(data);
	return CELIX_SUCCESS;
}
