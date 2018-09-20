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
#include <celix_bundle_activator.h>

#include "celix_api.h"

#include "log_service.h"
#include "log_helper.h"


#include "pubsub_topology_manager.h"
#include "pubsub_listeners.h"

typedef struct pstm_activator {
	pubsub_topology_manager_t *manager;

	long pubsubDiscoveryTrackerId;
	long pubsubAdminTrackerId;
	long pubsubSubscribersTrackerId;
	long pubsubPublishServiceTrackerId;
	listener_hook_service_pt hookService;
	service_registration_pt hook;

	pubsub_discovered_endpoint_listener_t discListenerSvc;
	long discListenerSvcId;

	log_helper_pt loghelper;
} pstm_activator_t;


static int pstm_start(pstm_activator_t *act, celix_bundle_context_t *ctx) {
	celix_status_t status = CELIX_SUCCESS;

	act->discListenerSvcId = -1L;
	act->pubsubSubscribersTrackerId = -1L;
	act->pubsubAdminTrackerId = -1L;
	act->pubsubDiscoveryTrackerId = -1L;
	act->pubsubPublishServiceTrackerId = -1L;

	logHelper_create(ctx, &act->loghelper);
	logHelper_start(act->loghelper);

	status = pubsub_topologyManager_create(ctx, act->loghelper, &act->manager);

	//create PSD tracker
	if (status == CELIX_SUCCESS) {
		celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
		opts.addWithProperties = pubsub_topologyManager_pubsubDiscoveryAdded;
		opts.removeWithProperties = pubsub_topologyManager_pubsubDiscoveryRemoved;
		opts.callbackHandle = act->manager;
		opts.filter.serviceName = PUBSUB_ANNOUNCE_ENDPOINT_LISTENER_SERVICE;
		act->pubsubDiscoveryTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}

	//create PSA tracker
	if (status == CELIX_SUCCESS) {
		celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
		opts.addWithProperties = pubsub_topologyManager_psaAdded;
		opts.removeWithProperties = pubsub_topologyManager_psaRemoved;
		opts.callbackHandle = act->manager;
		opts.filter.serviceName = PUBSUB_ADMIN_SERVICE;
        opts.filter.ignoreServiceLanguage = true;
		act->pubsubAdminTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}

	//create PSSubtracker
	if (status == CELIX_SUCCESS) {
		celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
		opts.addWithOwner = pubsub_topologyManager_subscriberAdded;
		opts.removeWithOwner = pubsub_topologyManager_subscriberRemoved;
		opts.callbackHandle = act->manager;
		opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
		act->pubsubSubscribersTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}


	//track interest for publishers
	if (status == CELIX_SUCCESS) {
		act->pubsubPublishServiceTrackerId = celix_bundleContext_trackServiceTrackers(ctx,
																					  PUBSUB_PUBLISHER_SERVICE_NAME,
																					  act->manager,
																					  pubsub_topologyManager_publisherTrackerAdded,
																					  pubsub_topologyManager_publisherTrackerRemoved);
	}

	//register listener for discovery event
	if (status == CELIX_SUCCESS) {
		act->discListenerSvc.handle = act->manager;
		act->discListenerSvc.addDiscoveredEndpoint = pubsub_topologyManager_addDiscoveredEndpoint;
		act->discListenerSvc.removeDiscoveredEndpoint = pubsub_topologyManager_removeDiscoveredEndpoint;
		act->discListenerSvcId = celix_bundleContext_registerService(ctx, &act->discListenerSvc, PUBSUB_DISCOVERED_ENDPOINT_LISTENER_SERVICE, NULL);
	}


	/* NOTE: Enable those line in order to remotely expose the topic_info service
	properties_pt props = properties_create();
	properties_set(props, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, (char *) PUBSUB_TOPIC_INFO_SERVICE);
	status += bundleContext_registerService(context, (char *) PUBSUB_TOPIC_INFO_SERVICE, activator->topicInfo, props, &activator->topicInfoService);
	*/

	return 0;
}

static int pstm_stop(pstm_activator_t *act, celix_bundle_context_t *ctx) {
	celix_bundleContext_stopTracker(ctx, act->pubsubSubscribersTrackerId);
	celix_bundleContext_stopTracker(ctx, act->pubsubDiscoveryTrackerId);
	celix_bundleContext_stopTracker(ctx, act->pubsubAdminTrackerId);
	celix_bundleContext_stopTracker(ctx, act->pubsubPublishServiceTrackerId);
	celix_bundleContext_unregisterService(ctx, act->discListenerSvcId);

	pubsub_topologyManager_destroy(act->manager);

	logHelper_stop(act->loghelper);
	logHelper_destroy(&act->loghelper);

	return 0;
}

CELIX_GEN_BUNDLE_ACTIVATOR(pstm_activator_t, pstm_start, pstm_stop);
