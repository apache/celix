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
#include "endpoint_listener.h"
#include "celix_event_remote_provider_service.h"
#include "CelixEarpmTestSuiteBaseClass.h"


class CelixEarpmActTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmActTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_act_test_cache"}{ }

    ~CelixEarpmActTestSuite() override = default;
};

TEST_F(CelixEarpmActTestSuite, ActivatorStartTest) {
    void *act{};
    auto status = celix_bundleActivator_create(ctx.get(), &act);
    ASSERT_EQ(CELIX_SUCCESS, status);
    status = celix_bundleActivator_start(act, ctx.get());
    ASSERT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());
    long svcId = celix_bundleContext_findService(ctx.get(), CELIX_EVENT_REMOTE_PROVIDER_SERVICE_NAME);
    EXPECT_TRUE(svcId >= 0);

    svcId = celix_bundleContext_findService(ctx.get(), CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME);
    EXPECT_TRUE(svcId >= 0);

    status = celix_bundleActivator_stop(act, ctx.get());
    ASSERT_EQ(CELIX_SUCCESS, status);
    status = celix_bundleActivator_destroy(act, ctx.get());
    ASSERT_EQ(CELIX_SUCCESS, status);
}