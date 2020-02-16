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

#include <atomic>

#include "gtest/gtest.h"

#include "celix/ComponentManager.h"

class ComponentManagerTest : public ::testing::Test {
public:
    std::shared_ptr<celix::ServiceRegistry> registry() { return reg; }
    std::shared_ptr<celix::IResourceBundle> bundle() { return bnd; }
private:
    std::shared_ptr<celix::ServiceRegistry> reg{new celix::ServiceRegistry{"test"}};
    std::shared_ptr<celix::IResourceBundle> bnd{new celix::EmptyResourceBundle{}};
};


class Cmp {};

class ISvc {};


TEST_F(ComponentManagerTest, CreateDestroy) {
    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    EXPECT_FALSE(cmpMng.isEnabled());
    EXPECT_FALSE(cmpMng.isResolved()); //disabled -> not resolved
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::Disabled);

    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());
    EXPECT_TRUE(cmpMng.isResolved()); //no deps -> resolved
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    cmpMng.disable();
    EXPECT_FALSE(cmpMng.isEnabled());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::Disabled);
}

TEST_F(ComponentManagerTest, AddSvcDep) {
    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>()
            .setRequired(true);
    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentUninitialized);


    //dep not available -> cmp manager not resolved
    EXPECT_FALSE(cmpMng.isResolved());


    auto svcReg = registry()->registerService(std::make_shared<ISvc>());
    //dep available -> cmp manager resolved
    EXPECT_TRUE(cmpMng.isResolved());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    cmpMng.disable();
    //cmp disabled -> not resolved
    EXPECT_FALSE(cmpMng.isResolved());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::Disabled);

    //dmp enabled & svc available -> resolved.
    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isResolved());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    svcReg.unregister();
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentInitialized);
    //dep unregisted -> state is Initialized
}

TEST_F(ComponentManagerTest, FindServiceDepenencies) {
    std::string svcDepUuid;

    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>()
            .extractUUID(svcDepUuid);

    EXPECT_TRUE(cmpMng.findServiceDependency<ISvc>(svcDepUuid).isValid());
    EXPECT_FALSE(cmpMng.findServiceDependency<Cmp /*in correct template*/>(svcDepUuid).isValid());
    EXPECT_FALSE(cmpMng.findServiceDependency<ISvc>("non existing uuid").isValid());
}

TEST_F(ComponentManagerTest, AddAndRemoveSvcDep) {
    std::string svcDepUuid;

    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>()
            .setRequired(true)
            .extractUUID(svcDepUuid);
    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());

    //dep not available -> cmp manager not resolved
    EXPECT_FALSE(cmpMng.isResolved());

    cmpMng.removeServiceDependency(svcDepUuid);
    //dep removed -> cmp manager resolved
    EXPECT_TRUE(cmpMng.isResolved());
}


TEST_F(ComponentManagerTest, AddAndEnableSvcDep) {

    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>()
            .setRequired(true);
    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());
    EXPECT_EQ(cmpMng.nrOfServiceDependencies(), 1);

    //dep not available -> cmp manager not resolved
    EXPECT_FALSE(cmpMng.isResolved());

    auto svcReg = registry()->registerService(std::make_shared<ISvc>());
    //svc add -> cmp resolved
    EXPECT_TRUE(cmpMng.isResolved());

    std::string svcUUID{};
    cmpMng.addServiceDependency<ISvc>()
            .setFilter("(non_existing=*)")
            .extractUUID(svcUUID);
    EXPECT_EQ(cmpMng.nrOfServiceDependencies(), 2);
    //new dep is still disabled -> cmp is resolved
    EXPECT_TRUE(cmpMng.isResolved());

    cmpMng.findServiceDependency<ISvc>(svcUUID).enable();
    //new required svc dep -> cmp not resolved
    EXPECT_FALSE(cmpMng.isResolved());
    //cmp already started should now be initialized
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentInitialized);
    EXPECT_EQ(cmpMng.nrOfServiceDependencies(), 2);

    cmpMng.removeServiceDependency(svcUUID);
    //required svc dep is gone -> resolved
    EXPECT_TRUE(cmpMng.isResolved());
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);
    EXPECT_EQ(cmpMng.nrOfServiceDependencies(), 1);
}

TEST_F(ComponentManagerTest, CmpLifecycleCallbacks) {
    class TestCmp {
    public:
        int initCount = 0;
        int startCount = 0;
        int stopCount = 0;
        int deinitCount = 0;

        void init() {++initCount;}
        void start() {++startCount;}
        void stop() {++stopCount;}
        void deinit() {++deinitCount;}
    };

    celix::ComponentManager<TestCmp> cmpMng{bundle(), registry(), std::make_shared<TestCmp>()};
    cmpMng.setCallbacks(&TestCmp::init, &TestCmp::start, &TestCmp::stop, &TestCmp::deinit);
    auto cmp = cmpMng.getCmpInstance();
    EXPECT_EQ(0, cmp->initCount);
    EXPECT_EQ(0, cmp->startCount);
    EXPECT_EQ(0, cmp->stopCount);
    EXPECT_EQ(0, cmp->deinitCount);

    cmpMng.enable(); // started
    EXPECT_EQ(1, cmp->initCount);
    EXPECT_EQ(1, cmp->startCount);
    EXPECT_EQ(0, cmp->stopCount);
    EXPECT_EQ(0, cmp->deinitCount);

    cmpMng.disable(); // disbabling -> stop -> deinit
    EXPECT_EQ(1, cmp->initCount);
    EXPECT_EQ(1, cmp->startCount);
    EXPECT_EQ(1, cmp->stopCount);
    EXPECT_EQ(1, cmp->deinitCount);
}

TEST_F(ComponentManagerTest, SuspenseAndLockingStrategy) {
    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>().setFilter("(id=locking)").setRequired(false).setStrategy(celix::UpdateServiceStrategy::Locking);
    cmpMng.addServiceDependency<ISvc>().setFilter("(id=suspense)").setRequired(false);
    cmpMng.enable(); //no required dep -> cmp started.
    
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    celix::Properties props{};
    props["id"] = "locking";
    auto reg1 = registry()->registerService(std::make_shared<ISvc>(), props); //svc registered -> cmp updated, using locking strategy so no suspense triggered.
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);
    EXPECT_EQ(cmpMng.getSuspendedCount(), 0); //no suspense needed;


    props["id"] = "suspense";
    auto reg2 = registry()->registerService(std::make_shared<ISvc>(), props); //svc registered -> cmp updated, using suspense strategy so 1 suspense triggered.
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);
    EXPECT_EQ(cmpMng.getSuspendedCount(), 1); //2x suspense needed (for both services);

    reg2.unregister();
    EXPECT_EQ(cmpMng.getSuspendedCount(), 2);
}

TEST_F(ComponentManagerTest, RequiredAndOptionalDependencies) {
    celix::ComponentManager<Cmp> cmpMng{bundle(), registry(), std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>().setFilter("(id=required)").setRequired(true);
    cmpMng.addServiceDependency<ISvc>().setFilter("(id=optional)").setRequired(false);
    cmpMng.enable(); //required dep -> cmp uninitialized.

    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentUninitialized);

    celix::Properties props{};
    props["id"] = "optional";
    auto reg1 = registry()->registerService(std::make_shared<ISvc>(), props);
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentUninitialized);


    props["id"] = "required";
    auto reg2 = registry()->registerService(std::make_shared<ISvc>(), props);
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    cmpMng.addServiceDependency<ISvc>().setFilter("(id=optional2)").setRequired(false).enable();
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);

    cmpMng.addServiceDependency<ISvc>().setFilter("(id=required2)").setRequired(true).enable();
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentInitialized);

    props["id"] = "required2";
    auto reg3 = registry()->registerService(std::make_shared<ISvc>(), props); //missing required cmp from started to initialized.
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);
}

TEST_F(ComponentManagerTest, AddProvidedServices) {
    class TestCmp : public ISvc {};

    celix::ComponentManager<TestCmp> cmpMng{bundle(), registry(), std::make_shared<TestCmp>()};
    cmpMng.addProvidedService<ISvc>().addProperty("nr", "1");
    cmpMng.enable();
    EXPECT_EQ(cmpMng.getState(), celix::ComponentManagerState::ComponentStarted);
    EXPECT_EQ(cmpMng.nrOfProvidedServices(), 1);

    //one service should be registered
    EXPECT_EQ(registry()->findServices<ISvc>().size(), 1);
    EXPECT_EQ(registry()->findServices<ISvc>("(nr=1)").size(), 1);

    std::string provideUUID;
    cmpMng.addProvidedService<ISvc>().addProperty("nr", "2").extractUUID(provideUUID);
    //still only one -> second provide not jet enabled
    EXPECT_EQ(registry()->findServices<ISvc>().size(), 1);
    EXPECT_EQ(cmpMng.nrOfProvidedServices(), 2);

    cmpMng.findProvidedService<ISvc>(provideUUID).enable();
    //enabled -> should be 2 now
    EXPECT_EQ(registry()->findServices<ISvc>().size(), 2);
    cmpMng.findProvidedService<ISvc>(provideUUID).disable();
    //disabled -> should be 1 now
    EXPECT_EQ(registry()->findServices<ISvc>().size(), 1);
    cmpMng.findProvidedService<ISvc>(provideUUID).enable();
    cmpMng.removeProvideService(provideUUID);
    EXPECT_EQ(cmpMng.nrOfProvidedServices(), 1);
}

TEST_F(ComponentManagerTest, FindProvidedServices) {
    class TestCmp : public ISvc {};

    celix::ComponentManager<TestCmp> cmpMng{bundle(), registry(), std::make_shared<TestCmp>()};

    std::string uuid;
    cmpMng.addProvidedService<ISvc>().extractUUID(uuid);

    EXPECT_TRUE(cmpMng.findProvidedService<ISvc>(uuid).isValid());
    EXPECT_FALSE(cmpMng.findProvidedService<ISvc>("bla").isValid());
}

//TODO test callbacks svc dep (set, add, rem, update)
