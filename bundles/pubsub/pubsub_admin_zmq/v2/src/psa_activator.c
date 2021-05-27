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

#include <stdlib.h>
#include <pubsub_message_serialization_service.h>

#include "celix_api.h"
#include "pubsub_protocol.h"
#include "celix_log_helper.h"

#include "pubsub_admin.h"
#include "pubsub_admin_metrics.h"
#include "pubsub_zmq_admin.h"
#include "celix_shell_command.h"

typedef struct psa_zmq_activator {
    celix_log_helper_t *logHelper;

    pubsub_zmq_admin_t *admin;

    long serializersTrackerId;

    long protocolsTrackerId;

    pubsub_admin_service_t adminService;
    long adminSvcId;

    celix_shell_command_t cmdSvc;
    long cmdSvcId;
} psa_zmq_activator_t;

int psa_zmq_start(psa_zmq_activator_t *act, celix_bundle_context_t *ctx) {
    act->adminSvcId = -1L;
    act->cmdSvcId = -1L;
    act->serializersTrackerId = -1L;
    act->protocolsTrackerId = -1L;

    act->logHelper = celix_logHelper_create(ctx, "celix_psa_zmq_v2");

    act->admin = pubsub_zmqAdmin_create(ctx, act->logHelper);
    celix_status_t status = act->admin != NULL ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;

    //track protocols
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = PUBSUB_PROTOCOL_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        opts.callbackHandle = act->admin;
        opts.addWithProperties = pubsub_zmqAdmin_addProtocolSvc;
        opts.removeWithProperties = pubsub_zmqAdmin_removeProtocolSvc;
        act->protocolsTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register pubsub admin service
    if (status == CELIX_SUCCESS) {
        pubsub_admin_service_t *psaSvc = &act->adminService;
        psaSvc->handle = act->admin;
        psaSvc->matchPublisher = pubsub_zmqAdmin_matchPublisher;
        psaSvc->matchSubscriber = pubsub_zmqAdmin_matchSubscriber;
        psaSvc->matchDiscoveredEndpoint = pubsub_zmqAdmin_matchDiscoveredEndpoint;
        psaSvc->setupTopicSender = pubsub_zmqAdmin_setupTopicSender;
        psaSvc->teardownTopicSender = pubsub_zmqAdmin_teardownTopicSender;
        psaSvc->setupTopicReceiver = pubsub_zmqAdmin_setupTopicReceiver;
        psaSvc->teardownTopicReceiver = pubsub_zmqAdmin_teardownTopicReceiver;
        psaSvc->addDiscoveredEndpoint = pubsub_zmqAdmin_addDiscoveredEndpoint;
        psaSvc->removeDiscoveredEndpoint = pubsub_zmqAdmin_removeDiscoveredEndpoint;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_ADMIN_SERVICE_TYPE, PUBSUB_ZMQ_ADMIN_TYPE);

        act->adminSvcId = celix_bundleContext_registerService(ctx, psaSvc, PUBSUB_ADMIN_SERVICE_NAME, props);
    }

    //register shell command service
    {
        act->cmdSvc.handle = act->admin;
        act->cmdSvc.executeCommand = pubsub_zmqAdmin_executeCommand;
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "celix::psa_zmq");
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, "psa_zmq");
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "Print the information about the TopicSender and TopicReceivers for the ZMQ PSA");
        act->cmdSvcId = celix_bundleContext_registerService(ctx, &act->cmdSvc, CELIX_SHELL_COMMAND_SERVICE_NAME, props);
    }

    return status;
}

int psa_zmq_stop(psa_zmq_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->adminSvcId);
    celix_bundleContext_unregisterService(ctx, act->cmdSvcId);
    celix_bundleContext_stopTracker(ctx, act->serializersTrackerId);
    celix_bundleContext_stopTracker(ctx, act->protocolsTrackerId);
    pubsub_zmqAdmin_destroy(act->admin);

    celix_logHelper_destroy(act->logHelper);

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psa_zmq_activator_t, psa_zmq_start, psa_zmq_stop);
