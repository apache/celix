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

#include "celix_log_helper.h"
#include "celix_shell_command.h"

#include "celix_bundle_context.h"
#include "celix_bundle_activator.h"
#include "celix_constants.h"
#include "celix_log.h"

#include "pubsub_listeners.h"
#include "pubsub_discovery_impl.h"

typedef struct psd_activator {
    pubsub_discovery_t *pubsub_discovery;

    long publishAnnounceSvcTrackerId;
    //service_tracker_pt pstmPublishersTracker;

    pubsub_announce_endpoint_listener_t listenerSvc;
    long listenerSvcId;

    celix_shell_command_t cmdSvc;
    long cmdSvcId;

    celix_log_helper_t *loghelper;
} psd_activator_t;

static celix_status_t psd_start(psd_activator_t *act, celix_bundle_context_t *ctx) {
    celix_status_t status;

    act->loghelper = celix_logHelper_create(ctx, "celix_psa_discovery_etcd");

    act->pubsub_discovery = pubsub_discovery_create(ctx, act->loghelper);
    // pubsub_discovery_start needs to be first to initialize
    status = pubsub_discovery_start(act->pubsub_discovery);

    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = PUBSUB_DISCOVERED_ENDPOINT_LISTENER_SERVICE;
    opts.callbackHandle = act->pubsub_discovery;
    opts.addWithOwner = pubsub_discovery_discoveredEndpointsListenerAdded;
    opts.removeWithOwner = pubsub_discovery_discoveredEndpointsListenerRemoved;
    act->publishAnnounceSvcTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

    act->listenerSvc.handle = act->pubsub_discovery;
    act->listenerSvc.announceEndpoint = pubsub_discovery_announceEndpoint;
    act->listenerSvc.revokeEndpoint = pubsub_discovery_revokeEndpoint;

    //register shell command service
    //register shell command
    if (status == CELIX_SUCCESS) {
        act->cmdSvc.handle = act->pubsub_discovery;
        act->cmdSvc.executeCommand = pubsub_discovery_executeCommand;
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "celix::psd_etcd");
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, "psd_etcd");
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "Overview of discovered/announced endpoints from/to ETCD");
        act->cmdSvcId = celix_bundleContext_registerService(ctx, &act->cmdSvc, CELIX_SHELL_COMMAND_SERVICE_NAME, props);
    }

    if (status == CELIX_SUCCESS) {
        act->listenerSvcId = celix_bundleContext_registerService(ctx, &act->listenerSvc, PUBSUB_ANNOUNCE_ENDPOINT_LISTENER_SERVICE, NULL);
    } else {
        act->listenerSvcId = -1L;
    }

    return status;
}

static celix_status_t psd_stop(psd_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(ctx, act->publishAnnounceSvcTrackerId);
    celix_bundleContext_unregisterService(ctx, act->listenerSvcId);
    celix_bundleContext_unregisterService(ctx, act->cmdSvcId);

    celix_status_t status = pubsub_discovery_stop(act->pubsub_discovery);
    pubsub_discovery_destroy(act->pubsub_discovery);

    celix_logHelper_destroy(act->loghelper);

    return status;
}


CELIX_GEN_BUNDLE_ACTIVATOR(psd_activator_t, psd_start, psd_stop);
