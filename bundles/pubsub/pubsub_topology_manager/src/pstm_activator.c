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
 * pstm_activator.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "bundle_activator.h"
#include "service_tracker.h"
#include "service_registration.h"

#include "listener_hook_service.h"
#include "log_service.h"
#include "log_helper.h"


#include "pubsub_topology_manager.h"
#include "publisher_endpoint_announce.h"

struct activator {
	bundle_context_pt context;

	pubsub_topology_manager_pt manager;

	service_tracker_pt pubsubDiscoveryTracker;
	service_tracker_pt pubsubAdminTracker;
	service_tracker_pt pubsubSubscribersTracker;

	listener_hook_service_pt hookService;
	service_registration_pt hook;

	publisher_endpoint_announce_pt publisherEPDiscover;
	service_registration_pt publisherEPDiscoverService;

	log_helper_pt loghelper;
};


static celix_status_t bundleActivator_createPSDTracker(struct activator *activator, service_tracker_pt *tracker);
static celix_status_t bundleActivator_createPSATracker(struct activator *activator, service_tracker_pt *tracker);
static celix_status_t bundleActivator_createPSSubTracker(struct activator *activator, service_tracker_pt *tracker);


static celix_status_t bundleActivator_createPSDTracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->manager,
			NULL,
			pubsub_topologyManager_pubsubDiscoveryAdded,
			pubsub_topologyManager_pubsubDiscoveryModified,
			pubsub_topologyManager_pubsubDiscoveryRemoved,
			&customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, (char *) PUBSUB_DISCOVERY_SERVICE, customizer, tracker);
	}

	return status;
}

static celix_status_t bundleActivator_createPSATracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->manager,
			NULL,
			pubsub_topologyManager_psaAdded,
			pubsub_topologyManager_psaModified,
			pubsub_topologyManager_psaRemoved,
			&customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, PUBSUB_ADMIN_SERVICE, customizer, tracker);
	}

	return status;
}

static celix_status_t bundleActivator_createPSSubTracker(struct activator *activator, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(activator->manager,
			NULL,
			pubsub_topologyManager_subscriberAdded,
			pubsub_topologyManager_subscriberModified,
			pubsub_topologyManager_subscriberRemoved,
			&customizer);

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_create(activator->context, PUBSUB_SUBSCRIBER_SERVICE_NAME, customizer, tracker);
	}

	return status;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = NULL;

	activator = calloc(1,sizeof(struct activator));

	if (!activator) {
		return CELIX_ENOMEM;
	}

	activator->context = context;

	logHelper_create(context, &activator->loghelper);
	logHelper_start(activator->loghelper);

	status = pubsub_topologyManager_create(context, activator->loghelper, &activator->manager);
	if (status == CELIX_SUCCESS) {
		status = bundleActivator_createPSDTracker(activator, &activator->pubsubDiscoveryTracker);
		if (status == CELIX_SUCCESS) {
			status = bundleActivator_createPSATracker(activator, &activator->pubsubAdminTracker);
			if (status == CELIX_SUCCESS) {
				status = bundleActivator_createPSSubTracker(activator, &activator->pubsubSubscribersTracker);
				if (status == CELIX_SUCCESS) {
					*userData = activator;
				}
			}
		}
	}

	if(status != CELIX_SUCCESS){
		bundleActivator_destroy(activator, context);
	}

	return status;
}


celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	publisher_endpoint_announce_pt pubEPDiscover = calloc(1, sizeof(*pubEPDiscover));
	pubEPDiscover->handle = activator->manager;
	pubEPDiscover->announcePublisher = pubsub_topologyManager_announcePublisher;
	pubEPDiscover->removePublisher = pubsub_topologyManager_removePublisher;
	activator->publisherEPDiscover = pubEPDiscover;

	status += bundleContext_registerService(context, (char *) PUBSUB_TM_ANNOUNCE_PUBLISHER_SERVICE, pubEPDiscover, NULL, &activator->publisherEPDiscoverService);


	listener_hook_service_pt hookService = calloc(1,sizeof(*hookService));
	hookService->handle = activator->manager;
	hookService->added = pubsub_topologyManager_publisherTrackerAdded;
	hookService->removed = pubsub_topologyManager_publisherTrackerRemoved;
	activator->hookService = hookService;

	status += bundleContext_registerService(context, (char *) OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, hookService, NULL, &activator->hook);

	/* NOTE: Enable those line in order to remotely expose the topic_info service
	properties_pt props = properties_create();
	properties_set(props, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, (char *) PUBSUB_TOPIC_INFO_SERVICE);
	status += bundleContext_registerService(context, (char *) PUBSUB_TOPIC_INFO_SERVICE, activator->topicInfo, props, &activator->topicInfoService);
	*/
	status += serviceTracker_open(activator->pubsubAdminTracker);

	status += serviceTracker_open(activator->pubsubDiscoveryTracker);

	status += serviceTracker_open(activator->pubsubSubscribersTracker);


	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	serviceTracker_close(activator->pubsubSubscribersTracker);
	serviceTracker_close(activator->pubsubDiscoveryTracker);
	serviceTracker_close(activator->pubsubAdminTracker);

	serviceRegistration_unregister(activator->publisherEPDiscoverService);
	free(activator->publisherEPDiscover);

	serviceRegistration_unregister(activator->hook);
	free(activator->hookService);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	struct activator *activator = userData;
	if (activator == NULL) {
		status = CELIX_BUNDLE_EXCEPTION;
	} else {

		if(activator->pubsubSubscribersTracker!=NULL){
			serviceTracker_destroy(activator->pubsubSubscribersTracker);
		}
		if(activator->pubsubDiscoveryTracker!=NULL){
			serviceTracker_destroy(activator->pubsubDiscoveryTracker);
		}
		if(activator->pubsubAdminTracker!=NULL){
			serviceTracker_destroy(activator->pubsubAdminTracker);
		}

		if(activator->manager!=NULL){
			status = pubsub_topologyManager_destroy(activator->manager);
		}

		logHelper_stop(activator->loghelper);
		logHelper_destroy(&activator->loghelper);

		free(activator);
	}

	return status;
}
