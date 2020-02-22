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

#include <functional>

#include "gtest/gtest.h"

#include "celix/Utils.h"

class UtilsTest : public ::testing::Test {
public:
    UtilsTest() {}
    ~UtilsTest(){}
};

class MarkerInterface{};

namespace example {
    class MarkerInterface{};
}

class SvcWithFqn {
public:
    static constexpr const char * const NAME = "[SvcWithFqn] [version 1]";
};

class SvcWithSpecializedName {
    //note no NAME
};

namespace celix {
    template<>
    constexpr inline const char* customServiceNameFor<SvcWithSpecializedName>() { return "SPECIALIZED"; }
}

TEST_F(UtilsTest, svcName) {
    std::string name = celix::serviceName<MarkerInterface>();
    EXPECT_EQ("MarkerInterface", name);

    name = celix::serviceName<example::MarkerInterface>();
    EXPECT_EQ("example::MarkerInterface", name);

    name = celix::serviceName<SvcWithFqn>();
    EXPECT_EQ("[SvcWithFqn] [version 1]", name);

    name = celix::serviceName<SvcWithSpecializedName>();
    EXPECT_EQ("SPECIALIZED", name);

    name = celix::functionServiceName<std::function<void()>>("do");
    EXPECT_EQ("[do] [std::function<void()>]", name);

    name = celix::functionServiceName<std::function<double()>>("do");
    EXPECT_EQ("[do] [std::function<double()>]", name);

    //TODO FIXME
//    name = celix::functionServiceName<void(double)>("do");
//    EXPECT_EQ("do [std::function<void(double)>]", name);

    //TODO FIXME
//    name = celix::functionServiceName<std::vector<std::vector<long>>(long, int, std::vector<double>)>("collect");
//    EXPECT_EQ("collect [std::function<std::vector<std::vector<long>>(long, int, std::vector<double>)>]", name);
}