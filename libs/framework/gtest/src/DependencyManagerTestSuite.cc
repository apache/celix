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
#include <atomic>

#include "celix_api.h"

class DependencyManagerTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    DependencyManagerTestSuite() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }

    ~DependencyManagerTestSuite() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    DependencyManagerTestSuite(DependencyManagerTestSuite&&) = delete;
    DependencyManagerTestSuite(const DependencyManagerTestSuite&) = delete;
    DependencyManagerTestSuite& operator=(DependencyManagerTestSuite&&) = delete;
    DependencyManagerTestSuite& operator=(const DependencyManagerTestSuite&) = delete;
};

TEST_F(DependencyManagerTestSuite, DmCreateComponent) {
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

TEST_F(DependencyManagerTestSuite, TestCheckActive) {
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

TEST_F(DependencyManagerTestSuite, OnlyActiveAfterBuildCheck) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    cmp.build();
    cmp.build(); //should be ok to call twice
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    dm.clear();
    dm.clear(); //should be ok to call twice
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm cleared so no components
}

TEST_F(DependencyManagerTestSuite, StartDmWillBuildCmp) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    dm.start();
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    dm.stop();
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm cleared so no components
}

struct TestService {
    void *handle = nullptr;
};

class Cmp1 : public TestService {

};

class Cmp2 : public TestService {
public:
    explicit Cmp2(const std::string& name) {
        std::cout << "usage arg: " << name;
    }
};

TEST_F(DependencyManagerTestSuite, CreateComponentVariant) {
    celix::dm::DependencyManager dm{ctx};

    dm.createComponent<Cmp1>().addInterface<TestService>(); //lazy
    dm.createComponent(std::unique_ptr<Cmp1>{new Cmp1}).addInterface<TestService>(); //with unique ptr
    dm.createComponent(std::make_shared<Cmp1>()).addInterface<TestService>(); //with shared ptr
    dm.createComponent(Cmp1{}).addInterface<TestService>(); //with value

    //dm.createComponent<Cmp2>(); //Does not compile ->  no default ctor
    dm.createComponent(std::unique_ptr<Cmp2>{new Cmp2{"a"}}).addInterface<TestService>(); //with unique ptr
    dm.createComponent(std::make_shared<Cmp2>("b")).addInterface<TestService>();; //with shared ptr
    dm.createComponent(Cmp2{"c"}).addInterface<TestService>();; //with value

    dm.start();
}

TEST_F(DependencyManagerTestSuite, AddSvcProvideAfterBuild) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    cmp.build();
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    TestService svc{};
    cmp.addCInterface(&svc, "TestService");

    long svcId = celix_bundleContext_findService(ctx, "TestService");
    EXPECT_EQ(-1, svcId); //not build -> not found

    cmp.build();
    cmp.build(); //should be ok to call twice
    svcId = celix_bundleContext_findService(ctx, "TestService");
    EXPECT_GT(svcId, -1); //(re)build -> found

    dm.clear();
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm cleared so no components
    svcId = celix_bundleContext_findService(ctx, "TestService");
    EXPECT_EQ(svcId, -1); //cleared -> not found
}

TEST_F(DependencyManagerTestSuite, BuildSvcProvide) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<Cmp1>(std::make_shared<Cmp1>(), "test2");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    cmp.build();
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    TestService svc{};
    cmp.createProvidedCService(&svc, "CTestService").addProperty("key1", "val1").addProperty("key2", 3);

    long svcId = celix_bundleContext_findService(ctx, "CTestService");
    EXPECT_EQ(-1, svcId); //not build -> not found

    cmp.build();
    cmp.build(); //should be ok to call twice
    svcId = celix_bundleContext_findService(ctx, "CTestService");
    EXPECT_GT(svcId, -1); //(re)build -> found

    celix_service_filter_options_t opts{};
    opts.serviceName = "CTestService";
    opts.filter = "(&(key1=val1)(key2=3))";
    svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    EXPECT_GT(svcId, -1); //found, so properties present

    celix::dm::Properties props{};
    props["key1"] = "value";
    cmp.createProvidedService<TestService>().setProperties(props).setVersion("1.0.0").build();

    opts.serviceName = "TestService";
    opts.filter = "(key1=value)";
    opts.ignoreServiceLanguage = true;
    svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    EXPECT_GT(svcId, -1); //found, so properties present

    dm.clear();
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm cleared so no components
    svcId = celix_bundleContext_findService(ctx, "CTestService");
    EXPECT_EQ(svcId, -1); //cleared -> not found
}

TEST_F(DependencyManagerTestSuite, AddSvcDepAfterBuild) {
    celix::dm::DependencyManager dm{ctx};
    EXPECT_EQ(0, dm.getNrOfComponents());

    auto& cmp = dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1");
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm not started yet / comp not build yet
    EXPECT_TRUE(cmp.isValid());

    cmp.build();
    cmp.build(); //should be ok to call twice
    EXPECT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    std::atomic<int> count{0};
    auto& dep = cmp.createCServiceDependency<TestService>("TestService")
            .setCallbacks([&count](const TestService*, celix::dm::Properties&&) {
                count++;
            });

    TestService svc{};
    long svcId = celix_bundleContext_registerService(ctx, &svc, "TestService", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, &svc, "AnotherService", nullptr); //note should not be found.

    ASSERT_EQ(0, count); //service dep not yet build -> so no set call

    dep.build();
    dep.build(); //should be ok to call twice
    ASSERT_EQ(1, count); //service dep build -> so count is 1;

    //create another service dep
    cmp.createCServiceDependency<TestService>("TestService")
            .setCallbacks([&count](const TestService*, celix::dm::Properties&&) {
                count++;
            });
    ASSERT_EQ(1, count); //new service dep not yet build -> so count still 1

    cmp.build(); //cmp build, which will build svc dep
    ASSERT_EQ(2, count); //new service dep build -> so count is 2

    celix_bundleContext_unregisterService(ctx, svcId);
    celix_bundleContext_unregisterService(ctx, svcId2);
}

TEST_F(DependencyManagerTestSuite, InCompleteBuildShouldNotLeak) {
    celix::dm::DependencyManager dm{ctx};
    dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1"); //note not build

    auto& cmp2 = dm.createComponent<Cmp1>(std::make_shared<Cmp1>(), "test2").build();
    cmp2.createCServiceDependency<TestService>("TestService").setFilter("(key=value"); //note not build
    cmp2.createServiceDependency<TestService>().setFilter("(key=value)"); //note not build

    TestService svc{};
    cmp2.createProvidedCService(&svc, "CTestService").addProperty("key1", "val1"); //note not build
    cmp2.createProvidedService<TestService>().setVersion("1.0.0"); //note not build
}
