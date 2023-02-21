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

#include "celix/Utils.h"

class CxxUtilsTestSuite : public ::testing::Test {
public:
};

namespace example {
    class TestType {
    public:
        std::string NAME; //dummy non-static NAME member, should not impact the typeName call
        std::string VERSION; //dummy non-static VERSION member, should not impact the typeVersion call
    };

#if __cplusplus >= 201703L //C++17 or higher
    class TestType2 {
    public:
        static constexpr std::string_view NAME = "AnotherTestTypeName";
        static constexpr const char * const VERSION = "1.2.0";
    };
#endif

    class TestType3 {
    public:
        static constexpr int NAME = 4; //dummy static int, which should not be used as type name (cannot be used to construct a string)
        static constexpr int VERSION = 1; //dummy static int, which should not be used as type name (cannot be used to construct a string)
    };
}


TEST_F(CxxUtilsTestSuite, testTypeName) {
    //When inferring a type name with no provided name and the type does not have a NAME static member,
    //the call should return an inferred name based on __PRETTY__FUNCTION__ (see celix::impl::extractTypeName)
    auto name = celix::typeName<example::TestType>();
    EXPECT_FALSE(name.empty());

#if __cplusplus >= 201703L //C++17 or higher
    //When inferring a type name with no provided name, but the type has a NAME static member,
    //the call should return the string value of the NAME static member (only support if C++17 is used).
    name = celix::typeName<example::TestType2>();
    EXPECT_EQ(std::string{name}, std::string{"AnotherTestTypeName"});
#endif

    //When inferring a type name with a provided name and the type does not have a NAME static member,
    //the call should return the provided name.
    name = celix::typeName<example::TestType>("OverrideName");
    EXPECT_EQ(name, std::string{"OverrideName"});

#if __cplusplus >= 201703L //C++17 or higher
    //When inferring a type name with a provided name and the type also has a NAME static member,
    //the call should return the provided name (only support if C++17 is used).
    name = celix::typeName<example::TestType2>("OverrideName");
    EXPECT_EQ(name, std::string{"OverrideName"});
#endif

    //When inferring a type name, where there is a static NAME member but not of the right type (constructable from string),
    //the call should return an inferred name based on __PRETTY_FUNCTION__ (see celix::impl::extractTypeName)
    name = celix::typeName<example::TestType3>();
    //note not testing what exactly is inferred (__PRETTY_FUNCTION__ can differ per platform)
    EXPECT_NE(std::string{"4"}, name);
}

TEST_F(CxxUtilsTestSuite, testTypeVersion) {
    //When inferring a type version with no provided version and the type does not have a VERSION static member,
    //the call should return an empty version.
    auto version = celix::typeVersion<example::TestType>();
    EXPECT_TRUE(version.empty());

#if __cplusplus >= 201703L //C++17 or higher
    //When inferring a type version with no provided version and the type has a VERSION static member,
    //the call should return the value of the static member VERSION (only support if C++17 is used).
    version = celix::typeVersion<example::TestType2>();
    EXPECT_EQ(std::string{version}, std::string{"1.2.0"});
#endif

    //When inferring a type version with a provided version and the type does not have a VERSION static member,
    //the call should return the provided version.
    version = celix::typeVersion<example::TestType>("2.2.2");
    EXPECT_EQ(version, std::string{"2.2.2"});

#if __cplusplus >= 201703L //C++17 or higher
    //When inferring a type version with a provided version and the type does have a VERSION static member,
    //the call should return the provided version (only support if C++17 is used).
    version = celix::typeVersion<example::TestType2>("2.2.2");
    EXPECT_EQ(version, std::string{"2.2.2"});
#endif
}

TEST_F(CxxUtilsTestSuite, testSplit) {
    auto tokens = celix::split("item1,item2,item3");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "item1");
    EXPECT_EQ(tokens[1], "item2");
    EXPECT_EQ(tokens[2], "item3");

    tokens = celix::split("  item1 , item2  ,  item3,item4  ");
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "item1");
    EXPECT_EQ(tokens[1], "item2");
    EXPECT_EQ(tokens[2], "item3");
    EXPECT_EQ(tokens[3], "item4");

    tokens = celix::split("  item1 ; item2  ;  item3;item4  ", ";");
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "item1");
    EXPECT_EQ(tokens[1], "item2");
    EXPECT_EQ(tokens[2], "item3");
    EXPECT_EQ(tokens[3], "item4");

    tokens = celix::split("  item1 , ");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "item1");

    tokens = celix::split("");
    EXPECT_EQ(tokens.size(), 0);

    tokens = celix::split("  , ,   ");
    EXPECT_EQ(tokens.size(), 0);
}