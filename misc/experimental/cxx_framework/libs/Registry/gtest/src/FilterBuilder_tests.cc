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

#include "gtest/gtest.h"

#include "celix/FilterBuilder.h"

class FilterBuilderTest : public ::testing::Test {
public:
    FilterBuilderTest() = default;
    ~FilterBuilderTest() override = default;
};

TEST_F(FilterBuilderTest, BuildEqualFilter) {
    celix::Filter filter = celix::FilterBuilder::where("key").is("value").build();
    EXPECT_FALSE(filter.isEmpty());
    EXPECT_EQ(std::string{"(key=value)"}, filter.toString());
}

//TEST_F(FilterBuilderTest, BuildEqualOperatorFilter) {
//    celix::Filter filter = celix::FilterBuilder::where("key") == "value";
//    EXPECT_FALSE(filter.isEmpty());
//    EXPECT_TRUE(filter.isValid());
//    EXPECT_EQ(std::string{"(key=value)"}, filter.toString());
//}

TEST_F(FilterBuilderTest, BuildAndFilter) {
    celix::Filter filter = celix::FilterBuilder::where("key1").is("value1").andd("key2").is("value2").build();
    EXPECT_FALSE(filter.isEmpty());
    EXPECT_EQ(std::string{"(&(key1=value1)(key2=value2))"}, filter.toString());
}



