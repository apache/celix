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

    };
}


TEST_F(CxxUtilsTestSuite, TypenameTest) {
    auto name = celix::typeName<example::TestType>();
    EXPECT_FALSE(name.empty());
}

TEST_F(CxxUtilsTestSuite, SplitTest) {
    auto tokens = celix::split("item1,item2,item3");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "item1");
    EXPECT_EQ(tokens[1], "item2");
    EXPECT_EQ(tokens[2], "item3");

    tokens = celix::split("  item1 , item2  ,  item3  ");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "item1");
    EXPECT_EQ(tokens[1], "item2");
    EXPECT_EQ(tokens[2], "item3");

    tokens = celix::split("  item1 , ");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "item1");

    tokens = celix::split("");
    EXPECT_EQ(tokens.size(), 0);

    tokens = celix::split("  , ,   ");
    EXPECT_EQ(tokens.size(), 0);
}