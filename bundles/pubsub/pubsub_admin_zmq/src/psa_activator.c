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

#include "pubsub_admin_impl.h"

typedef struct psa_zmq_activator {
	pubsub_admin_t *admin;

	pubsub_admin_service_t adminService;
	long adminSvcId;

	command_service_t cmdSvc;
	long cmdSvcId;

	long serializerTrackerId;
} psa_zmq_activator_t;

static celix_status_t shellCommand(void *handle, char * commandLine, FILE *outStream, FILE *errorStream) {
	psa_zmq_activator_t *act= handle;
	if (act->admin->externalPublications && !hashMap_isEmpty(act->admin->externalPublications)) {
		fprintf(outStream, "External Publications:\n");
		for(hash_map_iterator_t iter = hashMapIterator_construct(act->admin->externalPublications); hashMapIterator_hasNext(&iter);) {
			const char* key = (const char*)hashMapIterator_nextKey(&iter);
			fprintf(outStream, "    %s\n", key);
		}
	}
	if (act->admin->localPublications && !hashMap_isEmpty(act->admin->localPublications)) {
		fprintf(outStream, "Local Publications:\n");
		for (hash_map_iterator_t iter = hashMapIterator_construct(
				act->admin->localPublications); hashMapIterator_hasNext(&iter);) {
			const char *key = (const char *) hashMapIterator_nextKey(&iter);
			fprintf(outStream, "    %s\n", key);
		}
	}
	if (act->admin->subscriptions && !hashMap_isEmpty(act->admin->subscriptions)) {
		fprintf(outStream, "Active Subscriptions:\n");
		for (hash_map_iterator_t iter = hashMapIterator_construct(
				act->admin->subscriptions); hashMapIterator_hasNext(&iter);) {
			const char *key = (const char *) hashMapIterator_nextKey(&iter);
			fprintf(outStream, "    %s\n", key);
		}
	}
	if (act->admin->pendingSubscriptions && !hashMap_isEmpty(act->admin->pendingSubscriptions)) {
		fprintf(outStream, "Pending Subscriptions:\n");
		for (hash_map_iterator_t iter = hashMapIterator_construct(
				act->admin->pendingSubscriptions); hashMapIterator_hasNext(&iter);) {
			const char *key = (const char *) hashMapIterator_nextKey(&iter);
			fprintf(outStream, "    %s\n", key);
		}
	}
	return CELIX_SUCCESS;
}

int psa_zmq_start(psa_zmq_activator_t *act, celix_bundle_context_t *ctx) {
	act->adminSvcId = -1L;
	act->serializerTrackerId = -1l;

	celix_status_t status = pubsubAdmin_create(ctx, &(act->admin));

	//track pubsub serializers
	if (status == CELIX_SUCCESS) {
		celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
		opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
		opts.filter.ignoreServiceLanguage = true;

		opts.callbackHandle = act->admin;
		opts.addWithProperties = pubsubAdmin_addSerializer;
		opts.removeWithProperties = pubsubAdmin_removeSerializer;
		act->serializerTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	}

	//register psa service
	if (status == CELIX_SUCCESS) {
		pubsub_admin_service_t *psaSvc = &act->adminService;
		psaSvc->handle = act->admin;
		psaSvc->matchPublisher = pubsubAdmin_matchPublisher;
		psaSvc->matchSubscriber = pubsubAdmin_matchSubscriber;
		psaSvc->matchEndpoint = pubsubAdmin_matchEndpoint;
		psaSvc->setupTopicSender = pubsubAdmin_setupTopicSender;
		psaSvc->teardownTopicSender = pubsubAdmin_teardownTopicSender;
		psaSvc->setupTopicReciever = pubsubAdmin_setupTopicReciever;
		psaSvc->teardownTopicReciever = pubsubAdmin_teardownTopicReciever;
		psaSvc->addEndpoint = pubsubAdmin_addEndpoint;
		psaSvc->removeEndpoint = pubsubAdmin_removeEndpoint;

		celix_properties_t *props = celix_properties_create();
		celix_properties_set(props, PUBSUB_ADMIN_SERVICE_TYPE, PUBSUB_PSA_ZMQ_PSA_TYPE);

		act->adminSvcId = celix_bundleContext_registerService(ctx, psaSvc, PUBSUB_ADMIN_SERVICE_NAME, props);
	}

	//register psa shell command
	if (status == CELIX_SUCCESS) {
		act->cmdSvc.handle = act;
		act->cmdSvc.executeCommand = shellCommand;

		celix_properties_t *shellProps = celix_properties_create();
		celix_properties_set(shellProps, OSGI_SHELL_COMMAND_NAME, "psa_zmq_info");
		celix_properties_set(shellProps, OSGI_SHELL_COMMAND_USAGE, "psa_zmq_info");
		celix_properties_set(shellProps, OSGI_SHELL_COMMAND_DESCRIPTION, "psa_zmq_info: Overview of PubSub ZMQ Admin");

		act->cmdSvcId = celix_bundleContext_registerService(ctx, &act->cmdSvc, OSGI_SHELL_COMMAND_SERVICE_NAME, shellProps);
	}

	return status;
}

int psa_zmq_stop(psa_zmq_activator_t *act, celix_bundle_context_t *ctx) {
	celix_bundleContext_unregisterService(ctx, act->adminSvcId);
	celix_bundleContext_unregisterService(ctx, act->cmdSvcId);
	celix_bundleContext_stopTracker(ctx, act->serializerTrackerId);
	pubsubAdmin_destroy(act->admin);
	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(psa_zmq_activator_t, psa_zmq_start, psa_zmq_stop);
