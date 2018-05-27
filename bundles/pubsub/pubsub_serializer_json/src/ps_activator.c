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
 * ps_activator.c
 *
 *  \date       Mar 24, 2017
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "bundle_activator.h"
#include "service_registration.h"
#include "pubsub_constants.h"

#include "pubsub_serializer_impl.h"

struct activator {
	pubsub_serializer_t* serializer;
	pubsub_serializer_service_t* serializerService;
	service_registration_pt registration;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator;

	activator = calloc(1, sizeof(*activator));
	if (!activator) {
		status = CELIX_ENOMEM;
	}
	else{
		*userData = activator;
		status = pubsubSerializer_create(context, &(activator->serializer));
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	pubsub_serializer_service_t* pubsubSerializerSvc = calloc(1, sizeof(*pubsubSerializerSvc));

	if (!pubsubSerializerSvc) {
		status = CELIX_ENOMEM;
	}
	else{
		pubsubSerializerSvc->handle = activator->serializer;

		pubsubSerializerSvc->createSerializerMap = (void*)pubsubSerializer_createSerializerMap;
		pubsubSerializerSvc->destroySerializerMap = (void*)pubsubSerializer_destroySerializerMap;
		activator->serializerService = pubsubSerializerSvc;

		/* Set serializer type */
		properties_pt props = properties_create();
		properties_set(props, PUBSUB_SERIALIZER_TYPE_KEY, PUBSUB_SERIALIZER_TYPE);

		status = bundleContext_registerService(context, PUBSUB_SERIALIZER_SERVICE, pubsubSerializerSvc, props, &activator->registration);

	}

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceRegistration_unregister(activator->registration);
	activator->registration = NULL;

	free(activator->serializerService);
	activator->serializerService = NULL;

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	pubsubSerializer_destroy(activator->serializer);
	activator->serializer = NULL;

	free(activator);

	return status;
}


