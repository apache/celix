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


#include "celix/UseServiceBuilder.h"

class UseServiceBuilderTest : public ::testing::Test {
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


TEST_F(UseServiceBuilderTest, UseServiceBuilderTest) {
    auto builder = celix::UseServiceBuilder<Interface1>{bundle(), registry()};
    //NOTE by design not possible, to prevent wrong use:
    // auto copy = build;
    auto templ = builder.copy(); //this is possible

    {
        auto called = builder.use();
        EXPECT_FALSE(called);

        auto reg = registry()->registerService(std::make_shared<Interface1>());

        called = builder.use();
        EXPECT_TRUE(called);

        size_t count = 0;
        templ.setCallback([&count](Interface1 &) { ++count; });
        called = templ.use();
        EXPECT_TRUE(called);
        EXPECT_EQ(count, 1);

        int callCount = templ.useAll();
        EXPECT_EQ(callCount, 1);
        EXPECT_EQ(count, 2);
    }

    auto called = builder.use();
    EXPECT_FALSE(called);
}

TEST_F(UseServiceBuilderTest, UseFunctionServiceBuilderTest) {
    auto builder = celix::UseFunctionServiceBuilder<std::function<void()>>{bundle(), registry(), "test"};
    //NOTE by design not possible, to prevent wrong use:
    // auto copy = build;
    auto templ = builder.copy(); //this is possible

    {
        auto called = builder.use();
        EXPECT_FALSE(called);

        size_t count = 0;
        auto reg = registry()->registerFunctionService<std::function<void()>>("test", [&count]{++count;});

        called = builder.use();
        EXPECT_TRUE(called);

        templ.setCallback([](const std::function<void()> &f) { f(); });
        called = templ.use();
        EXPECT_TRUE(called);
        EXPECT_EQ(count, 1);

        int callCount = templ.useAll();
        EXPECT_EQ(callCount, 1);
        EXPECT_EQ(count, 2);
    }

    auto called = builder.use();
    EXPECT_FALSE(called);
}