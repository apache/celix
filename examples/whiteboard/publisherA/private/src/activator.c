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
 * activator.c
 *
 *  \date       Aug 23, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <constants.h>

#include "bundle_activator.h"
#include "publisher_private.h"

struct activatorData {
    publisher_service_pt ps;
    publisher_pt pub;

    service_registration_pt reg;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	*userData = calloc(1, sizeof(struct activatorData));
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
	properties_pt props = NULL;

	struct activatorData * data = (struct activatorData *) userData;
	data->ps = calloc(1, sizeof(*(data->ps)));
	data->pub = calloc(1, sizeof(*(data->pub)));
	data->ps->invoke = publisher_invoke;
	data->ps->publisher = data->pub;
	data->reg = NULL;

	props = properties_create();
	properties_set(props, "id", "A");
	properties_set(props,(char*)OSGI_FRAMEWORK_SERVICE_RANKING , "10");

	status = bundleContext_registerService(context, PUBLISHER_NAME, data->ps, props, &data->reg);
	if (status != CELIX_SUCCESS) {
		char error[256];
		printf("Error: %s\n", celix_strerror(status, error, 256));
	}

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;

    struct activatorData * data = (struct activatorData *) userData;
    serviceRegistration_unregister(data->reg);

    free(data->ps);
    free(data->pub);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	free(userData);
    return CELIX_SUCCESS;
}
