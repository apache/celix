/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <celix_bundle_activator.h>
#include <pubsub_admin.h>
#include <pubsub_admin_metrics.h>

#include "celix_api.h"

#include "celix_log_helper.h"

#include "pubsub_topology_manager.h"
#include "pubsub_listeners.h"

typedef struct pstm_activator {
    pubsub_topology_manager_t *manager;

    long pubsubDiscoveryTrackerId;
    long pubsubAdminTrackerId;
    long pubsubSubscribersTrackerId;
    long pubsubPublishServiceTrackerId;
    long pubsubPSAMetricsTrackerId;

    pubsub_discovered_endpoint_listener_t discListenerSvc;
    long discListenerSvcId;

    celix_shell_command_t shellCmdSvc;
    long shellCmdSvcId;

    celix_log_helper_t *loghelper;
} pstm_activator_t;


static int pstm_start(pstm_activator_t *act, celix_bundle_context_t *ctx) {
    celix_status_t status;

    act->discListenerSvcId = -1L;
    act->pubsubSubscribersTrackerId = -1L;
    act->pubsubAdminTrackerId = -1L;
    act->pubsubDiscoveryTrackerId = -1L;
    act->pubsubPublishServiceTrackerId = -1L;
    act->pubsubPSAMetricsTrackerId = -1L;
    act->shellCmdSvcId = -1L;

    act->loghelper = celix_logHelper_create(ctx, "celix_psa_topology_manager");

    status = pubsub_topologyManager_create(ctx, act->loghelper, &act->manager);

    //track pubsub announce endpoint listener services (pubsub discovery components)
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.addWithProperties = pubsub_topologyManager_pubsubAnnounceEndpointListenerAdded;
        opts.removeWithProperties = pubsub_topologyManager_pubsubAnnounceEndpointListenerRemoved;
        opts.callbackHandle = act->manager;
        opts.filter.serviceName = PUBSUB_ANNOUNCE_ENDPOINT_LISTENER_SERVICE;
        act->pubsubDiscoveryTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //track pubsub admin service (psa components)
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.addWithProperties = pubsub_topologyManager_psaAdded;
        opts.removeWithProperties = pubsub_topologyManager_psaRemoved;
        opts.callbackHandle = act->manager;
        opts.filter.serviceName = PUBSUB_ADMIN_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        act->pubsubAdminTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //track subscriber service
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.addWithOwner = pubsub_topologyManager_subscriberAdded;
        opts.removeWithOwner = pubsub_topologyManager_subscriberRemoved;
        opts.callbackHandle = act->manager;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        act->pubsubSubscribersTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }


    //track requests for publisher services
    if (status == CELIX_SUCCESS) {
        act->pubsubPublishServiceTrackerId = celix_bundleContext_trackServiceTrackers(ctx,
                                                                                      PUBSUB_PUBLISHER_SERVICE_NAME,
                                                                                      act->manager,
                                                                                      pubsub_topologyManager_publisherTrackerAdded,
                                                                                      pubsub_topologyManager_publisherTrackerRemoved);
    }

    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = act->manager;
        opts.addWithProperties = pubsub_topologyManager_addMetricsService;
        opts.removeWithProperties = pubsub_topologyManager_removeMetricsService;
        opts.filter.serviceName = PUBSUB_ADMIN_METRICS_SERVICE_NAME;
        act->pubsubPSAMetricsTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register discovered endpoints listener service (called by pubsub discovery components)
    if (status == CELIX_SUCCESS) {
        act->discListenerSvc.handle = act->manager;
        act->discListenerSvc.addDiscoveredEndpoint = pubsub_topologyManager_addDiscoveredEndpoint;
        act->discListenerSvc.removeDiscoveredEndpoint = pubsub_topologyManager_removeDiscoveredEndpoint;
        act->discListenerSvcId = celix_bundleContext_registerService(ctx, &act->discListenerSvc, PUBSUB_DISCOVERED_ENDPOINT_LISTENER_SERVICE, NULL);
    }

    //register shell command
    if (status == CELIX_SUCCESS) {
        act->shellCmdSvc.handle = act->manager;
        act->shellCmdSvc.executeCommand = pubsub_topologyManager_shellCommand;
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "celix::pstm");
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, "pstm [topology|metrics]"); //TODO add search topic/scope option
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "pubsub_topology_info: Overview of Topology information for PubSub");
        act->shellCmdSvcId = celix_bundleContext_registerService(ctx, &act->shellCmdSvc, CELIX_SHELL_COMMAND_SERVICE_NAME, props);
    }

    //TODO add tracker for pubsub_serializer and
    //1) on remove reset sender/receivers entries
    //2) on add indicate that topic/senders should be reevaluated.

    /* NOTE: Enable those line in order to remotely expose the topic_info service
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, OSGI_RSA_SERVICE_EXPORTED_INTERFACES, PUBSUB_TOPIC_INFO_SERVICE);
    status += bundleContext_registerService(context, (char *) PUBSUB_TOPIC_INFO_SERVICE, activator->topicInfo, props, &activator->topicInfoService);
    */

    return 0;
}

static int pstm_stop(pstm_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(ctx, act->pubsubSubscribersTrackerId);
    celix_bundleContext_stopTracker(ctx, act->pubsubDiscoveryTrackerId);
    celix_bundleContext_stopTracker(ctx, act->pubsubAdminTrackerId);
    celix_bundleContext_stopTracker(ctx, act->pubsubPublishServiceTrackerId);
    celix_bundleContext_stopTracker(ctx, act->pubsubPSAMetricsTrackerId);
    celix_bundleContext_unregisterService(ctx, act->discListenerSvcId);
    celix_bundleContext_unregisterService(ctx, act->shellCmdSvcId);

    pubsub_topologyManager_destroy(act->manager);

    celix_logHelper_destroy(act->loghelper);

    return 0;
}

CELIX_GEN_BUNDLE_ACTIVATOR(pstm_activator_t, pstm_start, pstm_stop);
