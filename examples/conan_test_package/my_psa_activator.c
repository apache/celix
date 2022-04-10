/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <celix_api.h>
#include <pubsub_serializer.h>
#include <celix_log_helper.h>
#include <pubsub_admin.h>
#include <pubsub_constants.h>
#include <celix_constants.h>
#include <stdlib.h>

typedef struct my_psa_activator {
    celix_log_helper_t *logHelper;
    long serializersTrackerId;
    pubsub_admin_service_t adminService;
    long adminSvcId;
} my_psa_activator_t;

static void myPsa_addSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    my_psa_activator_t *act = handle;
    const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    celix_logHelper_info(act->logHelper, "Serializer Added: %s=%s %s=%ld\n",
                         PUBSUB_SERIALIZER_TYPE_KEY, serType, OSGI_FRAMEWORK_SERVICE_ID, svcId);

}

static void myPsa_removeSerializerSvc(void *handle, void *svc, const celix_properties_t *props) {
    my_psa_activator_t *act = handle;
    const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
    long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    celix_logHelper_info(act->logHelper, "Serializer Removed: %s=%s %s=%ld\n",
                         PUBSUB_SERIALIZER_TYPE_KEY, serType, OSGI_FRAMEWORK_SERVICE_ID, svcId);
}

static celix_status_t matchPublisher(void *handle, long svcRequesterBndId,
                                     const celix_filter_t *svcFilter, celix_properties_t **outTopicProperties,
                                     double *outScore, long *outSerializerSvcId, long *outProtocolSvcId) {
    (void)handle;
    (void)svcRequesterBndId;
    (void)svcFilter;
    (void)outTopicProperties;
    (void)outScore;
    (void)outSerializerSvcId;
    (void)outProtocolSvcId;
    return CELIX_SUCCESS;
}

static celix_status_t matchSubscriber(void *handle, long svcProviderBndId,
                                      const celix_properties_t *svcProperties, celix_properties_t **outTopicProperties,
                                      double *outScore, long *outSerializerSvcId, long *outProtocolSvcId) {
    (void)handle;
    (void)svcProviderBndId;
    (void)svcProperties;
    (void)outTopicProperties;
    (void)outScore;
    (void)outSerializerSvcId;
    (void)outProtocolSvcId;
    return CELIX_SUCCESS;

}

static celix_status_t matchDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint, bool *match) {
    (void)handle;
    (void)endpoint;
    (void)match;
    return CELIX_SUCCESS;
}

static celix_status_t setupTopicSender(void *handle, const char *scope, const char *topic,
                                       const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId,
                                       celix_properties_t **publisherEndpoint) {
    (void)handle;
    (void)scope;
    (void)topic;
    (void)topicProperties;
    (void)serializerSvcId;
    (void)protocolSvcId;
    (void)publisherEndpoint;
    return CELIX_SUCCESS;
}

static celix_status_t teardownTopicSender(void *handle, const char *scope, const char *topic) {
    (void)handle;
    (void)scope;
    (void)topic;
    return CELIX_SUCCESS;
}

static celix_status_t setupTopicReceiver(void *handle, const char *scope, const char *topic,
                                         const celix_properties_t *topicProperties, long serializerSvcId, long protocolSvcId,
                                         celix_properties_t **subscriberEndpoint) {
    (void)handle;
    (void)scope;
    (void)topic;
    (void)topicProperties;
    (void)serializerSvcId;
    (void)protocolSvcId;
    (void)subscriberEndpoint;
    return CELIX_SUCCESS;
}

static celix_status_t teardownTopicReceiver(void *handle, const char *scope, const char *topic) {
    (void)handle;
    (void)scope;
    (void)topic;
    return CELIX_SUCCESS;
}

static celix_status_t addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    (void)handle;
    (void)endpoint;
    return CELIX_SUCCESS;
}

static celix_status_t removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    (void)handle;
    (void)endpoint;
}

int psa_udpmc_start(my_psa_activator_t *act, celix_bundle_context_t *ctx) {
    act->adminSvcId = -1L;
    act->serializersTrackerId = -1L;
    act->logHelper = celix_logHelper_create(ctx, "my_psa_admin");
    celix_status_t status = CELIX_SUCCESS;

    //track serializers
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        opts.callbackHandle = act;
        opts.addWithProperties = myPsa_addSerializerSvc;
        opts.removeWithProperties = myPsa_removeSerializerSvc;
        act->serializersTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    //register pubsub admin service
    if (status == CELIX_SUCCESS) {
        pubsub_admin_service_t *psaSvc = &act->adminService;
        psaSvc->handle = act;
        psaSvc->matchPublisher = matchPublisher;
        psaSvc->matchSubscriber = matchSubscriber;
        psaSvc->matchDiscoveredEndpoint = matchDiscoveredEndpoint;
        psaSvc->setupTopicSender = setupTopicSender;
        psaSvc->teardownTopicSender = teardownTopicSender;
        psaSvc->setupTopicReceiver = setupTopicReceiver;
        psaSvc->teardownTopicReceiver = teardownTopicReceiver;
        psaSvc->addDiscoveredEndpoint = addDiscoveredEndpoint;
        psaSvc->removeDiscoveredEndpoint = removeDiscoveredEndpoint;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_ADMIN_SERVICE_TYPE, "my_psa");

        act->adminSvcId = celix_bundleContext_registerService(ctx, psaSvc, PUBSUB_ADMIN_SERVICE_NAME, props);
    }

    return status;
}

int psa_udpmc_stop(my_psa_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->adminSvcId);
    celix_bundleContext_stopTracker(ctx, act->serializersTrackerId);
    celix_logHelper_destroy(act->logHelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(my_psa_activator_t, psa_udpmc_start, psa_udpmc_stop);
