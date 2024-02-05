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

#include "celix_filter.h"
#include "celix_utils.h"
#include "celix_err.h"

class FilterTestSuite : public ::testing::Test {
  public:
    FilterTestSuite() {
        celix_err_resetErrors();
    }

    ~FilterTestSuite() override {
        celix_err_printErrors(stderr, nullptr, nullptr);
    }
};

TEST_F(FilterTestSuite, CreateDestroyTest) {
    const char* filter_str = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))";
    celix_filter_t* filter;

    filter = celix_filter_create(filter_str);
    ASSERT_TRUE(filter != nullptr);

    celix_filter_destroy(filter);
    celix_filter_destroy(nullptr);
}

TEST_F(FilterTestSuite, MissingOpenBracketsCreateTest) {
    celix_filter_t* filter;

    // test missing opening brackets in main filter
    const char* str1 = "&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))";
    filter = celix_filter_create(str1);
    ASSERT_TRUE(filter == nullptr);

    // test missing opening brackets in AND comparator
    const char* str2 = "(&test_attr1=attr1|(test_attr2=attr2)(test_attr3=attr3))";
    filter = celix_filter_create(str2);
    ASSERT_TRUE(filter == nullptr);

    // test missing opening brackets in AND comparator
    const char* str3 = "(&(test_attr1=attr1)(|test_attr2=attr2(test_attr3=attr3))";
    filter = celix_filter_create(str3);
    ASSERT_TRUE(filter == nullptr);

    // test missing opening brackets in NOT comparator
    const char* str4 = "(&(test_attr1=attr1)(!test_attr2=attr2)";
    filter = celix_filter_create(str4);
    ASSERT_TRUE(filter == nullptr);
}

TEST_F(FilterTestSuite, MissingClosingBracketsCreateTest) {
    char* str;
    celix_filter_t* filter;
    // test missing closing brackets in substring
    str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3");
    filter = celix_filter_create(str);
    ASSERT_TRUE(filter == nullptr);
    free(str);

    // test missing closing brackets in value
    str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=attr3");
    filter = celix_filter_create(str);
    ASSERT_TRUE(filter == nullptr);
    free(str);
}

TEST_F(FilterTestSuite, InvalidClosingBracketsCreateTest) {
    celix_filter_t* filter;

    // test missing closing brackets in substring
    filter = celix_filter_create("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=at(tr3)))");
    ASSERT_TRUE(filter == nullptr);

    // test missing closing brackets in value
    filter = celix_filter_create("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=att(r3)))");
    ASSERT_TRUE(filter == nullptr);

    // test invalid AND ending
    filter = celix_filter_create("(&(");
    ASSERT_TRUE(filter == nullptr);
}

TEST_F(FilterTestSuite, MiscInvalidCreateTest) {
    celix_filter_t* filter;
    // test trailing chars
    const char* str1 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))) oh no! trailing chars";
    filter = celix_filter_create(str1);
    ASSERT_TRUE(filter == nullptr);

    // test half APPROX operator (should be "~=", instead is "~")
    const char* str2 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3~attr3)))";
    filter = celix_filter_create(str2);
    ASSERT_TRUE(filter == nullptr);

    // test PRESENT operator with trailing chars (should just register as substrings: "*" and "attr3")
    const char* str3 = "(test_attr3=*attr3)";
    filter = celix_filter_create(str3);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_EQ(CELIX_FILTER_OPERAND_SUBSTRING, filter->operand);
    ASSERT_EQ(2, celix_arrayList_size((celix_array_list_t*)filter->children));
    celix_filter_destroy(filter);

    // test parsing an attribute of 0 length
    const char* str4 = "(>=attr3)";
    filter = celix_filter_create(str4);
    ASSERT_TRUE(filter == nullptr);

    // test parsing a value of 0 length
    const char* str5 = "(test_attr3>=)";
    filter = celix_filter_create(str5);
    ASSERT_FALSE(filter == nullptr);
    celix_filter_destroy(filter);

    // test parsing a value with an escaped closing parenthesis "\ ")"
    const char* str6 = "(test_attr3>=strWith\\)inIt)";
    filter = celix_filter_create(str6);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_STREQ("strWith)inIt", (char*)filter->value);
    celix_filter_destroy(filter);

    // test parsing a substring with an escaped closing parenthesis "\"
    const char* str7 = "(test_attr3=strWith\\)inIt)";
    filter = celix_filter_create(str7);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_STREQ("strWith)inIt", (char*)filter->value);
    celix_filter_destroy(filter);

    // test parsing incomplete substring operator
    const char* str8 = "(test_attr3=*attr3\\";
    filter = celix_filter_create(str8);
    ASSERT_TRUE(filter == nullptr);

    // test parsing incomplete substring operator
    const char* str9 = "(test_attr3=*attr3";
    filter = celix_filter_create(str9);
    ASSERT_TRUE(filter == nullptr);

    // test parsing fiter missing )
    const char* str10 = "(|(a=b)";
    filter = celix_filter_create(str10);
    ASSERT_TRUE(filter == nullptr);

    // test parsing APPROX operator missing value
    const char* str11 = "(a~=)";
    filter = celix_filter_create(str11);
    ASSERT_FALSE(filter == nullptr);
    celix_filter_destroy(filter);

    // test parsing LESS operator missing value
    const char* str12 = "(a<)";
    filter = celix_filter_create(str12);
    ASSERT_FALSE(filter == nullptr);
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, MatchEqualTest) {
    celix_filter_t* filter;
    celix_properties_t* props = celix_properties_create();
    const char* key = "test_attr1";
    const char* val = "attr1";
    const char* key2 = "test_attr2";
    const char* val2 = "attr2";
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    // test AND
    filter = celix_filter_create("(&(test_attr1=attr1)(|(test_attr2=attr2)(!(test_attr3=attr3))))");
    bool result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);
    celix_filter_destroy(filter);

    // test AND false
    filter = celix_filter_create("(&(test_attr1=attr1)(test_attr1=attr2))");
    result = celix_filter_match(filter, props);
    ASSERT_FALSE(result);
    celix_filter_destroy(filter);

    // cleanup
    celix_properties_destroy(props);
}

TEST_F(FilterTestSuite, MatchTest) {
    celix_filter_t* filter;
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    ASSERT_NE(nullptr, props);
    celix_properties_set(props, "test_attr1", "attr1");
    celix_properties_set(props, "test_attr2", "attr2");
    celix_properties_set(props, "empty_attr", ""); //note empty string as value

    // Test EQUALS
    filter = celix_filter_create("(test_attr1=attr1)");
    bool result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // Test EQUALS false
    filter = celix_filter_create("(test_attr1=falseString)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    filter = celix_filter_create("(test_attr1~=attr1)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    filter = celix_filter_create("(test_attr1~=ATTR1)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    filter = celix_filter_create("(empty_attr=");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    filter = celix_filter_create("(empty_attr=val)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    // Test PRESENT
    filter = celix_filter_create("(test_attr1=*)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // Test NOT PRESENT
    filter = celix_filter_create("(test_attr3=*)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    // Test NOT PRESENT
    filter = celix_filter_create("(!(test_attr3=*))");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // Test LESSEQUAL less
    filter = celix_filter_create("(test_attr1<=attr5)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // Test LESSEQUAL equals
    filter = celix_filter_create("(test_attr2<=attr2)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // test LESSEQUAL false
    filter = celix_filter_create("(test_attr2<=attr1)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    // test GREATEREQUAL greater
    filter = celix_filter_create("(test_attr2>=attr1)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // test GREATEREQUAL equals
    filter = celix_filter_create("(test_attr2>=attr2)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // test GREATEREQUAL false
    filter = celix_filter_create("(test_attr1>=attr5)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    // test LESS less
    filter = celix_filter_create("(test_attr1<attr5)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    // test LESS equals
    filter = celix_filter_create("(test_attr2<attr2)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);

    // test LESS false
    filter = celix_filter_create("(test_attr2<attr1)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);


    // test GREATER greater
    filter = celix_filter_create("(test_attr2>attr1)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);


    // test GREATER equals
    filter = celix_filter_create("(test_attr2>attr2)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);


    // test GREATER false
    filter = celix_filter_create("(test_attr1>attr5)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);


    // test SUBSTRING equals
    filter = celix_filter_create("(test_attr1=attr*)");
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);
    celix_filter_destroy(filter);

    filter = celix_filter_create("(test_attr1=attr*charsNotPresent)");
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, MatchWithNullTest) {
    //match with null filter, should return true
    EXPECT_TRUE(celix_filter_match(nullptr, nullptr));

    //match with null properties with a valid match all filter, should return true
    celix_autoptr(celix_filter_t) filter1 = celix_filter_create("(|)");
    EXPECT_TRUE(celix_filter_match(filter1, nullptr));

    //match with null properties, should return false if an attribute is needed
    celix_autoptr(celix_filter_t) filter2 = celix_filter_create("(test_attr1=attr1)");
    EXPECT_FALSE(celix_filter_match(filter2, nullptr));
}


TEST_F(FilterTestSuite, MatchRecursionTest) {
    auto* str = "(&(test_attr1=attr1)(|(&(test_attr2=attr2)(!(&(test_attr1=attr1)(test_attr3=attr3))))(test_attr3=attr3)))";
    auto* filter = celix_filter_create(str);
    auto* props = celix_properties_create();
    auto* key = "test_attr1";
    auto* val = "attr1";
    auto* key2 = "test_attr2";
    auto* val2 = "attr2";
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);
    bool result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);

    celix_properties_destroy(props);
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, FalseMatchTest) {
    auto* str = "(&(test_attr1=attr1)(&(test_attr2=attr2)(test_attr3=attr3)))";
    celix_autoptr(celix_filter_t) filter = celix_filter_create(str);
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    auto* key = "test_attr1";
    auto* val = "attr1";
    auto* key2 = "test_attr2";
    auto* val2 = "attr2";
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);
    bool result = celix_filter_match(filter, props);
    ASSERT_FALSE(result);

    celix_autoptr(celix_filter_t) filter2 = celix_filter_create("(|(non-existing=false))");
    result = celix_filter_match(filter2, props);
    ASSERT_FALSE(result);
}

TEST_F(FilterTestSuite, GetStringTest) {
    auto* str = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))";
    celix_filter_t* filter = celix_filter_create(str);

    const char* get_str = celix_filter_getFilterString(filter);
    ASSERT_STREQ(str, get_str);

    // cleanup
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, FilterEqualsTest) {
    celix_autoptr(celix_filter_t) f1 = celix_filter_create("(test_attr1=attr1)");
    celix_autoptr(celix_filter_t) f2 = celix_filter_create("(test_attr1=attr1)");
    celix_autoptr(celix_filter_t) f3 = celix_filter_create("(test_attr1>attr1)");
    celix_autoptr(celix_filter_t) f4 = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2))");
    celix_autoptr(celix_filter_t) f5 = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2))");
    celix_autoptr(celix_filter_t) f6 = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2)(test_attr3=attr3))");
    EXPECT_TRUE(celix_filter_equals(f1, f1));
    EXPECT_FALSE(celix_filter_equals(f1, nullptr));
    EXPECT_TRUE(celix_filter_equals(f1, f2));
    EXPECT_FALSE(celix_filter_equals(f1, f3));
    EXPECT_TRUE(celix_filter_equals(f4, f5));
    EXPECT_FALSE(celix_filter_equals(f4, f6));

    // equals with substring test
    celix_autoptr(celix_filter_t) f7 = celix_filter_create("(test_attr1=*test*)");
    celix_autoptr(celix_filter_t) f8 = celix_filter_create("(test_attr1=*test*)");
    celix_autoptr(celix_filter_t) f9 = celix_filter_create("(test_attr1=*test)");
    EXPECT_TRUE(celix_filter_equals(f7, f8));
    EXPECT_FALSE(celix_filter_equals(f7, f9));
}

TEST_F(FilterTestSuite, AutoCleanupTest) {
    celix_autoptr(celix_filter_t) filter = celix_filter_create("(test_attr1=attr1)");
}

TEST_F(FilterTestSuite, MissingOperandCreateTest) {
    celix_filter_t* filter = celix_filter_create("(&(test))");
    ASSERT_TRUE(filter == nullptr);

    filter = celix_filter_create("(!(test))");
    ASSERT_TRUE(filter == nullptr);
}

TEST_F(FilterTestSuite, TypedPropertiesAndFilterTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_autoptr(celix_version_t) version = celix_version_createVersionFromString("1.2.3");
    celix_properties_set(props, "str", "test");
    celix_properties_setLong(props, "long", 1L);
    celix_properties_setDouble(props, "double", 0.0);
    celix_properties_setBool(props, "bool", true);
    celix_properties_setBool(props, "bool2", false);
    celix_properties_set(props, "strBool", "true");
    celix_properties_setVersion(props, "version", version);

    celix_autoptr(celix_filter_t) filter1 =
        celix_filter_create("(&(str=test)(long=1)(double=0.0)(bool=true)(version=1.2.3))");
    EXPECT_TRUE(celix_filter_match(filter1, props));

    celix_autoptr(celix_filter_t) filter2 =
        celix_filter_create("(&(str>tes)(long>0)(double>-1.0)(bool>false)(version>1.0.0))");
    EXPECT_TRUE(celix_filter_match(filter2, props));

    celix_autoptr(celix_filter_t) filter3 =
                celix_filter_create("(&(str<tesu)(long<2)(double<1.0)(bool<=true)(version<2.0.0))");
    EXPECT_TRUE(celix_filter_match(filter3, props));

    celix_autoptr(celix_filter_t) filter4 =
            celix_filter_create("(&(str!=test)(long!=1)(double!=0.0)(bool!=true)(bool2=true)(version!=1.2.3))");
    EXPECT_FALSE(celix_filter_match(filter4, props));

    celix_autoptr(celix_filter_t) filter5 = celix_filter_create("(strBool=true)");
    EXPECT_TRUE(celix_filter_match(filter5, props));
}


TEST_F(FilterTestSuite, SubStringTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "test", "John Bob Doe");
    celix_properties_set(props, "test2", "*ValueWithStar");
    celix_properties_set(props, "test3", " Value");

    //test filter with matching subInitial
    celix_autoptr(celix_filter_t) filter1 = celix_filter_create("(test=Jo*)");
    EXPECT_NE(nullptr, filter1);
    EXPECT_TRUE(celix_filter_match(filter1, props));

    //test filter with un-matching subInitial
    celix_autoptr(celix_filter_t) filter2 = celix_filter_create("(test=Joo*)");
    EXPECT_NE(nullptr, filter2);
    EXPECT_FALSE(celix_filter_match(filter2, props));

    //test filter with matching subFinal
    celix_autoptr(celix_filter_t) filter3 = celix_filter_create("(test=*Doe)");
    EXPECT_NE(nullptr, filter3);
    EXPECT_TRUE(celix_filter_match(filter3, props));

    //test filter with un-matching subFinal
    celix_autoptr(celix_filter_t) filter4 = celix_filter_create("(test=*Doo)");
    EXPECT_NE(nullptr, filter4);
    EXPECT_FALSE(celix_filter_match(filter4, props));

    //test filter with matching subAny
    celix_autoptr(celix_filter_t) filter5 = celix_filter_create("(test=*Bob*)");
    EXPECT_NE(nullptr, filter5);
    EXPECT_TRUE(celix_filter_match(filter5, props));

    //test filter with un-matching subAny
    celix_autoptr(celix_filter_t) filter6 = celix_filter_create("(test=*Boo*)");
    EXPECT_NE(nullptr, filter6);
    EXPECT_FALSE(celix_filter_match(filter6, props));

    //test filter with matching subAny, subInitial and subFinal
    celix_autoptr(celix_filter_t) filter7 = celix_filter_create("(test=Jo*Bob*Doe)");
    EXPECT_NE(nullptr, filter7);
    EXPECT_TRUE(celix_filter_match(filter7, props));

    //test filter with un-matching subAny, subInitial and subFinal
    celix_autoptr(celix_filter_t) filter8 = celix_filter_create("(test=Jo*Boo*Doe)");
    EXPECT_NE(nullptr, filter8);
    EXPECT_FALSE(celix_filter_match(filter8, props));

    //test filter with un-matching overlapping subAny and subInitial
    celix_autoptr(celix_filter_t) filter9 = celix_filter_create("(test=John B*Bob*b Doe)");
    EXPECT_NE(nullptr, filter9);
    EXPECT_FALSE(celix_filter_match(filter9, props));

    //test filter with un-matching overlapping subAny and subFinal
    celix_autoptr(celix_filter_t) filter10 = celix_filter_create("(test=*Bob*b Doe)");
    EXPECT_NE(nullptr, filter10);
    EXPECT_FALSE(celix_filter_match(filter10, props));

    //test filter with a starting escaped asterisk
    celix_autoptr(celix_filter_t) filter11 = celix_filter_create("(test2=\\*Value*)");
    EXPECT_NE(nullptr, filter11);
    EXPECT_TRUE(celix_filter_match(filter11, props));

    //test filter with an invalid substring
    celix_autoptr(celix_filter_t) filter12 = celix_filter_create("(test=Bob*");
    EXPECT_EQ(nullptr, filter12);

    //test filter with mathing subInitial beginning with whitespace
    celix_autoptr(celix_filter_t) filter13 = celix_filter_create("(test3= Value*)");
    EXPECT_NE(nullptr, filter13);
    EXPECT_TRUE(celix_filter_match(filter13, props));

    //test filter with matching subInitial consisting of spaces
    celix_autoptr(celix_filter_t) filter14 = celix_filter_create("(test3= *)");
    EXPECT_NE(nullptr, filter14);
    EXPECT_TRUE(celix_filter_match(filter14, props));

    //test filter with double escaped asterisk on both sides
    celix_autoptr(celix_filter_t) filter15 = celix_filter_create("(test=*John*)");
    EXPECT_NE(nullptr, filter15);
    EXPECT_TRUE(celix_filter_match(filter15, props));
}

TEST_F(FilterTestSuite, CreateEmptyFilter) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_autoptr(celix_filter_t) filter1 = celix_filter_create(nullptr); //fallback to "(|)"
    EXPECT_TRUE(filter1 != nullptr);
    EXPECT_TRUE(celix_filter_match(filter1, props));

    celix_autoptr(celix_filter_t) filter2 = celix_filter_create(""); //fallback to "(|)"
    EXPECT_TRUE(filter2 != nullptr);
    EXPECT_TRUE(celix_filter_match(filter2, props));

    celix_autoptr(celix_filter_t) filter3 = celix_filter_create("(|)");
    EXPECT_TRUE(filter3 != nullptr);
    EXPECT_TRUE(celix_filter_match(filter3, props));
}

TEST_F(FilterTestSuite, LogicalOperatorsWithNoCriteriaTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_autoptr(celix_filter_t) filter1 = celix_filter_create("(&)");
    EXPECT_TRUE(filter1 != nullptr);
    EXPECT_TRUE(celix_filter_match(filter1, props));

    celix_autoptr(celix_filter_t) filter2 = celix_filter_create("(|)");
    EXPECT_TRUE(filter2 != nullptr);
    EXPECT_TRUE(celix_filter_match(filter2, props));

    celix_autoptr(celix_filter_t) filter3 = celix_filter_create("(!)"); //NOT with no criteria is not valid
    EXPECT_TRUE(filter3 == nullptr);
}


TEST_F(FilterTestSuite, InvalidEscapeTest) {
    EXPECT_EQ(nullptr, celix_filter_create("(\\")); //escape without following char
}


#include "filter.h"
TEST_F(FilterTestSuite, DeprecatedApiTest) {
    auto* f1 = filter_create("(test_attr1=attr1)");
    auto* f2 = filter_create("(test_attr1=attr1)");
    bool result;
    auto status = filter_match_filter(f1, f2, &result);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_TRUE(result);

    const char* str;
    status = filter_getString(f1, &str);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_STREQ(str, "(test_attr1=attr1)");

    status = filter_match(f1, nullptr, &result);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_FALSE(result);

    filter_destroy(f1);
    filter_destroy(f2);
}
