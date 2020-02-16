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

#include "celix/Filter.h"

class FilterTest : public ::testing::Test {
public:
    FilterTest() {}
    ~FilterTest(){}
};

TEST_F(FilterTest, CreateFilterTest) {
    const char *input1 = "(test_attr1=attr1)";
    celix::Filter filter{input1};
    EXPECT_TRUE(filter.isValid());
    EXPECT_EQ("test_attr1", filter.getCriteria().attribute);
    EXPECT_EQ(celix::FilterOperator::EQUAL, filter.getCriteria().op);
    EXPECT_EQ("attr1", filter.getCriteria().value);
    EXPECT_EQ(0, filter.getCriteria().subcriteria.size());


    const char *input2 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))";
    celix::Filter filter2{input2};
    EXPECT_TRUE(filter2.isValid());
    EXPECT_EQ(std::string{input2}, filter2.toString());
    EXPECT_EQ(celix::FilterOperator::AND, filter2.getCriteria().op);
    EXPECT_EQ(2, filter2.getCriteria().subcriteria.size());
    EXPECT_EQ(2, filter2.getCriteria().subcriteria[1].subcriteria.size());
    EXPECT_EQ("test_attr1", filter2.getCriteria().subcriteria[0].attribute);
    EXPECT_EQ(celix::FilterOperator::EQUAL, filter2.getCriteria().subcriteria[0].op);
    EXPECT_EQ("attr1", filter2.getCriteria().subcriteria[0].value);

    //test last criteria
    EXPECT_EQ("test_attr3", filter2.getCriteria().subcriteria[1].subcriteria[1].attribute);
    EXPECT_EQ(celix::FilterOperator::EQUAL, filter2.getCriteria().subcriteria[1].subcriteria[1].op);
    EXPECT_EQ("attr3", filter2.getCriteria().subcriteria[1].subcriteria[1].value);
    EXPECT_EQ(0, filter2.getCriteria().subcriteria[1].subcriteria[1].subcriteria.size());


    //creation using a const char* literal and additional white spaces
    celix::Filter filter3 = "    (   &  (test_attr1=attr1)  (   |    (test_attr2=attr2)   (test_attr3=attr3   ) )  )    ";
    filter3.isValid();


    //test PRESENT operator with trailing chars (should just register as substrings: "*" and "attr3")
    celix::Filter f3 = "(test_attr3=*attr3)";
    EXPECT_TRUE(f3.isValid());
    EXPECT_EQ(celix::FilterOperator::SUBSTRING, f3.getCriteria().op);
    EXPECT_EQ("*attr3", f3.getCriteria().value);

    //test parsing a value with a escaped closing bracket "\)"
    celix::Filter f4 = "(test_attr3>=strWith\\)inIt)";
    EXPECT_TRUE(f4.isValid());
    EXPECT_EQ(celix::FilterOperator::GREATER_EQUAL, f4.getCriteria().op);
    EXPECT_EQ("strWith)inIt", f4.getCriteria().value);

    //test parsing a equal with a escaped closing bracket "\)"
    celix::Filter f5 = "(test_attr3=strWith\\)inIt)";
    EXPECT_TRUE(f5.isValid());
    EXPECT_EQ(celix::FilterOperator::EQUAL, f5.getCriteria().op);
    EXPECT_EQ("strWith)inIt", f5.getCriteria().value);

    //test parsing a substring with a escaped closing bracket "\)"
    celix::Filter f6 = "(test_attr3=*strWith\\)inIt)";
    EXPECT_TRUE(f6.isValid());
    EXPECT_EQ(celix::FilterOperator::SUBSTRING, f6.getCriteria().op);
    EXPECT_EQ("*strWith)inIt", f6.getCriteria().value);

    celix::Filter f7 = ""; //empty is considered a valid filter
    f7.isValid();
}

TEST_F(FilterTest, CreateInvalidFilterTest) {

    auto check = [](const char *filter) {
        std::cout << "Checking '" << filter << "'" << std::endl;
        celix::Filter f = filter;
        EXPECT_FALSE(f.isValid());
    };

    check("no-parentheses");

    //empty with whitespaces is also an invalid filter.
    check(" ");

    check("(attr=trailing_stuff) fdafa");

    //test missing opening brackets in main filter
    check("&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))");

    //test missing opening brackets in AND comparator
    check("(&test_attr1=attr1|(test_attr2=attr2)(test_attr3=attr3))");

    //test missing opening brackets in AND comparator
    check("(&(test_attr1=attr1)(|test_attr2=attr2(test_attr3=attr3))");

    //test missing opening brackets in NOT comparator
    check("(&(test_attr1=attr1)(!test_attr2=attr2)");

    //test missing closing brackets in substring
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3");

    //test missing closing brackets in value
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=attr3");

    //test missing closing brackets in substring
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=at(tr3)))");

    //test missing closing brackets in value
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=att(r3)))");

    //test trailing chars
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))) oh no! trailing chars");

    //test half APPROX operator (should be "~=", instead is "~")
    check("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3~attr3)))");;

    //test parsing a attribute of 0 length
    check("(>=attr3)");

    //test parsing a value of 0 length
    check("(test_attr3>=)");
}

TEST_F(FilterTest, MatchComparators) {
    celix::Properties props{};

    props["test_attr1"] = "attr1";
    props["test_attr2"] = "attr2";

    //test present
    {
        celix::Filter f1 = "(test_attr1=*)"; //match true
        celix::Filter f2 = "(test_attr3=*)"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }

    //test equal
    {
        celix::Filter f1 = "(test_attr1=attr1)"; //match true
        celix::Filter f2 = "(test_attr1=bla)"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }

    //test not
    {
        celix::Filter f1 = "(!(test_attr1=bla))"; //match true
        celix::Filter f2 = "(!(test_attr1=attr1))"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }

    //test AND
    {
        celix::Filter f1 = "(&(test_attr1=attr1)(test_attr2=attr2))"; //match true
        celix::Filter f2 = "(&(test_attr1=attr1)(test_attr3=attr3))"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }

    //test OR
    {
        celix::Filter f1 = "(|(test_attr3=attr3)(test_attr2=attr2))"; //match true
        celix::Filter f2 = "(|(test_attr4=attr4)(test_attr3=attr3))"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }

    //test COMBINE
    {
        celix::Filter f1 = "(|(test_attr3=attr3)(&(!(test_attr4=*))(test_attr2=attr2)))"; //match true
        celix::Filter f2 = "(|(test_attr3=attr3)(&(test_attr4=*)(test_attr2=attr2)))"; //match false
        EXPECT_TRUE(f1.isValid());
        EXPECT_TRUE(f2.isValid());
        EXPECT_TRUE(f1.match(props));
        EXPECT_FALSE(f2.match(props));
    }
}
