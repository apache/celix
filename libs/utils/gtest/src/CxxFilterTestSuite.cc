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

#include "celix/Filter.h"

class CxxFilterTestSuite : public ::testing::Test {
public:
};


TEST_F(CxxFilterTestSuite, CreateDestroy) {
    celix::Filter filter{};
    EXPECT_EQ(filter.getFilterString(), "(|)"); //match all filter
}

TEST_F(CxxFilterTestSuite, FilterString) {
    celix::Filter filter2{"(key=value)"};
    EXPECT_FALSE(filter2.empty());
    EXPECT_EQ(std::string{"(key=value)"}, filter2.getFilterString());
}

TEST_F(CxxFilterTestSuite, InvalidFilter) {
    EXPECT_THROW(celix::Filter{"bla"}, celix::FilterException);
}

TEST_F(CxxFilterTestSuite, EmptyFilterTest) {
    celix::Filter filter{};
    celix::Properties props{};
    EXPECT_TRUE(filter.match(props));
}

TEST_F(CxxFilterTestSuite, MatchTest) {
    celix::Filter filter1{"(key=value)"};
    celix::Filter filter2{"(key=value2)"};
    celix::Properties props1{};
    props1.set("key", "value");
    celix::Properties props2{};

    EXPECT_TRUE(filter1.match(props1));
    EXPECT_FALSE(filter2.match(props1));
    EXPECT_FALSE(filter1.match(props2));
    EXPECT_FALSE(filter2.match(props2));
}

TEST_F(CxxFilterTestSuite, FindAttributes) {
    celix::Filter filter1{"(&(key1=value1)(key2=value2)(|(key3=value3)(key4=*)))"};

    EXPECT_TRUE(filter1.hasAttribute("key1"));
    EXPECT_TRUE(filter1.hasAttribute("key2"));
    EXPECT_TRUE(filter1.hasAttribute("key3"));
    EXPECT_TRUE(filter1.hasAttribute("key4"));
    EXPECT_FALSE(filter1.hasAttribute("key"));

    EXPECT_EQ(filter1.findAttribute("key1"), std::string{"value1"});
    EXPECT_EQ(filter1.findAttribute("key2"), std::string{"value2"});
    EXPECT_EQ(filter1.findAttribute("key3"), std::string{"value3"});
    EXPECT_EQ(filter1.findAttribute("key4"), std::string{"*"});
    EXPECT_TRUE(filter1.findAttribute("key").empty());
}

TEST_F(CxxFilterTestSuite, HasMandatoryEqualsValueAttribute) {
    EXPECT_TRUE(celix::Filter{"(key1=value1)"}.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(!(key1=value1))"}.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(key1>=value1)"}.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(|(key1=value1)(key2=value2))"}.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(key1=*)"}.hasMandatoryEqualsValueAttribute("key1"));

    celix::Filter filter1{"(&(key1=value1)(key2=value2)(|(key3=value3)(key4=*)))"};
    EXPECT_TRUE(filter1.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_TRUE(filter1.hasMandatoryEqualsValueAttribute("key2"));
    EXPECT_FALSE(filter1.hasMandatoryEqualsValueAttribute("key3"));
    EXPECT_FALSE(filter1.hasMandatoryEqualsValueAttribute("key4"));

    celix::Filter filter2{"(&(key1=value)(!(&(key=value)(!(key3=value)))))"};
    EXPECT_TRUE(filter2.hasMandatoryEqualsValueAttribute("key1"));
    EXPECT_FALSE(filter2.hasMandatoryEqualsValueAttribute("key2"));
    EXPECT_TRUE(filter2.hasMandatoryEqualsValueAttribute("key3"));
}

TEST_F(CxxFilterTestSuite, HasMandatoryNegatedPresenceAttribute) {
    EXPECT_TRUE(celix::Filter{"(!(key1=*))"}.hasMandatoryNegatedPresenceAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(key1=*)"}.hasMandatoryNegatedPresenceAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(key1=value1)"}.hasMandatoryNegatedPresenceAttribute("key1"));
    EXPECT_FALSE(celix::Filter{"(|(!(key1=*))(key2=value))"}.hasMandatoryNegatedPresenceAttribute("key1"));

    celix::Filter filter1{"(&(!(key1=*))(key2=value2)(key3=value3))"};
    EXPECT_TRUE(filter1.hasMandatoryNegatedPresenceAttribute("key1"));
    EXPECT_FALSE(filter1.hasMandatoryNegatedPresenceAttribute("key2"));
    EXPECT_FALSE(filter1.hasMandatoryNegatedPresenceAttribute("key3"));

    celix::Filter filter2{"(&(key1=*)(!(&(key2=*)(!(key3=*)))))"};
    EXPECT_FALSE(filter2.hasMandatoryNegatedPresenceAttribute("key1"));
    EXPECT_TRUE(filter2.hasMandatoryNegatedPresenceAttribute("key2"));
    EXPECT_FALSE(filter2.hasMandatoryNegatedPresenceAttribute("key3"));
}

TEST_F(CxxFilterTestSuite, FilterForLongAttribute) {
    celix::Properties props{};
    props.set("key1", 1L);
    props.set("key2", 20L);

    EXPECT_EQ(celix::Properties::ValueType::Long, props.getType("key1"));
    EXPECT_EQ(celix::Properties::ValueType::Long, props.getType("key2"));

    celix::Filter filter1{"(key1>=1)"};
    EXPECT_TRUE(filter1.match(props));
    celix::Filter filter2{"(key2<=3)"};
    EXPECT_FALSE(filter2.match(props));
    celix::Filter filter3{"(key2>=1)"};
    EXPECT_TRUE(filter3.match(props));
}

TEST_F(CxxFilterTestSuite, FilterForDoubleAttribute) {
    celix::Properties props{};
    props.set("key1", 1.1);
    props.set("key2", 20.2);

    EXPECT_EQ(celix::Properties::ValueType::Double, props.getType("key1"));
    EXPECT_EQ(celix::Properties::ValueType::Double, props.getType("key2"));

    celix::Filter filter1{"(key1>=1.1)"};
    EXPECT_TRUE(filter1.match(props));
    celix::Filter filter2{"(key2<=3.3)"};
    EXPECT_FALSE(filter2.match(props));
}

TEST_F(CxxFilterTestSuite, FilterForVersionAttribute) {
    celix::Properties props{};
    props.set("key1", celix::Version{1, 2, 3});
    props.set("key2", celix::Version{2, 0, 0});

    EXPECT_EQ(celix::Properties::ValueType::Version, props.getType("key1"));
    EXPECT_EQ(celix::Properties::ValueType::Version, props.getType("key2"));

    celix::Filter filter1{"(key1>=1.2.3)"};
    EXPECT_TRUE(filter1.match(props));
    celix::Filter filter2{"(key2<=2.0.0)"};
    EXPECT_TRUE(filter2.match(props));
    celix::Filter filter3{"(key2<=1.2.3)"};
    EXPECT_FALSE(filter3.match(props));
}
