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


class FilterTestSuite : public ::testing::Test {};

TEST_F(FilterTestSuite, create_destroy){
    const char* filter_str = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))";
    celix_filter_t * get_filter;

    get_filter = celix_filter_create(filter_str);
    ASSERT_TRUE(get_filter != NULL);

    celix_filter_destroy(get_filter);
}

TEST_F(FilterTestSuite, create_fail_missing_opening_brackets){
    celix_filter_t * get_filter;

    //test missing opening brackets in main filter
    const char *filter_str1 = "&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str1);
    ASSERT_TRUE(get_filter == NULL);

    //test missing opening brackets in AND comparator
    const char *filter_str2 = "(&test_attr1=attr1|(test_attr2=attr2)(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str2);
    ASSERT_TRUE(get_filter == NULL);

    //test missing opening brackets in AND comparator
    const char *filter_str3 = "(&(test_attr1=attr1)(|test_attr2=attr2(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str3);
    ASSERT_TRUE(get_filter == NULL);

    //test missing opening brackets in NOT comparator
    const char *filter_str4 = "(&(test_attr1=attr1)(!test_attr2=attr2)";
    get_filter = celix_filter_create(filter_str4);
    ASSERT_TRUE(get_filter == NULL);
}

TEST_F(FilterTestSuite, create_fail_missing_closing_brackets){
    char * filter_str;
    celix_filter_t * get_filter;
    //test missing closing brackets in substring
    filter_str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3");
    get_filter = celix_filter_create(filter_str);
    ASSERT_TRUE(get_filter == NULL);
    free(filter_str);

    //test missing closing brackets in value
    filter_str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=attr3");
    get_filter = celix_filter_create(filter_str);
    ASSERT_TRUE(get_filter == NULL);
    free(filter_str);
}

TEST_F(FilterTestSuite, create_fail_invalid_closing_brackets) {
    char *filter_str;
    celix_filter_t *get_filter;

    //test missing closing brackets in substring
    filter_str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=at(tr3)))");
    get_filter = celix_filter_create(filter_str);
    ASSERT_TRUE(get_filter == NULL);
    free(filter_str);

    //test missing closing brackets in value
    filter_str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=att(r3)))");
    get_filter = celix_filter_create(filter_str);
    ASSERT_TRUE(get_filter == NULL);
    free(filter_str);
}

TEST_F(FilterTestSuite, create_misc) {
    celix_filter_t *get_filter;
    //test trailing chars
    const char *filter_str1 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))) oh no! trailing chars";
    get_filter = celix_filter_create(filter_str1);
    ASSERT_TRUE(get_filter == NULL);

    //test half APPROX operator (should be "~=", instead is "~")
    const char *filter_str2 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3~attr3)))";
    get_filter = celix_filter_create(filter_str2);
    ASSERT_TRUE(get_filter == NULL);

    //test PRESENT operator with trailing chars (should just register as substrings: "*" and "attr3")
    const char *filter_str3 = "(test_attr3=*attr3)";
    get_filter = celix_filter_create(filter_str3);
    ASSERT_TRUE(get_filter != NULL);
    ASSERT_EQ(CELIX_FILTER_OPERAND_SUBSTRING, get_filter->operand);
    ASSERT_EQ(2, celix_arrayList_size((celix_array_list_t *) get_filter->children));
    celix_filter_destroy(get_filter);

    //test parsing a attribute of 0 length
    const char *filter_str4 = "(>=attr3)";
    get_filter = celix_filter_create(filter_str4);
    ASSERT_TRUE(get_filter == NULL);

    //test parsing a value of 0 length
    const char *filter_str5 = "(test_attr3>=)";
    get_filter = celix_filter_create(filter_str5);
    ASSERT_TRUE(get_filter == NULL);

    //test parsing a value with a escaped closing bracket "\)"
    const char *filter_str6 = "(test_attr3>=strWith\\)inIt)";
    get_filter = celix_filter_create(filter_str6);
    ASSERT_TRUE(get_filter != NULL);
    ASSERT_STREQ("strWith)inIt", (char *) get_filter->value);
    celix_filter_destroy(get_filter);

    //test parsing a substring with a escaped closing bracket "\)"
    const char *filter_str7 = "(test_attr3=strWith\\)inIt)";
    get_filter = celix_filter_create(filter_str7);
    ASSERT_TRUE(get_filter != NULL);
    ASSERT_STREQ("strWith)inIt", (char *) get_filter->value);
    celix_filter_destroy(get_filter);
}

TEST_F(FilterTestSuite, match_comparators) {
    char *filter_str;
    celix_filter_t *filter;
    celix_properties_t *props = celix_properties_create();
    char *key = celix_utils_strdup("test_attr1");
    char *val = celix_utils_strdup("attr1");
    char *key2 = celix_utils_strdup("test_attr2");
    char *val2 = celix_utils_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);
    //test AND
    filter_str = celix_utils_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(!(test_attr3=attr3))))");
    filter = celix_filter_create(filter_str);
    bool result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);

    //test AND false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1=attr1)(test_attr1=attr2))");
    filter = celix_filter_create(filter_str);
    ASSERT_TRUE(filter == nullptr);
    result = celix_filter_match(filter, props);
    ASSERT_TRUE(result);

//cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);
}

TEST_F(FilterTestSuite, match_operators) {
    char *filter_str;
    celix_filter_t *filter;
    celix_properties_t *props = celix_properties_create();
    char *key = celix_utils_strdup("test_attr1");
    char *val = celix_utils_strdup("attr1");
    char *key2 = celix_utils_strdup("test_attr2");
    char *val2 = celix_utils_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    // Test EQUALS
    filter_str = celix_utils_strdup("(test_attr1=attr1)");
    filter = celix_filter_create(filter_str);
    bool result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test EQUALS false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1=falseString)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1~=attr1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1~=ATTR1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1=*)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test NOT PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr3=*)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    // Test NOT PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(!(test_attr3=*))");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test LESSEQUAL less
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1<=attr5)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    // Test LESSEQUAL equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2<=attr2)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    //test LESSEQUAL false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2<=attr1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test GREATEREQUAL greater
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2>=attr1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    //test GREATEREQUAL equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2>=attr2)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    //test GREATEREQUAL false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1>=attr5)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test LESS less
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1<attr5)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    //test LESS equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2<attr2)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test LESS false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2<attr1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test GREATER greater
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2>attr1)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    //test GREATER equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr2>attr2)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test GREATER false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1>attr5)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //test SUBSTRING equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1=attr*)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_TRUE(result);

    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = celix_utils_strdup("(test_attr1=attr*charsNotPresent)");
    filter = celix_filter_create(filter_str);
    result = celix_filter_match(filter, props);
    EXPECT_FALSE(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);
}

TEST_F(FilterTestSuite, match_recursion) {
    auto* filter_str = "(&(test_attr1=attr1)(|(&(test_attr2=attr2)(!(&(test_attr1=attr1)(test_attr3=attr3))))(test_attr3=attr3)))";
    auto* filter = celix_filter_create(filter_str);
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

TEST_F(FilterTestSuite, match_false) {
    auto* filter_str = "(&(test_attr1=attr1)(&(test_attr2=attr2)(test_attr3=attr3)))";
    celix_filter_t* filter = celix_filter_create(filter_str);
    celix_properties_t* props = celix_properties_create();
    auto* key = "test_attr1";
    auto* val = "attr1";
    auto* key2 = "test_attr2";
    auto* val2 = "attr2";
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    bool result = celix_filter_match(filter, props);
    ASSERT_FALSE(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
}

TEST_F(FilterTestSuite, getString) {
    auto* filter_str = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))";
    celix_filter_t * filter = celix_filter_create(filter_str);

    const char * get_str = celix_filter_getFilterString(filter);
    ASSERT_STREQ(filter_str, get_str);

    //cleanup
    celix_filter_destroy(filter);
}
