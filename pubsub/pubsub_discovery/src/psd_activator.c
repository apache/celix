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
#include <stdlib.h>
#include <string.h>

#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"
#include "constants.h"
#include "celix_log.h"

#include "pubsub_common.h"
#include "publisher_endpoint_announce.h"
#include "pubsub_discovery_impl.h"

struct activator {
	bundle_context_pt context;
	pubsub_discovery_pt pubsub_discovery;

	service_tracker_pt pstmPublishersTracker;

	publisher_endpoint_announce_pt publisherEPAnnounce;
	service_registration_pt publisherEPAnnounceService;
};

static celix_status_t createTMPublisherAnnounceTracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->pubsub_discovery,
			NULL,
			pubsub_discovery_tmPublisherAnnounceAdded,
			pubsub_discovery_tmPublisherAnnounceModified,
			pubsub_discovery_tmPublisherAnnounceRemoved,
			&customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, (char *) PUBSUB_TM_ANNOUNCE_PUBLISHER_SERVICE, customizer, tracker);
	}

	return status;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator* activator = calloc(1, sizeof(*activator));

	if (activator) {
		activator->context = context;
		activator->pstmPublishersTracker = NULL;
		activator->publisherEPAnnounce = NULL;
		activator->publisherEPAnnounceService = NULL;

		status = pubsub_discovery_create(context, &activator->pubsub_discovery);

		if (status == CELIX_SUCCESS) {
			status = createTMPublisherAnnounceTracker(activator, &(activator->pstmPublishersTracker));
		}

		if (status == CELIX_SUCCESS) {
			*userData = activator;
		} else {
			free(activator);
		}
	} else {
		status = CELIX_ENOMEM;
	}

	return status;

}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator = userData;

	publisher_endpoint_announce_pt pubEPAnnouncer = calloc(1, sizeof(*pubEPAnnouncer));

	if (pubEPAnnouncer) {

		pubEPAnnouncer->handle = activator->pubsub_discovery;
		pubEPAnnouncer->announcePublisher = pubsub_discovery_announcePublisher;
		pubEPAnnouncer->removePublisher = pubsub_discovery_removePublisher;
		pubEPAnnouncer->interestedInTopic = pubsub_discovery_interestedInTopic;
		pubEPAnnouncer->uninterestedInTopic = pubsub_discovery_uninterestedInTopic;
		activator->publisherEPAnnounce = pubEPAnnouncer;

		properties_pt props = properties_create();
		properties_set(props, "PUBSUB_DISCOVERY", "true");

		// pubsub_discovery_start needs to be first to initalize the propert etcd_watcher values
		status = pubsub_discovery_start(activator->pubsub_discovery);

		if (status == CELIX_SUCCESS) {
			status = serviceTracker_open(activator->pstmPublishersTracker);
		}

		if (status == CELIX_SUCCESS) {
			status = bundleContext_registerService(context, (char *) PUBSUB_DISCOVERY_SERVICE, pubEPAnnouncer, props, &activator->publisherEPAnnounceService);
		}


	}
	else{
		status = CELIX_ENOMEM;
	}

	if(status!=CELIX_SUCCESS && pubEPAnnouncer!=NULL){
		free(pubEPAnnouncer);
	}


	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	status += pubsub_discovery_stop(activator->pubsub_discovery);

	status += serviceTracker_close(activator->pstmPublishersTracker);

	status += serviceRegistration_unregister(activator->publisherEPAnnounceService);

	if (status == CELIX_SUCCESS) {
		free(activator->publisherEPAnnounce);
	}

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	status += serviceTracker_destroy(activator->pstmPublishersTracker);
	status += pubsub_discovery_destroy(activator->pubsub_discovery);

	activator->publisherEPAnnounce = NULL;
	activator->publisherEPAnnounceService = NULL;
	activator->pstmPublishersTracker = NULL;
	activator->pubsub_discovery = NULL;
	activator->context = NULL;

	free(activator);

	return status;
}
