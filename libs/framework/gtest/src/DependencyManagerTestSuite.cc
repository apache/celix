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
#include <condition_variable>
#include <cstdint>

#include "celix/dm/DependencyManager.h"
#include "celix_framework_factory.h"
#include "framework.h"

class DependencyManagerTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    DependencyManagerTestSuite() {
        properties = celix_properties_create();
        celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");
        celix_properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");


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

TEST_F(DependencyManagerTestSuite, DmComponentsWithConfiguredDestroyFunction) {
    //Given a simple component implementation (only a name field)
    struct CmpImpl {
        std::string name;
    };

    void(*destroyFn)(CmpImpl*) = [](CmpImpl* impl) {
        std::cout << "Destroying " << impl->name << std::endl;
        delete impl;
    };

    //When 10 of these components impls are created and configured (include destroy impl function)
    //as component in the DM.
    auto* dm = celix_bundleContext_getDependencyManager(ctx);
    for (int i = 0; i < 10; ++i) {
        auto* impl = new CmpImpl{std::string{"Component"} + std::to_string(i+1)};
        auto* dmCmp = celix_dmComponent_create(ctx, impl->name.c_str());
        celix_dmComponent_setImplementation(dmCmp, impl);
        CELIX_DM_COMPONENT_SET_IMPLEMENTATION_DESTROY_FUNCTION(dmCmp, CmpImpl, destroyFn);
        celix_dependencyManager_addAsync(dm, dmCmp);
    }

    //Then all component should become activate (note components with no svc dependencies)
    celix_dependencyManager_wait(dm);
    celix_dependencyManager_allComponentsActive(dm);
    EXPECT_EQ(celix_dependencyManager_nrOfComponents(dm), 10);

    //When all 10 components are removed
    celix_dependencyManager_removeAllComponents(dm);

    //Then all components should be removed
    EXPECT_EQ(celix_dependencyManager_nrOfComponents(dm), 0);

    //And when the test goes out of scope, no memory should be leaked
    //nop
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
    auto callback = [](void *data) {
        auto* c = static_cast<std::atomic<std::size_t>*>(data);
        c->fetch_add(1);
    };
    celix_dependencyManager_removeAllComponentsAsync(mng, &count, callback);
    celix_dependencyManager_wait(mng);
    EXPECT_EQ(0, celix_dependencyManager_nrOfComponents(mng));
    EXPECT_EQ(1, count.load());

    //call again if all components are removed -> should call callback
    celix_dependencyManager_removeAllComponentsAsync(mng, &count, callback);
    celix_dependencyManager_wait(mng);
    EXPECT_EQ(0, celix_dependencyManager_nrOfComponents(mng));
    EXPECT_EQ(2, count.load());
}

TEST_F(DependencyManagerTestSuite, InvalidAddInterface) {
    auto* cmp = celix_dmComponent_create(ctx, "test");
    void* dummyInterfacePointer = (void*)0x42;

    auto status = celix_dmComponent_addInterface(cmp, "", nullptr, dummyInterfacePointer, nullptr);
    EXPECT_NE(status, CELIX_SUCCESS);

    status = celix_dmComponent_addInterface(cmp, nullptr, nullptr, dummyInterfacePointer, nullptr);
    EXPECT_NE(status, CELIX_SUCCESS);

    celix_dmComponent_destroy(cmp);
}

TEST_F(DependencyManagerTestSuite, DmGetInfo) {
    auto* mng = celix_bundleContext_getDependencyManager(ctx);
    auto* cmp = celix_dmComponent_create(ctx, "test1");

    void* dummyInterfacePointer = (void*)0x42;

    auto* p = celix_properties_create();
    celix_properties_set(p, "key", "value");
    celix_dmComponent_addInterface(cmp, "test-interface", nullptr, dummyInterfacePointer, p);

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
    auto *intInf = (dm_interface_info_t*)celix_arrayList_get(info->interfaces, 0);
    EXPECT_EQ(2, celix_properties_size(intInf->properties)); //key and component.uuid
    EXPECT_EQ(1, celix_arrayList_size(info->dependency_list));
    celix_arrayList_destroy(infos);
}

TEST_F(DependencyManagerTestSuite, DmInterfaceAddRemove) {
    auto *mng = celix_bundleContext_getDependencyManager(ctx);
    auto *cmp = celix_dmComponent_create(ctx, "test1");

    void *dummyInterfacePointer = (void *) 0x42;

    auto *p = celix_properties_create();
    celix_properties_set(p, "key", "value");
    celix_dmComponent_addInterface(cmp, "test-interface", nullptr, dummyInterfacePointer, p);

    auto *dep = celix_dmServiceDependency_create();
    celix_dmServiceDependency_setService(dep, "test-interface", nullptr, nullptr);
    celix_dmComponent_addServiceDependency(cmp, dep);

    celix_dependencyManager_add(mng, cmp);

    celix_dmComponent_removeInterface(cmp, dummyInterfacePointer);

    auto *infos = celix_dependencyManager_createInfos(mng);
    ASSERT_EQ(1, celix_arrayList_size(infos));
    auto *dmInfo = (celix_dependency_manager_info_t *) celix_arrayList_get(infos, 0);
    ASSERT_EQ(1, celix_arrayList_size(dmInfo->components));
    auto *info = (celix_dm_component_info_t *) celix_arrayList_get(dmInfo->components, 0);
    EXPECT_EQ(0, celix_arrayList_size(info->interfaces));
    celix_arrayList_destroy(infos);
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

class Cmp3 /*note no inherit*/ {

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
    EXPECT_EQ(info.bndSymbolicName, "apache_celix_framework");
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
    EXPECT_TRUE(!intf.properties.empty());
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
    dm.waitIfAble();
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
    svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    EXPECT_GT(svcId, -1); //found, so properties present

    dm.clear();
    dm.wait();
    EXPECT_EQ(0, dm.getNrOfComponents()); //dm cleared so no components
    svcId = celix_bundleContext_findService(ctx, "CTestService");
    EXPECT_EQ(svcId, -1); //cleared -> not found
}

TEST_F(DependencyManagerTestSuite, BuildUnassociatedProvidedService) {
    celix::dm::DependencyManager dm{ctx};

    //Given a component which does not inherit any interfaces
    auto& cmp = dm.createComponent<Cmp1>(std::make_shared<Cmp1>());

    //Then I can create a provided service using a shared_ptr which is not associated with the component type
    // (TestService is not a base of Cmp3)
    cmp.createUnassociatedProvidedService(std::make_shared<TestService>())
        .addProperty("test1", "value1");

    //And I can create a provided service using a shared_ptr and a custom name
    cmp.createUnassociatedProvidedService(std::make_shared<TestService>(), "CustomName");

    //When I build the component
    cmp.build();

    //Then the nr of component is 1
    ASSERT_EQ(1, dm.getNrOfComponents()); //cmp "build", so active

    //And the nr of provided interfaces of that component is 2
    auto info = dm.getInfo();
    ASSERT_EQ(info.components[0].interfacesInfo.size(), 2);

    //And the first (index 0) provided service has a name TestService and a property test1 with value value1
    EXPECT_STREQ(info.components[0].interfacesInfo[0].serviceName.c_str(), "TestService");
    EXPECT_STREQ(info.components[0].interfacesInfo[0].properties["test1"].c_str(), "value1");

    //And the second (index 1) provide service has a name "CustomName".
    EXPECT_STREQ(info.components[0].interfacesInfo[1].serviceName.c_str(), "CustomName");
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
    removed = dm.removeComponentAsync(cmp4.getUUID());
    EXPECT_TRUE(removed);
    dm.clear();
}

TEST_F(DependencyManagerTestSuite, RequiredDepsAreInjectedDuringStartStop) {
    class LifecycleComponent {
    public:
        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void stop() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void setService(TestService* svc) {
            std::lock_guard<std::mutex> lck{mutex};
            setSvc = svc;
        }

        void addService(TestService* svc) {
            std::lock_guard<std::mutex> lck{mutex};
            services.emplace_back(svc);
        }

        void remService(TestService* svc) {
            std::lock_guard<std::mutex> lck{mutex};
            for (auto it = services.begin(); it != services.end(); ++it) {
                if (*it == svc) {
                    services.erase(it);
                    break;
                }
            }
        }
    private:
        std::mutex mutex{};
        TestService* setSvc{};
        std::vector<TestService*> services{};
    };

    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<LifecycleComponent>()
            .setCallbacks(nullptr, &LifecycleComponent::start, &LifecycleComponent::stop, nullptr);
    cmp.createServiceDependency<TestService>()
            .setRequired(true)
            .setCallbacks(&LifecycleComponent::setService)
            .setCallbacks(&LifecycleComponent::addService, &LifecycleComponent::remService);
    cmp.build();

    TestService svc;
    std::string svcName = celix::typeName<TestService>();
    celix_service_registration_options opts{};
    opts.svc = &svc;
    opts.serviceName = svcName.c_str();
    long svcId = celix_bundleContext_registerServiceWithOptions(dm.bundleContext(), &opts);
    EXPECT_GE(svcId, 0);

    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);
    celix_bundleContext_unregisterService(dm.bundleContext(), svcId);
}

TEST_F(DependencyManagerTestSuite, RemoveOwnDependencyShouldNotLeadToDoubleStop) {
    //Given a component LifecycleComponent which provides and requires a TestService service
    class LifecycleComponent : public TestService {
    public:
        void start() {
            startCount.fetch_add(1);
        }

        void stop() {
            stopCount.fetch_add(1);
        }

        int getStartCount() const { return startCount.load(); }

        int getStopCount() const { return stopCount.load(); }
    private:
        std::atomic<int> startCount{0};
        std::atomic<int> stopCount{0};
    };

    celix::dm::DependencyManager dm{ctx};
    auto lifecycleCmp = std::make_shared<LifecycleComponent>();
    auto& cmp = dm.createComponent<LifecycleComponent>(lifecycleCmp)
            .setCallbacks(nullptr, &LifecycleComponent::start, &LifecycleComponent::stop, nullptr);
    cmp.createProvidedService<TestService>();
    cmp.createServiceDependency<TestService>()
            .setRequired(true);
    cmp.build();
    using celix::dm::ComponentState;

    //Then the component state should be waiting for required
    EXPECT_EQ(cmp.getState(), ComponentState::WAITING_FOR_REQUIRED);

    //When a TestService is registered
    TestService svc{};
    long svcId = celix_bundleContext_registerService(ctx, &svc, "TestService", nullptr);
    celix_bundleContext_waitForEvents(ctx);

    //Then the component state should become tracking optional (active)
    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);
    //And the start count should become 1
    EXPECT_EQ(lifecycleCmp->getStartCount(), 1);

    //When the TestService is unregistered
    celix_bundleContext_unregisterService(ctx, svcId);

    //Then the component state will stay tracking optional (because it now depends on its own provided services)
    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);
    EXPECT_EQ(lifecycleCmp->getStopCount(), 0);

    //When an additional required service dependency is added
    cmp.createServiceDependency<TestService>("DummyName")
            .setRequired(true)
            .buildAsync();

    celix_bundleContext_waitForEvents(ctx);
    //Then the component state becomes waiting for required
    EXPECT_EQ(cmp.getState(), ComponentState::INSTANTIATED_AND_WAITING_FOR_REQUIRED);
    //And the stop count becomes 1 (not 2 becomes of a loop the dm component handling)
    EXPECT_EQ(lifecycleCmp->getStopCount(), 1);
}


TEST_F(DependencyManagerTestSuite, IntermediateStatesDuringInitDeinitStartingAndStopping) {
    //Given a component LifecycleComponent with an optional service dependency on TestService with a suspend-strategy
    //and a required service dependency on "RequiredTestService" with a locking-strategy.
    class LifecycleComponent {
    public:
        enum class LifecycleMethod : std::int8_t {
            None =      0,
            Init =      1,
            Start =     2,
            Stop =      3,
            Deinit =    4,
        };

        void init() {
            std::cout << "in init callback\n";
            setStateAndWaitToContinue(LifecycleMethod::Init);
        }

        void deinit() {
            std::cout << "in deinit callback\n";
            setStateAndWaitToContinue(LifecycleMethod::Deinit);
        }

        void start() {
            std::cout << "in start callback\n";
            setStateAndWaitToContinue(LifecycleMethod::Start);
        }

        void stop() {
            std::cout << "in stop callback\n";
            setStateAndWaitToContinue(LifecycleMethod::Stop);
        }

        void setStayInMethod(LifecycleMethod m) {
            std::lock_guard<std::mutex> lck{mutex};
            stayInMethod = m;
            cond.notify_all();
        }

        void waitUntilInMethod(LifecycleMethod m) {
            std::unique_lock<std::mutex> lck{mutex};
            cond.wait_for(lck, std::chrono::seconds{5}, [&]{ return currentMethod == m; });
        }
    private:
        void setStateAndWaitToContinue(LifecycleMethod m) {
            std::unique_lock<std::mutex> lck{mutex};
            currentMethod = m;
            cond.notify_all();
            cond.wait_for(lck, std::chrono::seconds{5}, [&] { return currentMethod != stayInMethod; });
            currentMethod = LifecycleMethod::None;
            cond.notify_all();
        }

        std::mutex mutex{};
        std::condition_variable cond{};
        LifecycleMethod stayInMethod = LifecycleMethod::None;
        LifecycleMethod currentMethod = LifecycleMethod::None;
    };
    using LifecycleMethod = LifecycleComponent::LifecycleMethod;


    celix::dm::DependencyManager dm{ctx};
    auto lifecycleCmp = std::make_shared<LifecycleComponent>();
    auto& cmp = dm.createComponent<LifecycleComponent>(lifecycleCmp)
            .setCallbacks(&LifecycleComponent::init, &LifecycleComponent::start, &LifecycleComponent::stop, &LifecycleComponent::deinit);
    cmp.createServiceDependency<TestService>()
            .setStrategy(celix::dm::DependencyUpdateStrategy::suspend)
            .setCallbacks([](std::shared_ptr<TestService> /*service*/, const std::shared_ptr<const celix::Properties>& /*properties*/){ std::cout << "Dummy set for svc callback\n"; })
            .setRequired(false);
    cmp.createServiceDependency<TestService>("RequiredTestService")
            .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
            .setRequired(true);
    cmp.buildAsync();

    //Then the component state should become waiting for required
    cmp.wait();
    EXPECT_EQ(cmp.getState(), ComponentState::WAITING_FOR_REQUIRED);

    //When the component should wait in the init callback
    lifecycleCmp->setStayInMethod(LifecycleMethod::Init);

    //And a "RequiredTestService" service is registered
    TestService reqSvc{};
    long reqSvcId = celix_bundleContext_registerServiceAsync(ctx, &reqSvc, "RequiredTestService", nullptr);
    EXPECT_GE(reqSvcId, 0);

    //Then the component state should become initializing
    using celix::dm::ComponentState;
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Init);
    EXPECT_EQ(cmp.getState(), ComponentState::INITIALIZING);

    //When the component should wait in the start callback
    //Then the component state should become starting
    lifecycleCmp->setStayInMethod(LifecycleMethod::Start);
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Start);
    EXPECT_EQ(cmp.getState(), ComponentState::STARTING);

    //When the component should not wait in the lifecycle callbacks
    lifecycleCmp->setStayInMethod(LifecycleMethod::None);

    //Then the component state should become tracking optional
    cmp.wait();
    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);


    //When the component should wait in the stop callback
    lifecycleCmp->setStayInMethod(LifecycleMethod::Stop);

    //And a new TestService is registered (leading to a suspending of the component)
    TestService svc;
    std::string svcName = celix::typeName<TestService>();
    celix_service_registration_options opts{};
    opts.svc = &svc;
    opts.serviceName = svcName.c_str();
    long optionalSvcId = celix_bundleContext_registerServiceWithOptionsAsync(dm.bundleContext(), &opts);
    EXPECT_GE(optionalSvcId, 0);

    //Then the component state should become suspending
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Stop);
    EXPECT_EQ(cmp.getState(), ComponentState::SUSPENDING);

    //When the component should wait in the start callback
    //Then the component state should become resuming (suspending->suspended->resuming)
    lifecycleCmp->setStayInMethod(LifecycleMethod::Start);
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Start);
    EXPECT_EQ(cmp.getState(), ComponentState::RESUMING);

    //When the component should not wait in the lifecycle callbacks
    lifecycleCmp->setStayInMethod(LifecycleMethod::None);

    //Then the component state should become tracking optional
    cmp.wait();
    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);

    //When the component should wait in the stop callback
    lifecycleCmp->setStayInMethod(LifecycleMethod::Stop);

    //And the "RequiredTestService" is unregistered
    celix_bundleContext_unregisterServiceAsync(ctx, reqSvcId, nullptr, nullptr);

    //Then the component state should become stopping
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Stop);
    EXPECT_EQ(cmp.getState(), ComponentState::STOPPING);

    //When the component should not wait in the lifecycle callbacks
    lifecycleCmp->setStayInMethod(LifecycleMethod::None);

    //Then the component state should become instantiated and waiting for required
    cmp.wait();
    EXPECT_EQ(cmp.getState(), ComponentState::INSTANTIATED_AND_WAITING_FOR_REQUIRED);

    //When the component should wait in the deinit callback
    lifecycleCmp->setStayInMethod(LifecycleMethod::Deinit);

    //And the component is removed from the dependency manager
    dm.removeComponentAsync(cmp.getUUID());

    //Then the component state should become deinitializing
    lifecycleCmp->waitUntilInMethod(LifecycleMethod::Deinit);
    EXPECT_EQ(cmp.getState(), ComponentState::DEINITIALIZING);

    lifecycleCmp->setStayInMethod(LifecycleMethod::None);
    celix_bundleContext_unregisterService(dm.bundleContext(), optionalSvcId);
}

TEST_F(DependencyManagerTestSuite, DepsAreInjectedAsSharedPointers) {
    class LifecycleComponent {
    public:
        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void stop() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void setService(const std::shared_ptr<TestService>& svc, const std::shared_ptr<const celix::Properties>& /*props*/) {
            std::lock_guard<std::mutex> lck{mutex};
            setSvc = svc;
        }

        void addService(const std::shared_ptr<TestService>& svc, const std::shared_ptr<const celix::Properties>& props) {
            EXPECT_TRUE(props);
            std::lock_guard<std::mutex> lck{mutex};
            services.emplace_back(svc);
        }

        void remService(const std::shared_ptr<TestService>& svc, const std::shared_ptr<const celix::Properties>& props) {
            EXPECT_TRUE(props);
            std::lock_guard<std::mutex> lck{mutex};
            for (auto it = services.begin(); it != services.end(); ++it) {
                if (*it == svc) {
                    services.erase(it);
                    break;
                }
            }
        }
    private:
        std::mutex mutex{};
        std::shared_ptr<TestService> setSvc{};
        std::vector<std::shared_ptr<TestService>> services{};
    };

    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<LifecycleComponent>()
            .setCallbacks(nullptr, &LifecycleComponent::start, &LifecycleComponent::stop, nullptr);
    cmp.createServiceDependency<TestService>()
            .setRequired(true)
            .setCallbacks(&LifecycleComponent::setService)
            .setCallbacks(&LifecycleComponent::addService, &LifecycleComponent::remService);
    cmp.build();

    TestService svc;
    std::string svcName = celix::typeName<TestService>();
    celix_service_registration_options opts{};
    opts.svc = &svc;
    opts.serviceName = svcName.c_str();
    long svcId = celix_bundleContext_registerServiceWithOptions(dm.bundleContext(), &opts);
    EXPECT_GE(svcId, 0);

    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);
    celix_bundleContext_unregisterService(dm.bundleContext(), svcId);
}

TEST_F(DependencyManagerTestSuite, DepsNoPropsAreInjectedAsSharedPointers) {
    class LifecycleComponent {
    public:
        void start() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void stop() {
            std::lock_guard<std::mutex> lck{mutex};
            EXPECT_TRUE(setSvc != nullptr);
            EXPECT_EQ(services.size(), 1);
        }

        void setService(const std::shared_ptr<TestService>& svc) {
            std::lock_guard<std::mutex> lck{mutex};
            setSvc = svc;
        }

        void addService(const std::shared_ptr<TestService>& svc) {
            std::lock_guard<std::mutex> lck{mutex};
            services.emplace_back(svc);
        }

        void remService(const std::shared_ptr<TestService>& svc) {
            std::lock_guard<std::mutex> lck{mutex};
            for (auto it = services.begin(); it != services.end(); ++it) {
                if (*it == svc) {
                    services.erase(it);
                    break;
                }
            }
        }
    private:
        std::mutex mutex{};
        std::shared_ptr<TestService> setSvc{};
        std::vector<std::shared_ptr<TestService>> services{};
    };

    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<LifecycleComponent>()
            .setCallbacks(nullptr, &LifecycleComponent::start, &LifecycleComponent::stop, nullptr);
    cmp.createServiceDependency<TestService>()
            .setRequired(true)
            .setCallbacks(&LifecycleComponent::setService)
            .setCallbacks(&LifecycleComponent::addService, &LifecycleComponent::remService);
    cmp.build();

    TestService svc;
    std::string svcName = celix::typeName<TestService>();
    celix_service_registration_options opts{};
    opts.svc = &svc;
    opts.serviceName = svcName.c_str();
    long svcId = celix_bundleContext_registerServiceWithOptions(dm.bundleContext(), &opts);
    EXPECT_GE(svcId, 0);

    EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);
    celix_bundleContext_unregisterService(dm.bundleContext(), svcId);
}

TEST_F(DependencyManagerTestSuite, UnneededSuspendIsPrevented) {
    class CounterComponent {
    public:
        void start() {
            startCount++;
        }

        void stop() {
            stopCount++;
        }

        void setService(TestService* /*svc*/) {
            //nop
        }

        void addService(TestService* /*svc*/) {
            //nop
        }

        void remService(TestService* /*svc*/) {
            //nop
        }

        std::atomic<int> startCount{0};
        std::atomic<int> stopCount{0};
    };

    celix::dm::DependencyManager dm{ctx};
    //cmp1 has lifecycle callbacks, but not set or add/rem callbacks for the service dependency -> should not trigger suspend
    auto& cmp1 = dm.createComponent<CounterComponent>("CounterCmp1")
            .setCallbacks(nullptr, &CounterComponent::start, &CounterComponent::stop, nullptr);
    cmp1.createServiceDependency<TestService>();
    cmp1.build();

    //cmp2 has lifecycle callbacks and set, add/rem callbacks for the service dependency -> should trigger suspend 2x
    auto& cmp2 = dm.createComponent<CounterComponent>("CounterCmp2")
            .setCallbacks(nullptr, &CounterComponent::start, &CounterComponent::stop, nullptr);
    cmp2.createServiceDependency<TestService>()
            .setCallbacks(&CounterComponent::setService)
            .setCallbacks(&CounterComponent::addService, &CounterComponent::remService);
    cmp2.build();

    EXPECT_EQ(cmp1.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);
    EXPECT_EQ(cmp2.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);

    TestService svc;
    std::string svcName = celix::typeName<TestService>();
    celix_service_registration_options opts{};
    opts.svc = &svc;
    opts.serviceName = svcName.c_str();
    long svcId = celix_bundleContext_registerServiceWithOptions(dm.bundleContext(), &opts);
    EXPECT_GE(svcId, 0);

    EXPECT_EQ(cmp1.getInstance().startCount, 1); //only once during creation
    EXPECT_EQ(cmp1.getInstance().stopCount, 0);
    EXPECT_EQ(cmp2.getInstance().startCount, 3); //1x creation, 1x suspend for set, 1x suspend for add
    EXPECT_EQ(cmp2.getInstance().stopCount, 2); //1x suspend for set, 1x suspend for add

    cmp1.getInstance().startCount = 0;
    cmp1.getInstance().stopCount = 0;
    cmp2.getInstance().startCount = 0;
    cmp2.getInstance().stopCount = 0;
    celix_bundleContext_unregisterService(dm.bundleContext(), svcId);

    EXPECT_EQ(cmp1.getInstance().startCount, 0);
    EXPECT_EQ(cmp1.getInstance().stopCount, 0);
    EXPECT_EQ(cmp2.getInstance().startCount, 2); //1x suspend for set nullptr, 1x suspend for rem
    EXPECT_EQ(cmp2.getInstance().stopCount, 2); //1x suspend for set nullptr, 1x suspend for rem
}

TEST_F(DependencyManagerTestSuite, ExceptionsInLifecycle) {
    class ExceptionComponent {
    public:
        enum class LifecycleMethod : std::uint8_t {
            INIT,
            START,
            STOP,
            DEINIT
        };

        explicit ExceptionComponent(LifecycleMethod _failAt) : failAt{_failAt} {}

        void failFor(LifecycleMethod lm) {
            if (lm == failAt) {
                throw std::logic_error("fail test");
            }
        }

        void init() {
            failFor(LifecycleMethod::INIT);
        }

        void start() {
            failFor(LifecycleMethod::START);
        }

        void stop() {
            failFor(LifecycleMethod::STOP);
        }

        void deinit() {
            failFor(LifecycleMethod::DEINIT);
        }

    private:
        const LifecycleMethod failAt;
    };

    celix::dm::DependencyManager dm{ctx};

    {
        auto& cmp = dm.createComponent(std::make_shared<ExceptionComponent>(ExceptionComponent::LifecycleMethod::INIT), "FailAtInitCmp")
                .setCallbacks(&ExceptionComponent::init, &ExceptionComponent::start, &ExceptionComponent::stop, &ExceptionComponent::deinit);
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        cmp.build(); //fails at init and should disable
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        dm.clear();
    }

    {
        auto& cmp = dm.createComponent(std::make_shared<ExceptionComponent>(ExceptionComponent::LifecycleMethod::START), "FailAtStartCmp")
                .setCallbacks(&ExceptionComponent::init, &ExceptionComponent::start, &ExceptionComponent::stop, &ExceptionComponent::deinit);
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        cmp.build(); //fails at init and should disable
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        dm.clear();
    }

    {
        auto& cmp = dm.createComponent(std::make_shared<ExceptionComponent>(ExceptionComponent::LifecycleMethod::STOP), "FailAtStopCmp")
                .setCallbacks(&ExceptionComponent::init, &ExceptionComponent::start, &ExceptionComponent::stop, &ExceptionComponent::deinit);
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        cmp.build();
        EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);

        //required service -> should stop, but fails at stop and should become inactive (component will disable itself)
        cmp.createServiceDependency<TestService>().setRequired(true).build();
        cmp.wait();
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        dm.clear();
    }

    {
        auto& cmp = dm.createComponent(std::make_shared<ExceptionComponent>(ExceptionComponent::LifecycleMethod::DEINIT), "FailAtDeinit")
                .setCallbacks(&ExceptionComponent::init, &ExceptionComponent::start, &ExceptionComponent::stop, &ExceptionComponent::deinit);
        EXPECT_EQ(cmp.getState(), ComponentState::INACTIVE);
        cmp.build();
        EXPECT_EQ(cmp.getState(), ComponentState::TRACKING_OPTIONAL);

        //required service -> should stop, but fails at stop and should become inactive (component will disable itself)
        dm.clear(); //dm clear will deinit component and this should fail, but not deadlock
    }
}

TEST_F(DependencyManagerTestSuite, ComponentContextShouldNotLeak) {
    celix::dm::DependencyManager dm{ctx};
    dm.createComponent<TestComponent>()
            .addContext(std::make_shared<std::vector<std::string>>())
            .build();
    //note should not leak mem
}

TEST_F(DependencyManagerTestSuite, installBundleWithDepManActivator) {
    auto* list = celix_bundleContext_listBundles(ctx);
    EXPECT_EQ(celix_arrayList_size(list), 0);
    celix_arrayList_destroy(list);

    auto bndId = celix_bundleContext_installBundle(ctx, SIMPLE_CXX_DEP_MAN_BUNDLE_LOC, true);
    EXPECT_GT(bndId, CELIX_FRAMEWORK_BUNDLE_ID);

    list = celix_bundleContext_listBundles(ctx);
    EXPECT_EQ(celix_arrayList_size(list), 1);
    celix_arrayList_destroy(list);
}

TEST_F(DependencyManagerTestSuite, testStateToString) {
    const char* state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_INACTIVE);
    EXPECT_STREQ(state, "INACTIVE");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_WAITING_FOR_REQUIRED);
    EXPECT_STREQ(state, "WAITING_FOR_REQUIRED");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_INITIALIZING);
    EXPECT_STREQ(state, "INITIALIZING");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_DEINITIALIZING);
    EXPECT_STREQ(state, "DEINITIALIZING");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_INITIALIZED_AND_WAITING_FOR_REQUIRED);
    EXPECT_STREQ(state, "INITIALIZED_AND_WAITING_FOR_REQUIRED");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_STARTING);
    EXPECT_STREQ(state, "STARTING");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_STOPPING);
    EXPECT_STREQ(state, "STOPPING");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_TRACKING_OPTIONAL);
    EXPECT_STREQ(state, "TRACKING_OPTIONAL");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_SUSPENDING);
    EXPECT_STREQ(state, "SUSPENDING");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_SUSPENDED);
    EXPECT_STREQ(state, "SUSPENDED");
    state = celix_dmComponent_stateToString(CELIX_DM_CMP_STATE_RESUMING);
    EXPECT_STREQ(state, "RESUMING");
    state = celix_dmComponent_stateToString((celix_dm_component_state_enum)0);
    EXPECT_STREQ(state, "INACTIVE");
    state = celix_dmComponent_stateToString((celix_dm_component_state_enum)16);
    EXPECT_STREQ(state, "INACTIVE");

    //test deprecated dm cmp states
    state = celix_dmComponent_stateToString(DM_CMP_STATE_INACTIVE);
    EXPECT_STREQ(state, "INACTIVE");
    state = celix_dmComponent_stateToString(DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED);
    EXPECT_STREQ(state, "INITIALIZED_AND_WAITING_FOR_REQUIRED");
    state = celix_dmComponent_stateToString(DM_CMP_STATE_TRACKING_OPTIONAL);
    EXPECT_STREQ(state, "TRACKING_OPTIONAL");
    state = celix_dmComponent_stateToString(DM_CMP_STATE_WAITING_FOR_REQUIRED);
    EXPECT_STREQ(state, "WAITING_FOR_REQUIRED");

}

class TestInterface {
public:
    static constexpr const char * const NAME = "TestName";
    static constexpr const char * const VERSION = "1.2.3";
};

TEST_F(DependencyManagerTestSuite, ProvideInterfaceInfo) {
    class TestComponent : public TestInterface {};
    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<TestComponent>();
    cmp.createProvidedService<TestInterface>(TestInterface::NAME)
            .setVersion(TestInterface::VERSION);
    cmp.build();
    EXPECT_EQ(cmp.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);

    auto info = dm.getInfo();
    EXPECT_EQ(info.components[0].interfacesInfo[0].serviceName, "TestName");
    auto& props = info.components[0].interfacesInfo[0].properties;
    const auto it = props.find(celix::SERVICE_VERSION);
    ASSERT_TRUE(it != props.end());
    EXPECT_EQ(it->second, "1.2.3");
}

TEST_F(DependencyManagerTestSuite, CreateInterfaceInfo) {
    class TestComponent : public TestInterface {};
    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<TestComponent>();
    cmp.addInterfaceWithName<TestInterface>(TestInterface::NAME, TestInterface::VERSION);
    cmp.build();
    EXPECT_EQ(cmp.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);

    auto info = dm.getInfo();
    EXPECT_EQ(info.components[0].interfacesInfo[0].serviceName, "TestName");
    auto& props = info.components[0].interfacesInfo[0].properties;
    const auto it = props.find(celix::SERVICE_VERSION);
    ASSERT_TRUE(it != props.end());
    EXPECT_EQ(it->second, "1.2.3");
}

TEST_F(DependencyManagerTestSuite, TestPrintInfo) {
    celix::dm::DependencyManager dm{ctx};
    auto& cmp = dm.createComponent<Cmp1>();
    cmp.addInterface<TestService>();
    cmp.createServiceDependency<TestInterface>(TestInterface::NAME);
    cmp.build();

    char* buf = nullptr;
    size_t bufLen = 0;
    FILE* testStream = open_memstream(&buf, &bufLen);
    celix_dependencyManager_printInfo(celix_bundleContext_getDependencyManager(ctx), true, true, testStream);
    fclose(testStream);
    std::cout << buf << std::endl;
    EXPECT_TRUE(strstr(buf, "Cmp1")); //cmp name
    EXPECT_TRUE(strstr(buf, "TestService")); //provided service name
    EXPECT_TRUE(strstr(buf, "TestName")); //service dependency name
    free(buf);

    buf = nullptr;
    bufLen = 0;
    testStream = open_memstream(&buf, &bufLen);
    celix_dependencyManager_printInfo(celix_bundleContext_getDependencyManager(ctx), true, false, testStream);
    fclose(testStream);
    EXPECT_TRUE(strstr(buf, "Cmp1")); //cmp name
    EXPECT_TRUE(strstr(buf, "TestService")); //provided service name
    EXPECT_TRUE(strstr(buf, "TestName")); //service dependency name
    free(buf);

    buf = nullptr;
    bufLen = 0;
    testStream = open_memstream(&buf, &bufLen);
    celix_dependencyManager_printInfo(celix_bundleContext_getDependencyManager(ctx), false, true, testStream);
    fclose(testStream);
    EXPECT_TRUE(strstr(buf, "Cmp1")); //cmp name
    free(buf);

    buf = nullptr;
    bufLen = 0;
    testStream = open_memstream(&buf, &bufLen);
    celix_dependencyManager_printInfo(celix_bundleContext_getDependencyManager(ctx), false, false, testStream);
    fclose(testStream);
    EXPECT_TRUE(strstr(buf, "Cmp1")); //cmp name
    free(buf);

    buf = nullptr;
    bufLen = 0;
    testStream = open_memstream(&buf, &bufLen);
    celix_dependencyManager_printInfoForBundle(celix_bundleContext_getDependencyManager(ctx), true, true, 0, testStream);
    fclose(testStream);
    EXPECT_TRUE(strstr(buf, "Cmp1")); //cmp name
    free(buf);

    //bundle does not exist -> empty print
    celix_dependencyManager_printInfoForBundle(celix_bundleContext_getDependencyManager(ctx), true, true, 1 /*non existing*/, stdout);

    std::stringstream ss{};
    ss << dm;
    EXPECT_TRUE(strstr(ss.str().c_str(), "Cmp1"));

    ss = std::stringstream{};
    ss << cmp;
    EXPECT_TRUE(strstr(ss.str().c_str(), "Cmp1"));
}
