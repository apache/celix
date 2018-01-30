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
 * ps_pub_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <sys/cdefs.h>
#include <stdlib.h>
#include <string.h>

#include "bundle_activator.h"
#include "service_tracker.h"
#include "constants.h"

#include "pubsub/publisher.h"
#include "pubsub_publisher_private.h"

static const char * PUB_TOPICS[] = {
		"poi1",
		"poi2",
		NULL
};


struct publisherActivator {
	pubsub_sender_pt client;
	array_list_pt trackerList;//List<service_tracker_pt>
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {

	const char* fwUUID = NULL;

	bundleContext_getProperty(context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PUBLISHER: Cannot retrieve fwUUID.\n");
		return CELIX_INVALID_BUNDLE_CONTEXT;
	}

	struct publisherActivator * act = malloc(sizeof(*act));

	bundle_pt bundle = NULL;
	long bundleId = 0;
	bundleContext_getBundle(context,&bundle);
	bundle_getBundleId(bundle,&bundleId);

	arrayList_create(&(act->trackerList));
	act->client = publisher_create(act->trackerList,fwUUID,bundleId);
	*userData = act;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {

	struct publisherActivator * act = (struct publisherActivator *) userData;

	int i;
	char filter[128];
	for(i=0; PUB_TOPICS[i] != NULL; i++){
		const char* topic = PUB_TOPICS[i];

		bundle_pt bundle = NULL;
		long bundleId = 0;
		bundleContext_getBundle(context,&bundle);
		bundle_getBundleId(bundle,&bundleId);

		service_tracker_pt tracker = NULL;
		memset(filter,0,128);
#ifdef USE_SCOPE
			char *scope;
			asprintf(&scope, "my_scope_%d", i);
			snprintf(filter, 128, "(&(&(%s=%s)(%s=%s))(%s=%s))",
					(char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_PUBLISHER_SERVICE_NAME,
					PUBSUB_PUBLISHER_TOPIC, topic,
					PUBLISHER_SCOPE, scope);
			free(scope);
#else
		snprintf(filter, 128, "(&(%s=%s)(%s=%s))", (char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_PUBLISHER_SERVICE_NAME, PUBSUB_PUBLISHER_TOPIC, topic);
#endif
		service_tracker_customizer_pt customizer = NULL;
		serviceTrackerCustomizer_create(act->client,NULL,publisher_publishSvcAdded,NULL,publisher_publishSvcRemoved,&customizer);
		serviceTracker_createWithFilter(context, filter, customizer, &tracker);

		arrayList_add(act->trackerList,tracker);

	}

	publisher_start(act->client);

	for(i=0;i<arrayList_size(act->trackerList);i++){
		service_tracker_pt tracker = arrayList_get(act->trackerList,i);
		serviceTracker_open(tracker);
	}

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt __attribute__((unused)) context) {
	struct publisherActivator * act = (struct publisherActivator *) userData;
	int i;

	for(i=0;i<arrayList_size(act->trackerList);i++){
		service_tracker_pt tracker = arrayList_get(act->trackerList,i);
		serviceTracker_close(tracker);
	}
	publisher_stop(act->client);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	struct publisherActivator * act = (struct publisherActivator *) userData;
	int i;

	for(i=0;i<arrayList_size(act->trackerList);i++){
		service_tracker_pt tracker = arrayList_get(act->trackerList,i);
		serviceTracker_destroy(tracker);
	}

	publisher_destroy(act->client);
	arrayList_destroy(act->trackerList);

	free(act);
	printf("PUBLISHER: bundleActivator_destroy\n");
	return CELIX_SUCCESS;
}
