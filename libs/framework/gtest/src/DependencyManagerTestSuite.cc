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
        properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");


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

TEST_F(DependencyManagerTestSuite, DmComponentAddRemove) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp = celix_dmComponent_create(ctx, "test1");
    celix_dependencyManager_add(mng, cmp);
    ASSERT_EQ(1, celix_dependencyManager_nrOfComponents(mng));

    celix_dependencyManager_remove(mng, cmp);
    ASSERT_EQ(0, celix_dependencyManager_nrOfComponents(mng));

    auto *cmp2 = celix_dmComponent_create(ctx, "test2");
    auto *cmp3 = celix_dmComponent_create(ctx, "test3");
    celix_dependencyManager_add(mng, cmp2);
    celix_dependencyManager_add(mng, cmp3);
    ASSERT_EQ(2, celix_dependencyManager_nrOfComponents(mng));

    celix_dependencyManager_removeAllComponents(mng);
    ASSERT_EQ(0, celix_dependencyManager_nrOfComponents(mng));
}


TEST_F(DependencyManagerTestSuite, DmComponentAddRemoveAsync) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp1 = celix_dmComponent_create(ctx, "test1");
    celix_dependencyManager_addAsync(mng, cmp1);
    celix_dependencyManager_wait(mng);
    EXPECT_EQ(1, celix_dependencyManager_nrOfComponents(mng));

    std::atomic<std::size_t> count{0};
    auto cb = [](void *data) {
        auto* c = static_cast<std::atomic<std::size_t>*>(data);
        c->fetch_add(1);
    };

    celix_dependencyManager_removeAsync(mng, cmp1, &count, cb);
    celix_dependencyManager_wait(mng);
    EXPECT_EQ(0, celix_dependencyManager_nrOfComponents(mng));
    EXPECT_EQ(1, count.load());
}

TEST_F(DependencyManagerTestSuite, DmComponentRemoveAllAsync) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp1 = celix_dmComponent_create(ctx, "test1");
    auto *cmp2 = celix_dmComponent_create(ctx, "test2");
    celix_dependencyManager_add(mng, cmp1);
    celix_dependencyManager_add(mng, cmp2);
    EXPECT_EQ(2, celix_dependencyManager_nrOfComponents(mng));

    std::atomic<std::size_t> count{0};
    celix_dependencyManager_removeAllComponentsAsync(mng, &count, [](void *data) {
        auto* c = static_cast<std::atomic<std::size_t>*>(data);
        c->fetch_add(1);
    });
    celix_dependencyManager_wait(mng);
    EXPECT_EQ(0, celix_dependencyManager_nrOfComponents(mng));
    EXPECT_EQ(1, count.load());
}

TEST_F(DependencyManagerTestSuite, CDmGetInfo) {
    auto* mng = celix_bundleContext_getDependencyManager(ctx);
    auto* cmp = celix_dmComponent_create(ctx, "test1");

    auto* p = celix_properties_create();
    celix_properties_set(p, "key", "value");
    celix_dmComponent_addInterface(cmp, "test-interface", nullptr, (void*)0x42, p);

    auto* dep = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep, "test-interface", nullptr, nullptr);
    celix_dmComponent_addServiceDependency(cmp, dep);

    celix_dependencyManager_add(mng, cmp);

    auto* infos = celix_dependencyManager_createInfos(mng);
    ASSERT_EQ(1, celix_arrayList_size(infos));
    auto* dmInfo = (celix_dependency_manager_info_t*)celix_arrayList_get(infos, 0);
    ASSERT_EQ(1, celix_arrayList_size(dmInfo->components));
    auto *info = (celix_dm_component_info_t*)celix_arrayList_get(dmInfo->components, 0);
    EXPECT_EQ(1, celix_arrayList_size(info->interfaces));
    EXPECT_EQ(1, celix_arrayList_size(info->dependency_list));
    celix_dependencyManager_destroyInfos(mng, infos);
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

struct TestService {
    void *handle = nullptr;
};

class Cmp1 : public TestService {

};

class Cmp2 : public TestService {
public:
    explicit Cmp2(const std::string& name) {
        std::cout << "usage arg: " << name << std::endl;
    }
};


TEST_F(DependencyManagerTestSuite, CxxDmGetInfo) {
    celix::dm::DependencyManager mng{ctx};

    auto& cmp = mng.createComponent<Cmp1>();
    cmp.createProvidedService<TestService>().addProperty("key", "value");
    cmp.createServiceDependency<TestService>().setVersionRange("[1,2)").setRequired(true);

    auto infos = mng.getInfos();
    EXPECT_EQ(infos.size(), 1);

    auto info = mng.getInfo();
    EXPECT_EQ(info.bndId, 0);
    EXPECT_EQ(info.bndSymbolicName, "Framework");
    EXPECT_EQ(info.components.size(), 0); //not build yet

    mng.build();
    info = mng.getInfo(); //new info
    ASSERT_EQ(info.components.size(), 1); //build

    auto& cmpInfo = info.components[0];
    EXPECT_TRUE(!cmpInfo.uuid.empty());
    EXPECT_EQ(cmpInfo.name, "Cmp1");
    EXPECT_EQ(cmpInfo.state, "WAITING_FOR_REQUIRED");
    EXPECT_FALSE(cmpInfo.isActive);
    EXPECT_EQ(cmpInfo.nrOfTimesStarted, 0);
    EXPECT_EQ(cmpInfo.nrOfTimesResumed, 0);

    ASSERT_EQ(cmpInfo.interfacesInfo.size(), 1);
    auto& intf = cmpInfo.interfacesInfo[0];
    EXPECT_EQ(intf.serviceName, "TestService");
    EXPECT_TRUE(intf.properties.size() > 0);
    EXPECT_NE(intf.properties.find("key"), intf.properties.end());

    ASSERT_EQ(cmpInfo.dependenciesInfo.size(), 1);
    auto& dep = cmpInfo.dependenciesInfo[0];
    EXPECT_EQ(dep.serviceName, "TestService");
    EXPECT_EQ(dep.isRequired, true);
    EXPECT_EQ(dep.versionRange, "[1,2)");
    EXPECT_EQ(dep.isAvailable, false);
    EXPECT_EQ(dep.nrOfTrackedServices, 0);
}

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
    dm.wait();
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
    dm.wait();
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
    dm.wait();
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
    celix_bundleContext_waitForEvents(ctx);
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
    dm.createComponent<TestComponent>(std::make_shared<TestComponent>(), "test1"); //NOTE NOT BUILD

    auto& cmp2 = dm.createComponent<Cmp1>(std::make_shared<Cmp1>(), "test2").build(); //NOTE BUILD
    cmp2.createCServiceDependency<TestService>("TestService").setFilter("(key=value"); //note not build
    cmp2.createServiceDependency<TestService>().setFilter("(key=value)").setName("alternative name"); //note not build

    TestService svc{};
    cmp2.createProvidedCService(&svc, "CTestService").addProperty("key1", "val1"); //note not build
    cmp2.createProvidedService<TestService>().setVersion("1.0.0"); //note not build
}


TEST_F(DependencyManagerTestSuite, RemoveAndClear) {
    celix::dm::DependencyManager dm{ctx};
    auto& cmp1 = dm.createComponent<TestComponent>(std::make_shared<TestComponent>()).build();
    auto& cmp2 = dm.createComponent<TestComponent>(std::make_shared<TestComponent>()).build();
    dm.createComponent<TestComponent>(std::make_shared<TestComponent>()).build();
    auto& cmp4 = dm.createComponent<TestComponent>(std::make_shared<TestComponent>());
    dm.wait();

    dm.destroyComponent(cmp1);
    bool removed = dm.removeComponent(cmp2.getUUID());
    EXPECT_TRUE(removed);
    removed = dm.removeComponent(cmp4.getUUID());
    EXPECT_TRUE(removed);
    dm.clear();
}
