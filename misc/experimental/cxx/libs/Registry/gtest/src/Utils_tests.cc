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

class MarkerInterface;

namespace example {
    class MarkerInterface;
}

class SvcWithFqn {
    static constexpr const char * const FQN = "SvcWithFqn[Version 1]";
};

TEST_F(UtilsTest, svcName) {
    std::string name = celix::serviceName<MarkerInterface>();
    EXPECT_EQ("MarkerInterface", name);

    name = celix::serviceName<example::MarkerInterface>();
    EXPECT_EQ("example::MarkerInterface", name);

    name = celix::serviceName<SvcWithFqn>();
    //TODO EXPECT_EQ("SvcWithFqn[Version 1]", name);

    name = celix::functionServiceName<std::function<void()>>("do");
    EXPECT_EQ("do [std::function<void()>]", name);


    name = celix::functionServiceName<std::function<std::vector<std::vector<long>>(long, int, std::vector<double>)>>("collect");
    //TODO EXPECT_EQ("collect[std::function<std::vector<std::vector<long>>(long, int, std::vector<double>)>]", name);
}