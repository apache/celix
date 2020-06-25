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

#include <iostream>

#include "gtest/gtest.h"

#include "celix/ServiceRegistry.h"
#include "celix/Constants.h"

class ServiceTrackingTest : public ::testing::Test {
public:
    celix::ServiceRegistry& registry() { return *reg; }
private:
    std::shared_ptr<celix::ServiceRegistry> reg{celix::ServiceRegistry::create("test")};
};

class MarkerInterface1 {
  //nop
};
class MarkerInterface2 {
    //nop
};
class MarkerInterface3 {
    //nop
};


TEST_F(ServiceTrackingTest, CreateTrackersTest) {
    EXPECT_EQ(0, registry().nrOfServiceTrackers());
    {
        celix::ServiceTrackerOptions<MarkerInterface1> opts{};
        celix::ServiceTracker trk1 = registry().trackServices(opts);
        EXPECT_EQ(1, registry().nrOfServiceTrackers());

        celix::ServiceTracker moved = std::move(trk1);
        EXPECT_EQ(1, registry().nrOfServiceTrackers());

        celix::ServiceTracker moved2 = celix::ServiceTracker{std::move(moved)};
        EXPECT_EQ(1, registry().nrOfServiceTrackers());

        auto trk2 = registry().trackServices(opts);
        auto trk3 = registry().trackServices(opts);
        EXPECT_EQ(3, registry().nrOfServiceTrackers());
    }
    EXPECT_EQ(0, registry().nrOfServiceTrackers());
}

TEST_F(ServiceTrackingTest, ServicesCountTrackersTest) {
    celix::ServiceTrackerOptions<MarkerInterface1> trkOpts1{};
    celix::ServiceTrackerOptions<MarkerInterface2> trkOpts2{};

    auto trk1 = registry().trackServices(trkOpts1);
    ASSERT_EQ(0, trk1.trackCount());

    auto svc1 = std::make_shared<MarkerInterface1>();
    auto svc2 = std::make_shared<MarkerInterface2>();
    auto svc3 = std::make_shared<MarkerInterface3>();

    auto reg1 = registry().registerService(svc1);
    auto reg2 = registry().registerService(svc2);
    reg1.wait();
    reg2.wait();

    auto trk2 = registry().trackServices(trkOpts2);
    ASSERT_EQ(1, trk1.trackCount());
    ASSERT_EQ(1, trk2.trackCount());

    {

        auto reg3 = registry().registerService(svc1);
        reg3.wait();
        ASSERT_EQ(2, trk1.trackCount());
        ASSERT_EQ(1, trk2.trackCount());

        {
            auto reg4 = registry().registerService(svc1);
            auto reg5 = registry().registerService(svc2);
            auto reg6 = registry().registerService(svc3);
            reg4.wait();
            reg5.wait();
            reg6.wait();

            ASSERT_EQ(3, trk1.trackCount());
            ASSERT_EQ(2, trk2.trackCount());

            //out of scope -> services are deregistered
        }

        ASSERT_EQ(2, trk1.trackCount());
        ASSERT_EQ(1, trk2.trackCount());
        //out of scope -> services are deregistered
    }

    ASSERT_EQ(1, trk1.trackCount());
    ASSERT_EQ(1, trk2.trackCount());
}

TEST_F(ServiceTrackingTest, SetServiceTest) {
    auto svc1 = std::make_shared<MarkerInterface1>();
    auto svc2 = std::make_shared<MarkerInterface2>();
    auto svc3 = std::make_shared<MarkerInterface3>();

    MarkerInterface1 *ptrToSvc = nullptr;
    //const celix::Properties *ptrToProps = nullptr;
    //const celix::IBundle *ptrToBnd = nullptr;

    celix::ServiceTrackerOptions<MarkerInterface1> opts{};
    opts.set = [&ptrToSvc](std::shared_ptr<MarkerInterface1> svc) {
        ptrToSvc = svc.get();
    };

    auto reg1 = registry().registerService(svc1);
    reg1.wait();
    auto reg2 = registry().registerService(svc2);
    reg2.wait();
    auto reg3 = registry().registerService(svc3);
    reg3.wait();

    auto trk1 = registry().trackServices(opts);
    EXPECT_EQ(svc1.get(), ptrToSvc); //should be intf1

    reg1.unregister();
    EXPECT_EQ(nullptr, ptrToSvc); //unregistered -> nullptr


    reg1 = registry().registerService(svc1);
    reg1.wait();
    EXPECT_EQ(svc1.get(), ptrToSvc); //set to intf1 again

    auto svc4 = std::make_shared<MarkerInterface1>();
    auto reg4 = registry().registerService(svc4);
    reg4.wait();
    EXPECT_EQ(svc1.get(), ptrToSvc); //no change -> intf1 is older

    reg1.unregister();
    EXPECT_EQ(svc4.get(), ptrToSvc); //intf1 is gone, intf4 should be set

    trk1.stop();
    EXPECT_EQ(nullptr, ptrToSvc); //stop tracking nullptr svc is set

    {
        auto trk2 = registry().trackServices(opts);
        EXPECT_EQ(svc4.get(), ptrToSvc); //intf1 is gone, intf4 should be set
        //out of scope -> tracker stopped
    }
    EXPECT_EQ(nullptr, ptrToSvc); //stop tracking nullptr svc is set
}

TEST_F(ServiceTrackingTest, SetServiceWithPropsAndOwnderTest) {
    /*
    MarkerInterface1 intf1;
    MarkerInterface2 intf2;
    MarkerInterface3 intf3;

    MarkerInterface1 *ptrToSvc = nullptr;
    const celix::Properties *ptrToProps = nullptr;
    const celix::IBundle *ptrToBnd = nullptr;
    */
    //TODO
}

TEST_F(ServiceTrackingTest, AddRemoveTest) {
    auto svc1 = std::make_shared<MarkerInterface1>();
    auto svc2 = std::make_shared<MarkerInterface2>();
    auto svc3 = std::make_shared<MarkerInterface3>();

    std::vector<std::shared_ptr<MarkerInterface1>> services{};

    celix::ServiceTrackerOptions<MarkerInterface1> opts{};
    opts.add = [&services](std::shared_ptr<MarkerInterface1> svc) {
        services.push_back(svc);
    };
    opts.remove = [&services](std::shared_ptr<MarkerInterface1> svc) {
        services.erase(std::remove(services.begin(), services.end(), svc), services.end());
    };

    auto reg1 = registry().registerService(svc1);
    reg1.wait();
    auto reg2 = registry().registerService(svc2);
    reg2.wait();
    auto reg3 = registry().registerService(svc3);
    reg3.wait();

    auto trk1 = registry().trackServices(opts);
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc1.get(), services[0].get()); //should be intf1

    reg1.unregister();
    EXPECT_EQ(0, services.size());

    reg1 = registry().registerService(svc1);
    reg1.wait();
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc1.get(), services[0].get()); //should be intf1 again

    auto svc4 = std::make_shared<MarkerInterface1>();
    auto reg4 = registry().registerService(svc4);
    reg4.wait();
    ASSERT_EQ(2, services.size());
    EXPECT_EQ(svc1.get(), services[0].get());
    EXPECT_EQ(svc4.get(), services[1].get());

    reg1.unregister();
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc4.get(), services[0].get()); //intf1 gone -> index 0: intf4

    trk1.stop();
    EXPECT_EQ(0, services.size());

    {
        auto trk2 = registry().trackServices(opts);
        ASSERT_EQ(1, services.size());
        EXPECT_EQ(svc4.get(), services[0].get());
        //out of scope -> tracker stopped
    }
    EXPECT_EQ(0, services.size()); //stop tracking -> services removed
}

TEST_F(ServiceTrackingTest, AddRemoveServicesWithPropsAndOwnderTest) {
    //TODO
}

TEST_F(ServiceTrackingTest, UpdateTest) {
    auto svc1 = std::make_shared<MarkerInterface1>();
    auto svc2 = std::make_shared<MarkerInterface2>();
    auto svc3 = std::make_shared<MarkerInterface3>();


    std::vector<std::shared_ptr<MarkerInterface1>> services{};

    celix::ServiceTrackerOptions<MarkerInterface1> opts{};
    opts.update = [&services](std::vector<std::shared_ptr<MarkerInterface1>> rankedServices) {
        services = rankedServices;
    };

    auto reg1 = registry().registerService(svc1);
    reg1.wait();
    auto reg2 = registry().registerService(svc2);
    reg2.wait();
    auto reg3 = registry().registerService(svc3);
    reg3.wait();

    auto trk1 = registry().trackServices(opts);
    EXPECT_EQ(1, trk1.trackCount());
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc1.get(), services[0].get()); //should be intf1

    reg1.unregister();
    EXPECT_EQ(0, services.size());

    reg1 = registry().registerService(svc1);
    reg1.wait();
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc1.get(), services[0].get()); //should be intf1 again

    auto svc4 = std::make_shared<MarkerInterface1>();
    celix::Properties properties{};
    properties[celix::SERVICE_RANKING] = "100";
    auto reg4 = registry().registerService(svc4, properties);
    reg4.wait();
    ASSERT_EQ(2, services.size());
    EXPECT_EQ(svc4.get(), services[0].get()); //note 4 higher ranking
    EXPECT_EQ(svc1.get(), services[1].get());

    reg1.unregister();
    ASSERT_EQ(1, services.size());
    EXPECT_EQ(svc4.get(), services[0].get()); //intf1 gone -> index 0: intf4

    trk1.stop();
    EXPECT_EQ(0, services.size());

    {
        auto trk2 = registry().trackServices(opts);
        ASSERT_EQ(1, services.size());
        EXPECT_EQ(svc4.get(), services[0].get());
        //out of scope -> tracker stopped
    }
    EXPECT_EQ(0, services.size()); //stop tracking -> services removed
}

TEST_F(ServiceTrackingTest, UpdateTestWithPropsAndOwner) {
    //TODO
}

//TODO function service tracking tests
