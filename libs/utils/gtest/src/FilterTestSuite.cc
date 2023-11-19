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
    char* str;
    celix_filter_t* filter;

    // test missing closing brackets in substring
    str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=at(tr3)))");
    filter = celix_filter_create(str);
    ASSERT_TRUE(filter == nullptr);
    free(str);

    // test missing closing brackets in value
    str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=att(r3)))");
    filter = celix_filter_create(str);
    ASSERT_TRUE(filter == nullptr);
    free(str);
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
    ASSERT_TRUE(filter == nullptr);

    // test parsing a value with a escaped closing bracket "\)"
    const char* str6 = "(test_attr3>=strWith\\)inIt)";
    filter = celix_filter_create(str6);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_STREQ("strWith)inIt", (char*)filter->value);
    celix_filter_destroy(filter);

    // test parsing a substring with an escaped closing bracket "\)"
    const char* str7 = "(test_attr3=strWith\\)inIt)";
    filter = celix_filter_create(str7);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_STREQ("strWith)inIt", (char*)filter->value);
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, MatchEqualTest) {
    char* str;
    celix_filter_t* filter;
    celix_properties_t* props = celix_properties_create();
    char* key = celix_utils_strdup("test_attr1");
    char* val = celix_utils_strdup("attr1");
    char* key2 = celix_utils_strdup("test_attr2");
    char* val2 = celix_utils_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);
    // test AND
    str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(!(test_attr3=attr3))))");
    filter = celix_filter_create(str);
    bool result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);

    // test AND false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1=attr1)(test_attr1=attr2))");
    filter = celix_filter_create(str);
    ASSERT_TRUE(filter == nullptr);
    result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);

    // cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(str);
    free(key);
    free(key2);
    free(val);
    free(val2);
}

TEST_F(FilterTestSuite, MatchTest) {
    char* str;
    celix_filter_t* filter;
    celix_properties_t* props = celix_properties_create();
    char* key = celix_utils_strdup("test_attr1");
    char* val = celix_utils_strdup("attr1");
    char* key2 = celix_utils_strdup("test_attr2");
    char* val2 = celix_utils_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    // Test EQUALS
    str = celix_utils_strdup("(test_attr1=attr1)");
    filter = celix_filter_create(str);
    bool result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test EQUALS false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1=falseString)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1~=attr1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1~=ATTR1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test PRESENT
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1=*)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test NOT PRESENT
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr3=*)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // Test NOT PRESENT
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(!(test_attr3=*))");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test LESSEQUAL less
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1<=attr5)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test LESSEQUAL equals
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2<=attr2)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // test LESSEQUAL false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2<=attr1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test GREATEREQUAL greater
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2>=attr1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // test GREATEREQUAL equals
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2>=attr2)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // test GREATEREQUAL false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1>=attr5)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test LESS less
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1<attr5)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // test LESS equals
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2<attr2)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test LESS false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2<attr1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test GREATER greater
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2>attr1)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // test GREATER equals
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr2>attr2)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test GREATER false
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1>attr5)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // test SUBSTRING equals
    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1=attr*)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    celix_filter_destroy(filter);
    free(str);
    str = celix_utils_strdup("(test_attr1=attr*charsNotPresent)");
    filter = celix_filter_create(str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(str);
    free(key);
    free(key2);
    free(val);
    free(val2);
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
    celix_filter_t* filter = celix_filter_create(str);
    celix_properties_t* props = celix_properties_create();
    auto* key = "test_attr1";
    auto* val = "attr1";
    auto* key2 = "test_attr2";
    auto* val2 = "attr2";
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    bool result = celix_filter_match(filter, props);
    ASSERT_FALSE(result);

    // cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
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
    auto* f1 = celix_filter_create("(test_attr1=attr1)");
    auto* f2 = celix_filter_create("(test_attr1=attr1)");
    auto* f3 = celix_filter_create("(test_attr1=attr2)");
    EXPECT_TRUE(celix_filter_equals(f1, f2));
    EXPECT_FALSE(celix_filter_equals(f1, f3));
    celix_filter_destroy(f1);
    celix_filter_destroy(f2);
    celix_filter_destroy(f3);
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
            celix_filter_create("(&(str!=test)(long!=1)(double!=0.0)(bool!=true)(version!=1.2.3))");
    EXPECT_FALSE(celix_filter_match(filter4, props));
}


TEST_F(FilterTestSuite, SubStringTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "test", "John Bob Doe");

    //test filter with matching subInitial
    celix_autoptr(celix_filter_t) filter1 = celix_filter_create("(test=Jo*)");
    EXPECT_TRUE(celix_filter_match(filter1, props));

    //test filter with un-matching subInitial
    celix_autoptr(celix_filter_t) filter2 = celix_filter_create("(test=Joo*)");
    EXPECT_FALSE(celix_filter_match(filter2, props));

    //test filter with matching subFinal
    celix_autoptr(celix_filter_t) filter3 = celix_filter_create("(test=*Doe)");
    EXPECT_TRUE(celix_filter_match(filter3, props));

    //test filter with un-matching subFinal
    celix_autoptr(celix_filter_t) filter4 = celix_filter_create("(test=*Doo)");
    EXPECT_FALSE(celix_filter_match(filter4, props));

    //test filter with matching subAny
    celix_autoptr(celix_filter_t) filter5 = celix_filter_create("(test=*Bob*)");
    EXPECT_TRUE(celix_filter_match(filter5, props));

    //test filter with un-matching subAny
    //TODO fixme
//    celix_autoptr(celix_filter_t) filter6 = celix_filter_create("(test=*Boo*)");
//    EXPECT_FALSE(celix_filter_match(filter6, props));

    //test filter with matching subAny, subInitial and subFinal
    celix_autoptr(celix_filter_t) filter7 = celix_filter_create("(test=Jo*Bob*Doe)");
    EXPECT_TRUE(celix_filter_match(filter7, props));

    //test filter with un-matching subAny, subInitial and subFinal
    celix_autoptr(celix_filter_t) filter8 = celix_filter_create("(test=Jo*Boo*Doe)");

    //test filter with un-matching overlapping subAny, subInitial and subFinal
    celix_autoptr(celix_filter_t) filter9 = celix_filter_create("(test=John B*Bob*b Doe)");
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

    filter_destroy(f1);
    filter_destroy(f2);
}
