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



#include <memory>
#include <climits>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "celix/ServiceRegistry.h"

class RegistryTest : public ::testing::Test {
public:
    RegistryTest() {}
    ~RegistryTest(){}

    celix::ServiceRegistry& registry() { return reg; }
private:
    celix::ServiceRegistry reg{"C++"};
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


TEST_F(RegistryTest, ServiceRegistrationTest) {
    //empty registry
    EXPECT_EQ(0, registry().nrOfRegisteredServices());

    {
        celix::Properties props{};
        props["TEST"] = "VAL";
        celix::ServiceRegistration reg = registry().registerService(std::make_shared<MarkerInterface1>(), props);
        //no copy possible celix::ServiceRegistration copy = reg;

        long svcId = reg.serviceId();
        EXPECT_GE(svcId, -1L);
        EXPECT_TRUE(reg.valid());
        EXPECT_EQ(reg.properties().at(celix::SERVICE_NAME), celix::serviceName<MarkerInterface1>());
        EXPECT_EQ("VAL", celix::getProperty(reg.properties(), "TEST", ""));

        EXPECT_EQ(1, registry().nrOfRegisteredServices());
        EXPECT_EQ(1, registry().findServices<MarkerInterface1>().size());



        celix::ServiceRegistration moved = std::move(reg); //moving is valid
        EXPECT_EQ(svcId, moved.serviceId());
        EXPECT_EQ(1, registry().nrOfRegisteredServices());

        celix::ServiceRegistration moved2{std::move(moved)};
        EXPECT_EQ(1, registry().nrOfRegisteredServices());

        //out of scope -> deregister
    }
    EXPECT_EQ(0, registry().nrOfRegisteredServices());
    EXPECT_EQ(0, registry().findServices<MarkerInterface1>().size());

    MarkerInterface1 intf1{};
    MarkerInterface1 intf2{};
    MarkerInterface2 intf3{};
    {
        celix::ServiceRegistration reg1 = registry().registerService(intf1);
        EXPECT_EQ(1, registry().nrOfRegisteredServices());
        celix::ServiceRegistration reg2 = registry().registerService(intf2);
        EXPECT_EQ(2, registry().nrOfRegisteredServices());
        celix::ServiceRegistration reg3 = registry().registerService(intf3);
        EXPECT_EQ(3, registry().nrOfRegisteredServices());

        EXPECT_GT(reg1.serviceId(), 0);
        EXPECT_GT(reg2.serviceId(), 0);
        EXPECT_GT(reg3.serviceId(), 0);
        EXPECT_TRUE(reg1.valid());
        EXPECT_TRUE(reg2.valid());
        EXPECT_TRUE(reg3.valid());

        EXPECT_EQ(3, registry().nrOfRegisteredServices());
        EXPECT_EQ(2, registry().findServices<MarkerInterface1>().size());
        EXPECT_EQ(1, registry().findServices<MarkerInterface2>().size());
        EXPECT_EQ(0, registry().findServices<MarkerInterface3>().size());

        //out of scope -> deregister
    }
    EXPECT_EQ(0, registry().nrOfRegisteredServices());
    EXPECT_EQ(0, registry().findServices<MarkerInterface1>().size());
    EXPECT_EQ(0, registry().findServices<MarkerInterface2>().size());
    EXPECT_EQ(0, registry().findServices<MarkerInterface3>().size());
}

TEST_F(RegistryTest, SimpleFindServicesTest) {
    MarkerInterface1 intf1{};
    MarkerInterface2 intf2{};

    auto reg1 = registry().registerService(intf1);
    auto reg2 = registry().registerService(intf2);

    long firstSvc = registry().findService<MarkerInterface1>();
    EXPECT_GT(firstSvc, 0);
    std::vector<long> services = registry().findServices<MarkerInterface1>();
    EXPECT_EQ(1, services.size());

    auto reg3 = registry().registerService(intf1);
    auto reg4 = registry().registerService(intf2);

    long foundSvc = registry().findService<MarkerInterface1>(); //should find the first services
    EXPECT_GT(foundSvc, 0);
    EXPECT_EQ(firstSvc, foundSvc);
    services = registry().findServices<MarkerInterface1>();
    EXPECT_EQ(2, services.size());
}

TEST_F(RegistryTest, FindServicesTest) {
    MarkerInterface1 intf1{};

    celix::Properties props1{};
    props1["loc"] = "front";
    props1["answer"] = "42";

    celix::Properties props2{};
    props2["loc"] = "back";

    auto reg1 = registry().registerService(intf1, props1);
    auto reg2 = registry().registerService(intf1, std::move(props2));
    auto reg3 = registry().registerService(intf1);

    auto find1 = registry().findServices<MarkerInterface1>("(loc=*)"); // expecting 2
    auto find2 = registry().findServices<MarkerInterface1>("(answer=42)"); // expecting 1
    auto find3 = registry().findServices<MarkerInterface1>("(!(loc=*))"); // expecting 1

    EXPECT_EQ(2, find1.size());
    EXPECT_EQ(1, find2.size());
    EXPECT_EQ(1, find3.size());
}

TEST_F(RegistryTest, UseServices) {
    MarkerInterface1 intf1{};
    auto reg1 = registry().registerService(intf1);
    auto reg2 = registry().registerService(intf1);
    auto reg3 = registry().registerService(intf1);

    long svcId1 = reg1.serviceId();

    bool called = registry().useService<MarkerInterface1>([&](MarkerInterface1& /*svc*/) -> void {
        //nop
    });
    EXPECT_TRUE(called);

    called = registry().useService<MarkerInterface1>([&](MarkerInterface1& /*svc*/, const celix::Properties &props) -> void {
        long id = celix::getPropertyAsLong(props, celix::SERVICE_ID, 0);
        EXPECT_EQ(svcId1, id);
    });
    EXPECT_TRUE(called);

    called = registry().useService<MarkerInterface1>([&](MarkerInterface1& /*svc*/, const celix::Properties &props, const celix::IResourceBundle &bnd) -> void {
        long id = celix::getPropertyAsLong(props, celix::SERVICE_ID, 0L);
        EXPECT_EQ(svcId1, id);
        EXPECT_EQ(LONG_MAX, bnd.id()); //not nullptr -> use empty bundle (bndId 0)
    });
    EXPECT_TRUE(called);
}

TEST_F(RegistryTest, RankingTest) {
    MarkerInterface1 intf1{};
    auto reg1 = registry().registerService(intf1); //no props -> ranking 0
    auto reg2 = registry().registerService(intf1); //no props -> ranking 0

    {
        auto reg3 = registry().registerService(intf1); //no props -> ranking 0
        auto find = registry().findServices<MarkerInterface1>(); // expecting 3
        EXPECT_EQ(3, find.size());
        EXPECT_EQ(reg1.serviceId(), find[0]); //first registered service should be found first
        EXPECT_EQ(reg2.serviceId(), find[1]); //first registered service should be found first
        EXPECT_EQ(reg3.serviceId(), find[2]); //first registered service should be found first

        //note reg 3 out of scope
    }

    celix::Properties props{};
    props[celix::SERVICE_RANKING] = "100";
    auto reg4 = registry().registerService(intf1, props); // ranking 100 -> before services with ranking 0

    {
        auto reg5 = registry().registerService(intf1, props); // ranking 100 -> before services with ranking 0
        auto find = registry().findServices<MarkerInterface1>();
        EXPECT_EQ(4, find.size());
        EXPECT_EQ(reg4.serviceId(), find[0]); //ranking 100, oldest (i.e. highest ranking with lowest svcId)
        EXPECT_EQ(reg5.serviceId(), find[1]); //ranking 100, newest
        EXPECT_EQ(reg1.serviceId(), find[2]); //ranking 0, oldest
        EXPECT_EQ(reg2.serviceId(), find[3]); //ranking 0, newest

        //note reg 5 out of scope
    }

    props[celix::SERVICE_RANKING] = "-100";
    auto reg6 = registry().registerService(intf1, props); // ranking -100 -> after the rest
    auto reg7 = registry().registerService(intf1, props); // ranking -100 -> after the rest
    props[celix::SERVICE_RANKING] = "110";
    auto reg8 = registry().registerService(intf1, props); // ranking 110 -> before the rest
    props[celix::SERVICE_RANKING] = "80";
    auto reg9 = registry().registerService(intf1, props); // ranking 80 -> between ranking 0 adn 100
    {
        auto find = registry().findServices<MarkerInterface1>();
        EXPECT_EQ(7, find.size());
        EXPECT_EQ(reg8.serviceId(), find[0]); //ranking 110
        EXPECT_EQ(reg4.serviceId(), find[1]); //ranking 100
        EXPECT_EQ(reg9.serviceId(), find[2]); //ranking 80
        EXPECT_EQ(reg1.serviceId(), find[3]); //ranking 0, oldest
        EXPECT_EQ(reg2.serviceId(), find[4]); //ranking 0, newest
        EXPECT_EQ(reg6.serviceId(), find[5]); //ranking -100, oldest
        EXPECT_EQ(reg7.serviceId(), find[6]); //ranking -100, newest
    }
}

TEST_F(RegistryTest, StdFunctionTest) {
    int count = 0;
    std::function<void()> func1 = [&count]{
        count++;
    };

    auto reg1 = registry().registerFunctionService("count", func1);
    LOG(INFO) << reg1.serviceName() << std::endl;
    EXPECT_TRUE(reg1.valid());
    EXPECT_EQ(1, registry().nrOfRegisteredServices());

    std::function<int(long,double,const std::string&)> funcWithReturnAndArgs = [](long a, double b, const std::string &ref) -> int {
        return (int)(a/b) + (int)ref.size();
    };
    auto reg2 = registry().registerFunctionService("yet another function", std::move(funcWithReturnAndArgs));
    EXPECT_TRUE(reg2.valid());
    EXPECT_EQ(2, registry().nrOfRegisteredServices());
    LOG(INFO) << reg2.serviceName() << std::endl;

    std::function<void(std::function<void()>&)> use = [](std::function<void()> &count) {
        count();
    };
    EXPECT_EQ(0, count);
    registry().useFunctionService("count", use);
    EXPECT_EQ(1, count);
    registry().useFunctionService<std::function<void()>>("count", [](std::function<void()> &count) {
        count();
    });
}

TEST_F(RegistryTest, ListServicesTest) {
    std::vector<std::string> serviceNames = registry().listAllRegisteredServiceNames();
    EXPECT_EQ(0, serviceNames.size());

    std::function<void()> nop = []{/*nop*/};
    class MarkerInterface1 {};
    MarkerInterface1 intf1;

    {
        auto reg1 = registry().registerFunctionService("nop", nop);
        serviceNames = registry().listAllRegisteredServiceNames();
        EXPECT_EQ(1, serviceNames.size());

        auto reg2 = registry().registerService(intf1);
        serviceNames = registry().listAllRegisteredServiceNames();
        EXPECT_EQ(2, serviceNames.size());
    }
    serviceNames = registry().listAllRegisteredServiceNames();
    EXPECT_EQ(0, serviceNames.size());
}

//TODO function use with props and bnd
//TODO use with filter
//TODO use with sync test (see BundleContext tests)