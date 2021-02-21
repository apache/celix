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

#include "celix/BundleContext.h"

#include "celix_framework_factory.h"
#include "celix_framework.h"

class CxxBundleContextTestSuite : public ::testing::Test {
public:
    static constexpr const char * const TEST_BND1_LOC = "" SIMPLE_TEST_BUNDLE1_LOCATION "";
    static constexpr const char * const TEST_BND2_LOC = "" SIMPLE_TEST_BUNDLE2_LOCATION "";

    CxxBundleContextTestSuite() {
        auto* properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheCxxBundleContextTestFramework");

        auto* cfw = celix_frameworkFactory_createFramework(properties);
        fw = std::shared_ptr<celix_framework_t>{cfw, [](celix_framework_t* f){ celix_frameworkFactory_destroyFramework(f); }};
        ctx = std::make_shared<celix::BundleContext>(celix_framework_getFrameworkContext(fw.get()));
    }
    
    ~CxxBundleContextTestSuite() override {
        //ensure that ctx is destroyed before fw
        ctx = nullptr;
        fw = nullptr;
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

class TestInterface {

};

class TestImplementation : public TestInterface {

};

struct CInterface {
    void *handle;
    void (*hello)(void *handle);
};

TEST_F(CxxBundleContextTestSuite, RegisterServiceTest) {
    long svcId = ctx->findService<TestInterface>();
    EXPECT_EQ(svcId, -1L);

    {
        auto impl = std::make_shared<TestImplementation>();
        auto svcReg = ctx->registerService<TestInterface>(impl).build();
        svcReg->wait();
        svcId = ctx->findService<TestInterface>();
        EXPECT_GE(svcId, 0L);
    }

    ctx->waitForEvents();
    svcId = ctx->findService<TestInterface>();
    EXPECT_EQ(svcId, -1L);
}

TEST_F(CxxBundleContextTestSuite, RegisterCServiceTest) {
    auto svc = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg = ctx->registerService<CInterface>(svc).build();
    svcReg->wait();

    std::cout << "Name is " << celix::typeName<CInterface>() << std::endl;
    long svcId = ctx->findService<CInterface>();
    EXPECT_GE(svcId, 0L);

    svcReg->unregister();
    svcId = ctx->findService<CInterface>();
    EXPECT_GE(svcId, -1);

    CInterface svc2{nullptr, nullptr};
    auto svcReg2 = ctx->registerUnmanagedService<CInterface>(&svc2).build();

    ctx->waitForEvents();
    svcId = ctx->findService<CInterface>();
    EXPECT_GE(svcId, 0L);

    svcReg2->unregister();
    ctx->waitForEvents();
    svcId = ctx->findService<CInterface>();
    EXPECT_GE(svcId, -1);
}

TEST_F(CxxBundleContextTestSuite, UseServicesTest) {
    auto count = ctx->useService<CInterface>().build();
    EXPECT_EQ(count, 0);

    auto svc = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc).build();
    auto svcReg2 = ctx->registerService<CInterface>(svc).build();

    std::atomic<int> countFromFunction{0};
    count = ctx->useService<CInterface>().addUseCallback([&countFromFunction](CInterface&){countFromFunction += 1;}).build();
    EXPECT_EQ(count, 1);
    EXPECT_EQ(countFromFunction.load(), 1);

    countFromFunction = 0;
    count = ctx->useServices<CInterface>()
        .setTimeout(std::chrono::seconds{1})
        .addUseCallback([&countFromFunction](CInterface&, const celix::Properties& props){
            countFromFunction += 1;
            auto id = props.getAsLong(OSGI_FRAMEWORK_SERVICE_ID, -1L);
            EXPECT_GT(id, -1L);
        })
        .build();
    count += ctx->useServices<CInterface>().addUseCallback([&countFromFunction](CInterface&, const celix::Properties& props, const celix::Bundle& bnd) {
        countFromFunction += 1;
        EXPECT_GE(props.getAsLong(OSGI_FRAMEWORK_SERVICE_ID, -1L), 0);
        EXPECT_GE(bnd.getId(), -1L);
    }).build();
    EXPECT_EQ(count, 4);
    EXPECT_EQ(countFromFunction.load(), 4);
}

TEST_F(CxxBundleContextTestSuite, UseServicesWithFilterTest) {
    auto svc = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc).build();
    auto svcReg2 = ctx->registerService<CInterface>(svc).addProperty("key", "val1").build();
    auto svcReg3 = ctx->registerService<CInterface>(svc).addProperty("key", "val2").build();

    EXPECT_EQ(3, ctx->useServices<CInterface>().build());
    EXPECT_EQ(1, ctx->useServices<CInterface>().setFilter("(key=val1)").build());
    EXPECT_EQ(2, ctx->useServices<CInterface>().setFilter("(key=*)").build());
    EXPECT_EQ(3, ctx->useServices<CInterface>().setFilter(celix::Filter{}).build());

    celix::Filter f{"(key=val2)"};
    EXPECT_EQ(1, ctx->useServices<CInterface>().setFilter(f).build());

    EXPECT_THROW(ctx->useServices<CInterface>().setFilter(celix::Filter{"bla"}).build(), celix::Exception);
}


TEST_F(CxxBundleContextTestSuite, FindServicesTest) {
    auto svcId = ctx->findService<CInterface>();
    EXPECT_EQ(svcId, -1L);
    auto svcIds = ctx->findServices<CInterface>();
    EXPECT_EQ(svcIds.size(), 0);


    celix::Properties props{};
    props["key1"] = "value1";
    auto svc = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc)
            .setProperties(props)
            .addProperty("key2", "value2")
            .addProperty(celix::SERVICE_RANKING, 11)
            .build();
    svcReg1->wait();
    EXPECT_EQ(11, svcReg1->getServiceRanking());
    auto svcReg2 = ctx->registerService<CInterface>(svc).build();
    svcReg2->wait();
    EXPECT_EQ(0, svcReg2->getServiceRanking()); //no explicit svc ranking -> 0

    svcId = ctx->findService<CInterface>();
    EXPECT_EQ(svcId, svcReg1->getServiceId()); //svcReg1 -> registered first

    svcIds = ctx->findServices<CInterface>();
    EXPECT_EQ(svcIds.size(), 2);
    svcIds = ctx->findServices<CInterface>("(&(key1=value1)(key2=value2))");
    EXPECT_EQ(svcIds.size(), 1); //only 1 svc matches the filter

    svcReg1->unregister();
    svcReg1->wait();
    svcId = ctx->findService<CInterface>();
    EXPECT_EQ(svcId, svcReg2->getServiceId()); //svcReg2 -> svcReg1 unregistered
}


TEST_F(CxxBundleContextTestSuite, TrackServicesTest) {
    auto tracker = ctx->trackServices<CInterface>().build();
    tracker->wait();
    EXPECT_TRUE(tracker->isOpen());
    EXPECT_EQ(0, tracker->getServiceCount());

    celix::Properties props{};
    props["key1"] = "value1";
    auto svc = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc)
            .setProperties(props)
            .addProperty("key2", "value2")
            .build();
    svcReg1->wait();
    auto svcReg2 = ctx->registerService<CInterface>(svc).build();
    svcReg2->wait();
    EXPECT_EQ(2, tracker->getServiceCount());

    auto tracker2 = ctx->trackServices<CInterface>().setFilter("(key1=value1)").build();
    tracker2->wait();
    EXPECT_EQ(1, tracker2->getServiceCount());

    auto tracker3 = ctx->trackServices<CInterface>().setFilter(celix::Filter{"(key1=value1)"}).build();
    tracker3->wait();
    EXPECT_EQ(1, tracker3->getServiceCount());

    std::atomic<int> count{0};
    auto tracker4 = ctx->trackServices<CInterface>()
            .addAddCallback([&count](const std::shared_ptr<CInterface>&) {
                count += 1;
            })
            .addRemCallback([&count](const std::shared_ptr<CInterface>&) {
                count += 1;
            })
            .build();
    tracker4->wait();
    EXPECT_EQ(2, count); //2x add called
    svcReg1->unregister();
    svcReg1->wait();
    EXPECT_EQ(3, count); //2x add called, 1x rem called
    tracker4->close();
    tracker4->wait();
    EXPECT_EQ(4, count); //2x add called, 2x rem called (1 rem call for closing the tracker)

    EXPECT_EQ(1, tracker->getServiceCount()); //only 1 left

}


TEST_F(CxxBundleContextTestSuite, TrackServicesRanked) {
    auto tracker = ctx->trackServices<CInterface>().build();

    auto svc1 = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc1).build();

    auto svc2 = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg2 = ctx->registerService<CInterface>(svc2).build();

    auto svc3 = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg3 = ctx->registerService<CInterface>(svc3)
            .addProperty(celix::SERVICE_RANKING, 100)
            .build();
    ctx->waitForEvents();

    auto trackedServices = tracker->getServices();
    ASSERT_EQ(trackedServices.size(), 3);

    //NOTE expected order:
    // svc3 is first: highest ranking service
    // svc1 is second: same ranking as svc2, but older (lower service id)
    // svc2 is last
    EXPECT_EQ(trackedServices[0].get(), svc3.get());
    EXPECT_EQ(trackedServices[1].get(), svc1.get());
    EXPECT_EQ(trackedServices[2].get(), svc2.get());

    EXPECT_EQ(tracker->getHighestRankingService().get(), svc3.get());
}

TEST_F(CxxBundleContextTestSuite, TrackBundlesTest) {
    std::atomic<int> count{0};
    auto cb = [&count](const celix::Bundle& bnd) {
        EXPECT_GE(bnd.getId(), 0);
        count++;
    };

    auto tracker = ctx->trackBundles()
            .addOnInstallCallback(cb)
            .addOnStartCallback(cb)
            .addOnStopCallback(cb)
            .build();
    tracker->wait();
    EXPECT_EQ(celix::TrackerState::OPEN, tracker->getState());
    EXPECT_TRUE(tracker->isOpen());
    EXPECT_EQ(0, count.load());

    long bndId = ctx->installBundle("non-existing");
    EXPECT_EQ(-1, bndId); //not installed

    long bndId1 = ctx->installBundle(TEST_BND1_LOC);
    EXPECT_GE(bndId1, 0);
    EXPECT_EQ(2, count.load()); // 1x install, 1x start

    long bndId2 = ctx->installBundle(TEST_BND2_LOC, false);
    EXPECT_GE(bndId1, 0);
    EXPECT_EQ(3, count.load()); // 2x install, 1x start
    ctx->startBundle(bndId2);
    EXPECT_EQ(4, count.load()); // 2x install, 2x start

    ctx->uninstallBundle(bndId1);
    EXPECT_EQ(5, count.load()); // 2x install, 2x start, 1x stop

    ctx->stopBundle(bndId2);
    EXPECT_EQ(6, count.load()); // 2x install, 2x start, 2x stop
    ctx->startBundle(bndId2);
    EXPECT_EQ(7, count.load()); // 2x install, 3x start, 2x stop
}

TEST_F(CxxBundleContextTestSuite, OnRegisterAndUnregisterCallbacks) {
    std::atomic<int> count{};
    auto callback1 = [&count](celix::ServiceRegistration& reg) {
        EXPECT_EQ(celix::ServiceRegistrationState::REGISTERED, reg.getState());
        count++;
    };
    auto callback2 = [&count](celix::ServiceRegistration& reg) {
        EXPECT_EQ(celix::ServiceRegistrationState::UNREGISTERED, reg.getState());
        count--;
    };

    auto svcReg = ctx->registerService<TestInterface>(std::make_shared<TestImplementation>())
            .setVersion("1.0.0")
            .addOnRegistered(callback1)
            .addOnRegistered(callback1)
            .addOnRegistered([](celix::ServiceRegistration& reg) {
                std::cout << "Done registering service '" << reg.getServiceName() << "' with id " << reg.getServiceId() << std::endl;
            })
            .addOnUnregistered(callback2)
            .addOnUnregistered(callback2)
            .addOnUnregistered([](celix::ServiceRegistration& reg) {
                std::cout << "Done unregistering service '" << reg.getServiceName() << "' with id " << reg.getServiceId() << std::endl;
            })
            .build();
    svcReg->wait();
    EXPECT_EQ(count.load(), 2); //2x incr callback1
    svcReg->unregister();
    svcReg->wait();
    EXPECT_EQ(count.load(), 0); //2x descr callback2
}

TEST_F(CxxBundleContextTestSuite, InstallCxxBundle) {
    std::string loc{SIMPLE_CXX_BUNDLE_LOC};
    ASSERT_FALSE(loc.empty());
    long bndId = ctx->installBundle(loc);
    EXPECT_GE(bndId, 0);
}

TEST_F(CxxBundleContextTestSuite, LoggingUsingContext) {
    ctx->logTrace("trace");
    ctx->logDebug("debug");
    ctx->logInfo("info");
    ctx->logWarn("warn");
    ctx->logError("error");
    ctx->logFatal("fatal");
}

TEST_F(CxxBundleContextTestSuite, GetFramework) {
    EXPECT_FALSE(ctx->getFramework()->getUUID().empty());
    EXPECT_EQ(ctx->getFramework()->getFrameworkBundleContext()->getBundle().getId(), 0);
}

TEST_F(CxxBundleContextTestSuite, GetConfigurations) {
    EXPECT_EQ(ctx->getConfigProperty("non-existing", "test"), std::string{"test"});
    EXPECT_EQ(ctx->getConfigPropertyAsLong("non-existing", -1L), -1L);
    EXPECT_EQ(ctx->getConfigPropertyAsDouble("non-existing", 0), 0);
    EXPECT_EQ(ctx->getConfigPropertyAsBool("non-existing", true), true);

}

TEST_F(CxxBundleContextTestSuite, TrackAnyServices) {
    auto svc1 = std::make_shared<CInterface>(CInterface{nullptr, nullptr});
    auto svcReg1 = ctx->registerService<CInterface>(svc1).build();
    svcReg1->wait();

    auto svc2 = std::make_shared<TestImplementation>();
    auto svcReg2 = ctx->registerService<TestInterface>(svc2).build();
    svcReg2->wait();

    auto trk = ctx->trackAnyServices().build();
    trk->wait();
    EXPECT_EQ(2, trk->getServiceCount());
}

TEST_F(CxxBundleContextTestSuite, TrackServices) {
    std::atomic<int> count{0};

    auto metaTracker = ctx->trackServiceTrackers<TestInterface>()
            .addOnTrackerCreatedCallback([&count](const celix::ServiceTrackerInfo& info) {
                EXPECT_EQ(celix::typeName<TestInterface>(), info.serviceName);
                count++;
            })
            .addOnTrackerDestroyedCallback([&count](const celix::ServiceTrackerInfo& info) {
                EXPECT_EQ(celix::typeName<TestInterface>(), info.serviceName);
                count--;
            })
            .build();
    metaTracker->wait();
    EXPECT_EQ(0, count.load()); // 0x created, 0x destroyed

    auto trk1 = ctx->trackServices<TestInterface>().build();
    auto trk2 = ctx->trackServices<TestInterface>().build();
    auto trk3 = ctx->trackAnyServices().build(); //should not trigger callbacks
    auto trk4 = ctx->trackServices<CInterface>().build(); //should not trigger callbacks
    trk1->wait();
    trk2->wait();
    trk3->wait();
    trk4->wait();
    EXPECT_EQ(2, count.load()); // 2x created, 0x destroyed

    trk1->close();
    trk2->close();
    trk3->close();
    trk4->close();
    trk1->wait();
    trk2->wait();
    trk3->wait();
    trk4->wait();
    EXPECT_EQ(0, count.load()); // 2x created, 2x destroyed
}

TEST_F(CxxBundleContextTestSuite, TrackAnyServiceTracker) {
    std::atomic<int> count{0};

    auto metaTracker = ctx->trackAnyServiceTrackers()
            .addOnTrackerCreatedCallback([&count](const celix::ServiceTrackerInfo& info) {
                EXPECT_FALSE(info.filter.getFilterString().empty());
                count++;
            })
            .addOnTrackerDestroyedCallback([&count](const celix::ServiceTrackerInfo& info) {
                EXPECT_FALSE(info.filter.getFilterString().empty());
                count--;
            })
            .build();
    metaTracker->wait();
    EXPECT_EQ(0, count.load()); // 0x created, 0x destroyed

    auto trk1 = ctx->trackServices<TestInterface>().build();
    auto trk2 = ctx->trackServices<TestInterface>().build();
    auto trk3 = ctx->trackAnyServices().build();
    auto trk4 = ctx->trackServices<CInterface>().build();
    trk1->wait();
    trk2->wait();
    trk3->wait();
    trk4->wait();
    EXPECT_EQ(4, count.load()); // 4x created, 0x destroyed

    trk1->close();
    trk2->close();
    trk3->close();
    trk4->close();
    trk1->wait();
    trk2->wait();
    trk3->wait();
    trk4->wait();
    EXPECT_EQ(0, count.load()); // 4x created, 4x destroyed
}

TEST_F(CxxBundleContextTestSuite, SyncServiceRegistration) {
    long svcId = ctx->findService<TestInterface>();
    EXPECT_EQ(svcId, -1L);

    {
        auto impl = std::make_shared<TestImplementation>();
        auto svcReg = ctx->registerService<TestInterface>(impl)
                .setRegisterAsync(false) //default is true
                .setUnregisterAsync(false) //default is true
                .build();
        svcId = ctx->findService<TestInterface>();
        EXPECT_EQ(svcReg->getState(), celix::ServiceRegistrationState::REGISTERED);
        EXPECT_GE(svcId, 0L);
    }
    svcId = ctx->findService<TestInterface>();
    EXPECT_EQ(svcId, -1L);
}

TEST_F(CxxBundleContextTestSuite, UnregisterServiceWhileRegistering) {
    auto context = ctx;
    ctx->getFramework()->fireGenericEvent(
            ctx->getBundleId(),
            "register/unregister in Celix event thread",
            [context]() {
                //NOTE only testing register async in combination with unregister async/sync, because a synchronized
                //service registration will return as REGISTERED.
                auto reg1 = context->registerService<TestInterface>(std::make_shared<TestImplementation>())
                        .build(); //register & unregister async
                EXPECT_EQ(reg1->getState(), celix::ServiceRegistrationState::REGISTERING);
                reg1->unregister();
                EXPECT_EQ(reg1->getState(), celix::ServiceRegistrationState::UNREGISTERING);

                auto reg2 = context->registerService<TestInterface>(std::make_shared<TestImplementation>())
                        .setUnregisterAsync(false)
                        .build();
                EXPECT_EQ(reg2->getState(), celix::ServiceRegistrationState::REGISTERING);
                reg2->unregister(); //sync unregister -> cancel registration
                EXPECT_EQ(reg2->getState(), celix::ServiceRegistrationState::UNREGISTERED);
            }
    );
    ctx->waitForEvents();
}

TEST_F(CxxBundleContextTestSuite, KeepSharedPtrActiveWhileDeregistering) {
    auto svcReg = ctx->registerService<TestInterface>(std::make_shared<TestImplementation>())
            .build();
    auto tracker = ctx->trackServices<TestInterface>().build();
    tracker->wait();
    auto svc = tracker->getHighestRankingService();
    EXPECT_TRUE(svc.get() != nullptr);

    svcReg->unregister();
    std::this_thread::sleep_for(std::chrono::milliseconds {1500});
    EXPECT_EQ(svcReg->getState(), celix::ServiceRegistrationState::UNREGISTERING); //not still unregistering, because svc is still in use.
}

TEST_F(CxxBundleContextTestSuite, setServicesWithTrackerWhenMultipleRegistrationAlreadyExists) {
    auto reg1 = ctx->registerService<TestInterface>(std::make_shared<TestImplementation>()).build();
    auto reg2 = ctx->registerService<TestInterface>(std::make_shared<TestImplementation>()).build();
    auto reg3 = ctx->registerService<TestInterface>(std::make_shared<TestImplementation>()).build();
    ASSERT_GE(reg1->getServiceId(), 0);
    ASSERT_GE(reg2->getServiceId(), 0);
    ASSERT_GE(reg3->getServiceId(), 0);

    std::atomic<int> count{0};
    auto tracker = ctx->trackServices<TestInterface>()
            .addSetWithPropertiesCallback([&count](std::shared_ptr<TestInterface> /*svc*/, std::shared_ptr<const celix::Properties> props) {
                if (props) {
                    std::cout << "Setting svc with svc id " << props->getAsLong(celix::SERVICE_ID, -1) << std::endl;
                } else {
                    std::cout << "Unsetting svc" << std::endl;
                }
                count++;
            })
            .build();
    tracker->wait();
    EXPECT_EQ(1, count.load()); //NOTE ensure that the service tracker only calls set once for opening tracker even if 3 service exists.

    count = 0;
    tracker->close();
    tracker->wait();

    //TODO improve this. For now closing a tracker will inject other service before getting to nullptr.
    //Also look into why this is not happening in the C service tracker test.
    EXPECT_EQ(3, count.load());
}
