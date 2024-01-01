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
#include <unordered_map>
#include <map>

#include "celix/Version.h"

class CxxVersionTestSuite : public ::testing::Test {};

#include "gtest/gtest.h"
#include "celix/Version.h"

TEST_F(CxxVersionTestSuite, DefaultConstructorTest) {
    celix::Version v;
    EXPECT_EQ(0, v.getMajor());
    EXPECT_EQ(0, v.getMinor());
    EXPECT_EQ(0, v.getMicro());
    EXPECT_EQ("", v.getQualifier());
}

TEST_F(CxxVersionTestSuite, ConstructorTest) {
    celix::Version v1{1, 2, 3, "qualifier"};
    EXPECT_EQ(1, v1.getMajor());
    EXPECT_EQ(2, v1.getMinor());
    EXPECT_EQ(3, v1.getMicro());
    EXPECT_EQ("qualifier", v1.getQualifier());
}

TEST_F(CxxVersionTestSuite, MoveConstructorTest) {
    celix::Version v1{2, 3, 4, "qualifier"};
    celix::Version v2{std::move(v1)};

    //v2 should have the values of v1 before the move
    EXPECT_EQ(v2.getMajor(), 2);
    EXPECT_EQ(v2.getMinor(), 3);
    EXPECT_EQ(v2.getMicro(), 4);
    EXPECT_EQ(v2.getQualifier(), "qualifier");
}

TEST_F(CxxVersionTestSuite, CopyConstructorTest) {
    celix::Version v1{1, 2, 3, "qualifier"};
    celix::Version v2 = v1;

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1.getMajor(), v2.getMajor());
    EXPECT_EQ(v1.getMinor(), v2.getMinor());
    EXPECT_EQ(v1.getMicro(), v2.getMicro());
    EXPECT_EQ(v1.getQualifier(), v2.getQualifier());
}

TEST_F(CxxVersionTestSuite, MapKeyTest) {
    // Test using Version as key in std::map
    std::map<celix::Version, int> map;
    map[celix::Version{1, 2, 3}] = 1;
    map[celix::Version{3, 2, 1}] = 2;

    EXPECT_EQ(map[(celix::Version{1, 2, 3})], 1);
    EXPECT_EQ(map[(celix::Version{3, 2, 1})], 2);
}

TEST_F(CxxVersionTestSuite, UnorderedMapKeyTest) {
    // Test using Version as key in std::unordered_map
    std::unordered_map<celix::Version, int> unorderedMap;
    unorderedMap[celix::Version{1, 2, 3}] = 1;
    unorderedMap[celix::Version{3, 2, 1}] = 2;

    EXPECT_EQ(unorderedMap[(celix::Version{1, 2, 3})], 1);
    EXPECT_EQ(unorderedMap[(celix::Version{3, 2, 1})], 2);
}
