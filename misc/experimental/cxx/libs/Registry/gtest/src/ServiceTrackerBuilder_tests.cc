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

#include "celix/ServiceTrackerBuilder.h"

class ServiceTrackerBuilderTest : public ::testing::Test {
public:
    std::shared_ptr<celix::ServiceRegistry> registry() const { return reg; }
    std::shared_ptr<celix::IResourceBundle> bundle() const { return bnd; }
private:
    std::shared_ptr<celix::ServiceRegistry> reg{new celix::ServiceRegistry{"test"}};
    std::shared_ptr<celix::IResourceBundle> bnd{new celix::EmptyResourceBundle{}};
};


class Interface1 {
  //nop
};
class Interface2 {
    //nop
};
class Interface3 {
    //nop
};


TEST_F(ServiceTrackerBuilderTest, ServiceRegistrationBuilderTest) {
    auto builder = celix::ServiceTrackerBuilder<Interface1>{bundle(), registry()};
    //NOTE by design not possible, to prevent wrong use:
    // auto copy = build;
    const auto templ = builder.copy(); //this is possible

    {
        EXPECT_EQ(0, registry()->nrOfServiceTrackers());
        auto tracker = builder.build();
        EXPECT_EQ(1, registry()->nrOfServiceTrackers());
        EXPECT_TRUE(tracker.valid());
    } //RAII -> stop tracker
    EXPECT_EQ(0, registry()->nrOfServiceTrackers());

    auto services = registry()->findServices<Interface1>("(name=val)");
    EXPECT_EQ(services.size(), 0);

    {
        size_t count = 0;

        celix::Properties props1{};
        props1["name"] = "value1";
        auto reg1 = registry()->registerService(std::make_shared<Interface1>(), props1);
        celix::Properties props2{};
        props2["name"] = "value2";
        auto reg2 = registry()->registerService(std::make_shared<Interface1>(), props2);
        celix::Properties props3{};
        props3["name"] = "value3";
        auto reg3 = registry()->registerService(std::make_shared<Interface1>(), props3);

        EXPECT_EQ(0, count);
        auto tracker = builder
                .setFilter("(|(name=value1)(name=value2))")
                .setCallback([&count](const std::shared_ptr<Interface1> &){++count;})
                .build();
        EXPECT_EQ(2, tracker.trackCount());
        EXPECT_EQ(1, count);
    }

    //TODO test set with properties/owner
    //TODO test add/rem
    //TODO test update
}
