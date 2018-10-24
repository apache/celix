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


#include <stdlib.h>

#include "celix_api.h"
#include "pubsub_serializer.h"
#include "log_helper.h"

#include "pubsub_admin.h"
#include "pubsub_nanomsg_admin.h"
#include "../../../shell/shell/include/command.h"

typedef struct psa_nanomsg_activator {
	log_helper_t *logHelper;

	pubsub_nanomsg_admin_t *admin;

	long serializersTrackerId;

	pubsub_admin_service_t adminService;
	long adminSvcId;

	command_service_t cmdSvc;
	long cmdSvcId;
} psa_nanomsg_activator_t;

int psa_nanomsg_start(psa_nanomsg_activator_t *act, celix_bundle_context_t *ctx) {
	act->adminSvcId = -1L;
	act->cmdSvcId = -1L;
	act->serializersTrackerId = -1L;

	logHelper_create(ctx, &act->logHelper);
	logHelper_start(act->logHelper);

	act->admin = pubsub_nanoMsgAdmin_create(ctx, act->logHelper);
	celix_status_t status = act->admin != NULL ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;

	//track serializers
	if (status == CELIX_SUCCESS) {
		celix_service_tracking_options_t opts{};
		opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
		opts.filter.ignoreServiceLanguage = true;
		opts.callbackHandle = act->admin;
		opts.addWithProperties = pubsub_nanoMsgAdmin_addSerializerSvc;
		opts.removeWithProperties = pubsub_nanoMsgAdmin_removeSerializerSvc;
		act->serializersTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}

	//register pubsub admin service
	if (status == CELIX_SUCCESS) {
		pubsub_admin_service_t *psaSvc = &act->adminService;
		psaSvc->handle = act->admin;
		psaSvc->matchPublisher = pubsub_nanoMsgAdmin_matchPublisher;
		psaSvc->matchSubscriber = pubsub_nanoMsgAdmin_matchSubscriber;
		psaSvc->matchEndpoint = pubsub_nanoMsgAdmin_matchEndpoint;
		psaSvc->setupTopicSender = pubsub_nanoMsgAdmin_setupTopicSender;
		psaSvc->teardownTopicSender = pubsub_nanoMsgAdmin_teardownTopicSender;
		psaSvc->setupTopicReceiver = pubsub_nanoMsgAdmin_setupTopicReceiver;
		psaSvc->teardownTopicReceiver = pubsub_nanoMsgAdmin_teardownTopicReceiver;
		psaSvc->addEndpoint = pubsub_nanoMsgAdmin_addEndpoint;
		psaSvc->removeEndpoint = pubsub_nanoMsgAdmin_removeEndpoint;

		celix_properties_t *props = celix_properties_create();
		celix_properties_set(props, PUBSUB_ADMIN_SERVICE_TYPE, PUBSUB_NANOMSG_ADMIN_TYPE);

		act->adminSvcId = celix_bundleContext_registerService(ctx, psaSvc, PUBSUB_ADMIN_SERVICE_NAME, props);
	}

	//register shell command service
	{
		act->cmdSvc.handle = act->admin;
		act->cmdSvc.executeCommand = pubsub_nanoMsgAdmin_executeCommand;
		celix_properties_t *props = celix_properties_create();
		celix_properties_set(props, OSGI_SHELL_COMMAND_NAME, "psa_nanomsg");
		celix_properties_set(props, OSGI_SHELL_COMMAND_USAGE, "psa_nanomsg");
		celix_properties_set(props, OSGI_SHELL_COMMAND_DESCRIPTION, "Print the information about the TopicSender and TopicReceivers for the ZMQ PSA");
		act->cmdSvcId = celix_bundleContext_registerService(ctx, &act->cmdSvc, OSGI_SHELL_COMMAND_SERVICE_NAME, props);
	}

	return status;
}

int psa_nanomsg_stop(psa_nanomsg_activator_t *act, celix_bundle_context_t *ctx) {
	celix_bundleContext_unregisterService(ctx, act->adminSvcId);
	celix_bundleContext_unregisterService(ctx, act->cmdSvcId);
	celix_bundleContext_stopTracker(ctx, act->serializersTrackerId);
	pubsub_nanoMsgAdmin_destroy(act->admin);

	logHelper_stop(act->logHelper);
	logHelper_destroy(&act->logHelper);

	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psa_nanomsg_activator_t, psa_nanomsg_start, psa_nanomsg_stop);
