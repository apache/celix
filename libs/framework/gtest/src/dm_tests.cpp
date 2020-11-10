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

#include <gtest/gtest.h>

#include "celix_api.h"

class DepenencyManagerTests : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    DepenencyManagerTests() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }

    ~DepenencyManagerTests() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    DepenencyManagerTests(DepenencyManagerTests&&) = delete;
    DepenencyManagerTests(const DepenencyManagerTests&) = delete;
    DepenencyManagerTests& operator=(DepenencyManagerTests&&) = delete;
    DepenencyManagerTests& operator=(const DepenencyManagerTests&) = delete;
};

TEST_F(DepenencyManagerTests, DmCreateComponent) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp = celix_dmComponent_create(ctx, "test1");
    celix_dependencyManager_add(mng, cmp);

    ASSERT_EQ(1, celix_dependencyManager_nrOfComponents(mng));
    ASSERT_TRUE(celix_dependencyManager_allComponentsActive(mng));

    cmp = celix_dmComponent_create(ctx, "test2");
    celix_dependencyManager_add(mng, cmp);

    ASSERT_EQ(2, celix_dependencyManager_nrOfComponents(mng));
    ASSERT_TRUE(celix_dependencyManager_allComponentsActive(mng));
}

TEST_F(DepenencyManagerTests, TestCheckActive) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp = celix_dmComponent_create(ctx, "test1");

    auto *dep = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep, "svcname", nullptr, nullptr);
    celix_dmServiceDependency_setRequired(dep, true);
    celix_dmComponent_addServiceDependency(cmp, dep); //required dep -> cmp not active


    celix_dependencyManager_add(mng, cmp);
    ASSERT_FALSE(celix_dependencyManager_areComponentsActive(mng));
}

class TestComponent {

};

TEST_F(DepenencyManagerTests, OnlyActiveAfterBuildCheck) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    cmp.build();
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active
}

TEST_F(DepenencyManagerTests, StartDmWillBuildCmp) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    dm.start();
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active
}

//TODO add test where not all svc dependency are active and active them by a build call
