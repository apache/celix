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

#include "gtest/gtest.h"

#include <memory>
#include <cstdarg>

#include "celix_api.h"
#include "pubsub_message_serialization_service.h"
#include "dyn_message.h"
#include "pubsub_protocol.h"
#include "pubsub_constants.h"
#include "pubsub_matching.h"
#include "pubsub/api.h"
#include "pubsub_endpoint.h"
#include "pubsub_message_serialization_marker.h"

static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

class PubSubMatchingTestSuite : public ::testing::Test {
public:
    PubSubMatchingTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".pubsub_utils_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        bndId = celix_bundleContext_installBundle(ctx.get(), MATCHING_BUNDLE, true);

        dynMessage_logSetup(stdLog, NULL, 1);
    }

    ~PubSubMatchingTestSuite() override {
        celix_bundleContext_uninstallBundle(ctx.get(), bndId);
    }

    long registerMarkerSerSvc(const char* type) {
        auto* p = celix_properties_create();
        celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY, type);
        celix_service_registration_options_t opts{};
        opts.svc = static_cast<void*>(&serMarkerSvc);
        opts.properties = p;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_MARKER_VERSION;
        return celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    pubsub_message_serialization_marker_t serMarkerSvc{};
    pubsub_protocol_service_t protocolSvc{};
    long bndId{};
};

TEST_F(PubSubMatchingTestSuite, MatchPublisherSimple) {
    auto serMarkerId = registerMarkerSerSvc("fiets");
    long foundSvcId = -1;
    pubsub_utils_matchPublisher(ctx.get(), bndId, "(&(objectClass=pubsub.publisher)(service.lang=C)(topic=fiets))", "admin?", 0, 0, 0, false, NULL, &foundSvcId, NULL);
    EXPECT_EQ(foundSvcId, serMarkerId);
    celix_bundleContext_unregisterService(ctx.get(), serMarkerId);
}

TEST_F(PubSubMatchingTestSuite, MatchPublisherMultiple) {
    auto serFietsId = registerMarkerSerSvc("fiets");
    auto serFiets2Id = registerMarkerSerSvc("fiets");
    auto serAutoId = registerMarkerSerSvc("auto");
    auto serBelId = registerMarkerSerSvc("bel");
    long foundSvcId = -1;
    pubsub_utils_matchPublisher(ctx.get(), bndId, "(&(objectClass=pubsub.publisher)(service.lang=C)(topic=fiets))", "admin?", 0, 0, 0, false, NULL, &foundSvcId, NULL);
    EXPECT_EQ(foundSvcId, serFietsId); //older service are ranked higher
    celix_bundleContext_unregisterService(ctx.get(), serFietsId);
    celix_bundleContext_unregisterService(ctx.get(), serFiets2Id);
    celix_bundleContext_unregisterService(ctx.get(), serAutoId);
    celix_bundleContext_unregisterService(ctx.get(), serBelId);
}

TEST_F(PubSubMatchingTestSuite, MatchSubscriberSimple) {
    auto serId = registerMarkerSerSvc("fiets");

    long foundSvcId = -1;
    auto* p = celix_properties_create();
    celix_properties_set(p, PUBSUB_SUBSCRIBER_SCOPE, "scope");
    celix_properties_set(p, PUBSUB_SUBSCRIBER_TOPIC, "fiets");
    pubsub_utils_matchSubscriber(ctx.get(), bndId, p, "admin?", 0, 0, 0, false, NULL, &foundSvcId, NULL);
    EXPECT_EQ(foundSvcId, serId);
    celix_properties_destroy(p);
    celix_bundleContext_unregisterService(ctx.get(), serId);
}

TEST_F(PubSubMatchingTestSuite, MatchSubscriberMultiple) {
    auto serFietsId = registerMarkerSerSvc("fiets");
    auto serFiets2Id = registerMarkerSerSvc("fiets");
    auto serAutoId = registerMarkerSerSvc("auto");
    auto serBelId = registerMarkerSerSvc("bel");

    long foundSvcId = -1;

    auto* p = celix_properties_create();
    celix_properties_set(p, PUBSUB_SUBSCRIBER_SCOPE, "scope");
    celix_properties_set(p, PUBSUB_SUBSCRIBER_TOPIC, "fiets");

    pubsub_utils_matchSubscriber(ctx.get(), bndId, p, "admin?", 0, 0, 0, false, NULL, &foundSvcId, NULL);

    EXPECT_EQ(foundSvcId, serFietsId);

    celix_properties_destroy(p);
    celix_bundleContext_unregisterService(ctx.get(), serFietsId);
    celix_bundleContext_unregisterService(ctx.get(), serFiets2Id);
    celix_bundleContext_unregisterService(ctx.get(), serAutoId);
    celix_bundleContext_unregisterService(ctx.get(), serBelId);
}

TEST_F(PubSubMatchingTestSuite, MatchEndpointSimple) {
    auto serId = registerMarkerSerSvc("fiets");

    long foundSvcId = -1;

    auto logHelper = celix_logHelper_create(ctx.get(), "logger");
    auto* ep = celix_properties_create();
    celix_properties_set(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, "admin?");
    celix_properties_set(ep, PUBSUB_ENDPOINT_SERIALIZER, "fiets");

    pubsub_utils_matchEndpoint(ctx.get(), logHelper, ep, "admin?", false, &foundSvcId, NULL);

    EXPECT_EQ(foundSvcId, serId);

    celix_properties_destroy(ep);
    celix_logHelper_destroy(logHelper);
    celix_bundleContext_unregisterService(ctx.get(), serId);
}

TEST_F(PubSubMatchingTestSuite, MatchEndpointMultiple) {
    auto serFietsId = registerMarkerSerSvc("fiets");
    auto serFiets2Id = registerMarkerSerSvc("fiets");
    auto serAutoId = registerMarkerSerSvc("auto");
    auto serBelId = registerMarkerSerSvc("bel");

    long foundSvcId = -1;

    auto logHelper = celix_logHelper_create(ctx.get(), "logger");
    auto* ep = celix_properties_create();
    celix_properties_set(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, "admin?");
    celix_properties_set(ep, PUBSUB_ENDPOINT_SERIALIZER, "fiets");

    pubsub_utils_matchEndpoint(ctx.get(), logHelper, ep, "admin?", false, &foundSvcId, NULL);

    EXPECT_EQ(foundSvcId, serFietsId);

    celix_properties_destroy(ep);
    celix_logHelper_destroy(logHelper);
    celix_bundleContext_unregisterService(ctx.get(), serFietsId);
    celix_bundleContext_unregisterService(ctx.get(), serFiets2Id);
    celix_bundleContext_unregisterService(ctx.get(), serAutoId);
    celix_bundleContext_unregisterService(ctx.get(), serBelId);
}