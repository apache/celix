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

#include "celix/ServiceRegistrationBuilder.h"

class ServiceRegistrationBuilderTest : public ::testing::Test {
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


TEST_F(ServiceRegistrationBuilderTest, ServiceRegistrationBuilderTest) {
    auto builder = celix::ServiceRegistrationBuilder<Interface1>{bundle(), registry()};
    //NOTE by design not possible, to prevent wrong use:
    // auto copy = build;
    const auto templ = builder.copy(); //this is possible

    {
        auto reg = builder.setService(std::make_shared<Interface1>()).build();
        auto services = registry()->findServices<Interface1>();
        EXPECT_EQ(services.size(), 1);
    } //RAII -> deregister service
    auto services = registry()->findServices<Interface1>();
    EXPECT_EQ(services.size(), 0);

    services = registry()->findServices<Interface1>("(name=val)");
    EXPECT_EQ(services.size(), 0);
    {
        celix::Properties props0{};
        props0["name0"] = "value0";
        celix::Properties props1{};
        props1["name2"] = "value2";
        celix::Properties props2{};
        props2["name3"] = "value3";
        auto reg = templ.copy()
                .setService(new Interface1{})
                .setProperties(props0)
                .setProperties(std::move(props1)) //overrides props0
                .addProperty("name1", "value1")
                .addProperties(props2)
                .build();
        services = registry()->findServices<Interface1>("(name1=value1)");
        EXPECT_EQ(services.size(), 1);

        celix::UseServiceOptions<Interface1> useOpts{};
        useOpts.useWithProperties = [](const std::shared_ptr<Interface1>& /*svc*/, const celix::Properties &props) {
            EXPECT_EQ(props.find("name0"), props.end());
            EXPECT_EQ(props.at("name1"), "value1");
            EXPECT_EQ(props.at("name2"), "value2");
            EXPECT_EQ(props.at("name3"), "value3");
        };

        auto called = registry()->useService(useOpts);
        EXPECT_TRUE(called);
    }
}


TEST_F(ServiceRegistrationBuilderTest, FunctionServiceRegistrationBuilderTest) {

    auto fn = std::string{"test function"};
    celix::FunctionServiceRegistrationBuilder<void()> builder{bundle(), registry(), fn};
    //NOTE by design not possible, to prevent wrong use:
    // auto copy = build;
    const auto templ = builder.copy(); //this is possible

    {
        auto reg = builder.setFunctionService([]{/*nop*/}).build();
        auto functions = registry()->findFunctionServices<void()>(fn);
        EXPECT_EQ(functions.size(), 1);
    } //RAII -> deregister service
    auto services = registry()->findFunctionServices<void()>(fn);
    EXPECT_EQ(services.size(), 0);

    services = registry()->findServices<Interface1>("(name=val)");
    EXPECT_EQ(services.size(), 0);
    {
        celix::Properties props0{};
        props0["name0"] = "value0";
        celix::Properties props1{};
        props1["name2"] = "value2";
        celix::Properties props2{};
        props2["name3"] = "value3";
        auto reg = templ.copy()
                .setFunctionService([]{/*nop two*/})
                .setProperties(props0)
                .setProperties(std::move(props1)) //overrides props0
                .addProperty("name1", "value1")
                .addProperties(props2)
                .build();
        services = registry()->findFunctionServices<void()>(fn, "(name1=value1)");
        EXPECT_EQ(services.size(), 1);

        celix::UseFunctionServiceOptions<void()> useOpts{fn};
        useOpts.useWithProperties = [](const std::function<void()>& /*f*/, const celix::Properties &props) {
            EXPECT_EQ(props.find("name0"), props.end());
            EXPECT_EQ(props.at("name1"), "value1");
            EXPECT_EQ(props.at("name2"), "value2");
            EXPECT_EQ(props.at("name3"), "value3");
        };

        auto called = registry()->useFunctionService<void()>(useOpts);
        EXPECT_TRUE(called);
    }
}