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

#include "celix_bundle_activator.h"
#include "celix_event_admin_service.h"
#include "celix_dm_component_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_properties_ei.h"
#include "malloc_ei.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_broker_discovery.h"
#include "CelixEarpmTestSuiteBaseClass.h"


class CelixEarpmActErrorInjectionTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmActErrorInjectionTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_act_ej_test_cache"}{}

    ~CelixEarpmActErrorInjectionTestSuite() override {
        celix_ei_expect_celix_dmComponent_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_dmServiceDependency_create(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getDependencyManager(nullptr, 0, nullptr);
        celix_ei_expect_celix_dmServiceDependency_setService(nullptr, 0, 0);
        celix_ei_expect_celix_dmComponent_addServiceDependency(nullptr, 0, 0);
        celix_ei_expect_celix_dmComponent_addInterface(nullptr, 0, 0);
        celix_ei_expect_celix_dependencyManager_addAsync(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
        celix_ei_expect_celix_properties_setBool(nullptr, 0, 0);
    }

    void TestEarpmActivator(void (testBody)(void *act, celix_bundle_context_t *ctx)) {
        void *act{};
        auto status = celix_bundleActivator_create(ctx.get(), &act);
        ASSERT_EQ(CELIX_SUCCESS, status);

        testBody(act, ctx.get());

        status = celix_bundleActivator_destroy(act, ctx.get());
        ASSERT_EQ(CELIX_SUCCESS, status);
    }
};

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEarpmDiscoveryComponentTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_create((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEarpmDiscoveryTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_calloc((void*)&celix_earpmDiscovery_create, 0, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEndpointListenerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToSetServiceForEndpointListenerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_setService((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEndpointListenerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_addServiceDependency((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEarpmComponentTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_create((void*)&celix_bundleActivator_start, 1, nullptr, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEarpmTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_calloc((void*)&celix_earpm_create, 0, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEventHandlerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 1, nullptr, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToSetServiceForEventHandlerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_setService((void*)&celix_bundleActivator_start, 1, ENOMEM, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEventHandlerDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_addServiceDependency((void*)&celix_bundleActivator_start, 1, ENOMEM, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEventAdminDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 1, nullptr, 3);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToSetServiceForEventAdminDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_setService((void*)&celix_bundleActivator_start, 1, ENOMEM, 3);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEventAdminDependencyTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_addServiceDependency((void*)&celix_bundleActivator_start, 1, ENOMEM, 3);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEventRemoteProviderServiceTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_addInterface((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToCreateEndpointListenerServiceProperiesTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_properties_create((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToSetEndpointListenerServiceScopeTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_properties_set((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToSetEndpointListenerServiceInterfaceSpecificEndpointsOptionTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_properties_setBool((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEndpointListenerServiceTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_addInterface((void*)&celix_bundleActivator_start, 1, ENOMEM, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToGetDependencyManagerTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_bundleContext_getDependencyManager((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEarpmDiscoveryComponentTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dependencyManager_addAsync((void*)&celix_bundleActivator_start, 1, ENOMEM);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmActErrorInjectionTestSuite, FailedToAddEarpmComponentTest) {
    TestEarpmActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dependencyManager_addAsync((void*)&celix_bundleActivator_start, 1, ENOMEM, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(ENOMEM, status);
    });
}
