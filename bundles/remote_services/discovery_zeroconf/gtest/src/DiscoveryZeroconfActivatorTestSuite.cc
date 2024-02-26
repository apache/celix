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

#include "discovery_zeroconf_announcer.h"
#include "discovery_zeroconf_watcher.h"
#include "celix_dm_component_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_properties_ei.h"
#include "celix_log_helper_ei.h"
#include "eventfd_ei.h"
#include "celix_framework.h"
#include "celix_framework_factory.h"
#include "celix_bundle_activator.h"
#include "celix_errno.h"
#include <gtest/gtest.h>

class DiscoveryZeroconfActivatorTestSuite : public ::testing::Test {
public:
    DiscoveryZeroconfActivatorTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".dzc_act_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

    }

    ~DiscoveryZeroconfActivatorTestSuite() override {
        celix_ei_expect_eventfd(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getDependencyManager(nullptr, 0, nullptr);
        celix_ei_expect_celix_dmComponent_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_dmServiceDependency_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(DiscoveryZeroconfActivatorTestSuite, ActivatorStart) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = celix_bundleActivator_stop(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, DiscoveryZeroconfAnnouncerCreateFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_eventfd((void*)&discoveryZeroconfAnnouncer_create, 0, -1);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, DiscoveryZeroconfWatcherCreateFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_eventfd((void *)&discoveryZeroconfWatcher_create, 0, -1);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, GetFrameworkUuidFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_bundleContext_getProperty(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateLogHelperFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_logHelper_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateAnnouncerDmComponentFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_dmComponent_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateAnnouncerEndpointListenerPropertiesFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, SetEndpointListenerSocpePropertyFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_properties_set((void*)&celix_bundleActivator_start, 1, CELIX_ENOMEM);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, SetEndpointListenerInterfaceSpecificEndpointsPropertyFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_properties_set((void*)&celix_bundleActivator_start, 1, CELIX_ENOMEM, 3);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateWatcherDmComponentFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_dmComponent_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateEndpointListenerDependencyFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_dmServiceDependency_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, CreateRsaDependencyFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_dmServiceDependency_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfActivatorTestSuite, GetDependencyManagerFailed) {
    void *act{nullptr};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_ei_expect_celix_bundleContext_getDependencyManager(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(act, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);
    status = celix_bundleActivator_destroy(act, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}