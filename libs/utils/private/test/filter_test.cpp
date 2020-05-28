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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

#include "celix_filter.h"
#include "filter.h"

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char *) calloc (len + 1,sizeof(char));

    if (d == NULL) {
        return NULL;
    }

    strncpy(d,s,len);
    return d;
}

//----------------TESTGROUPS----------------
TEST_GROUP(filter) {
    void setup() {
    }

    void teardown() {
    }
};

//----------------FILTER TESTS----------------
TEST(filter, create_destroy){
    char * filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))");
    celix_filter_t * get_filter;

    get_filter = celix_filter_create(filter_str);
    CHECK(get_filter != NULL);

    celix_filter_destroy(get_filter);

    //cleanup
    free(filter_str);
}

TEST(filter, create_fail_missing_opening_brackets){
    celix_filter_t * get_filter;

    //test missing opening brackets in main filter
    //mock().expectNCalls(2, "framework_log");
    const char *filter_str1 = "&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str1);
    POINTERS_EQUAL(NULL, get_filter);

    //test missing opening brackets in AND comparator
    //mock().expectNCalls(3, "framework_log");
    const char *filter_str2 = "(&test_attr1=attr1|(test_attr2=attr2)(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str2);
    POINTERS_EQUAL(NULL, get_filter);

    //test missing opening brackets in AND comparator
    //mock().expectNCalls(4, "framework_log");
    const char *filter_str3 = "(&(test_attr1=attr1)(|test_attr2=attr2(test_attr3=attr3))";
    get_filter = celix_filter_create(filter_str3);
    POINTERS_EQUAL(NULL, get_filter);

    //test missing opening brackets in NOT comparator
    //mock().expectNCalls(4, "framework_log");
    const char *filter_str4 = "(&(test_attr1=attr1)(!test_attr2=attr2)";
    get_filter = celix_filter_create(filter_str4);
    POINTERS_EQUAL(NULL, get_filter);
}

TEST(filter, create_fail_missing_closing_brackets){
    char * filter_str;
    celix_filter_t * get_filter;
    //test missing closing brackets in substring
    filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3");
    get_filter = celix_filter_create(filter_str);
    POINTERS_EQUAL(NULL, get_filter);
    free(filter_str);

    //test missing closing brackets in value
    filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=attr3");
    get_filter = celix_filter_create(filter_str);
    POINTERS_EQUAL(NULL, get_filter);
    free(filter_str);
}

TEST(filter, create_fail_invalid_closing_brackets){
    char * filter_str;
    celix_filter_t * get_filter;

    //test missing closing brackets in substring
    //mock().expectNCalls(6, "framework_log");
    filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=at(tr3)))");
    get_filter = celix_filter_create(filter_str);
    POINTERS_EQUAL(NULL, get_filter);
    free(filter_str);

    //test missing closing brackets in value
    //mock().expectNCalls(5, "framework_log");
    filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3>=att(r3)))");
    get_filter = celix_filter_create(filter_str);
    POINTERS_EQUAL(NULL, get_filter);
    free(filter_str);
}

TEST(filter, create_misc){
    celix_filter_t * get_filter;

    //test trailing chars
    //mock().expectOneCall("framework_log");
    const char *filter_str1 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3))) oh no! trailing chars";
    get_filter = celix_filter_create(filter_str1);
    POINTERS_EQUAL(NULL, get_filter);

    //test half APPROX operator (should be "~=", instead is "~")
    //mock().expectNCalls(5, "framework_log");
    const char* filter_str2 = "(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3~attr3)))";
    get_filter = celix_filter_create(filter_str2);
    POINTERS_EQUAL(NULL, get_filter);

    //test PRESENT operator with trailing chars (should just register as substrings: "*" and "attr3")
    const char *filter_str3 = "(test_attr3=*attr3)";
    get_filter = celix_filter_create(filter_str3);
    CHECK(get_filter != NULL);
    LONGS_EQUAL(CELIX_FILTER_OPERAND_SUBSTRING, get_filter->operand);
    LONGS_EQUAL(2, celix_arrayList_size((celix_array_list_t*) get_filter->children));
    celix_filter_destroy(get_filter);

    //test parsing a attribute of 0 length
    //mock().expectNCalls(3, "framework_log");
    const char* filter_str4 = "(>=attr3)";
    get_filter = celix_filter_create(filter_str4);
    POINTERS_EQUAL(NULL, get_filter);

    //test parsing a value of 0 length
    //mock().expectOneCall("framework_log");
    const char* filter_str5 = "(test_attr3>=)";
    get_filter = celix_filter_create(filter_str5);
    POINTERS_EQUAL(NULL, get_filter);

    //test parsing a value with a escaped closing bracket "\)"
    const char* filter_str6 = "(test_attr3>=strWith\\)inIt)";
    get_filter = celix_filter_create(filter_str6);
    CHECK(get_filter != NULL);
    STRCMP_EQUAL("strWith)inIt", (char*)get_filter->value);
    celix_filter_destroy(get_filter);

    //test parsing a substring with a escaped closing bracket "\)"
    const char *filter_str7 = "(test_attr3=strWith\\)inIt)";
    get_filter = celix_filter_create(filter_str7);
    CHECK(get_filter != NULL);
    STRCMP_EQUAL("strWith)inIt", (char*)get_filter->value);
    celix_filter_destroy(get_filter);
}

TEST(filter, match_comparators){
    char * filter_str;
    celix_filter_t * filter;
    celix_properties_t *props = celix_properties_create();
    char * key = my_strdup("test_attr1");
    char * val = my_strdup("attr1");
    char * key2 = my_strdup("test_attr2");
    char * val2 = my_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    //test AND
    filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(!(test_attr3=attr3))))");
    filter = celix_filter_create(filter_str);
    bool result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test AND false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(&(test_attr1=attr1)(test_attr1=attr2))");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);

}

TEST(filter, match_operators){
    char * filter_str;
    celix_filter_t * filter;
    celix_properties_t *props = celix_properties_create();
    char * key = my_strdup("test_attr1");
    char * val = my_strdup("attr1");
    char * key2 = my_strdup("test_attr2");
    char * val2 = my_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    //test EQUALS
    filter_str = my_strdup("(test_attr1=attr1)");
    filter = celix_filter_create(filter_str);
    bool result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test EQUALS false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1=falseString)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test APPROX TODO: update this test once APPROX is implemented
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1~=attr1)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test APROX false TODO: update this test once APPROX is implemented
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1~=ATTR1)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1=*)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test NOT PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr3=*)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test NOT PRESENT
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(!(test_attr3=*))");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK_TRUE(result);

    //test LESSEQUAL less
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1<=attr5)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test LESSEQUAL equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2<=attr2)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test LESSEQUAL false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2<=attr1)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test GREATEREQUAL greater
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2>=attr1)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test GREATEREQUAL equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2>=attr2)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test GREATEREQUAL false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1>=attr5)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test LESS less
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1<attr5)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test LESS equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2<attr2)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test LESS false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2<attr1)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test GREATER greater
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2>attr1)");
    filter = celix_filter_create(filter_str);
    result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //test GREATER equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr2>attr2)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test GREATER false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1>attr5)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //test SUBSTRING equals
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1=attr*)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK(result);

    //test SUBSTRING false
    celix_filter_destroy(filter);
    free(filter_str);
    filter_str = my_strdup("(test_attr1=attr*charsNotPresent)");
    filter = celix_filter_create(filter_str);
    result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);

}

TEST(filter, match_recursion){

    char * filter_str = my_strdup("(&(test_attr1=attr1)(|(&(test_attr2=attr2)(!(&(test_attr1=attr1)(test_attr3=attr3))))(test_attr3=attr3)))");
    celix_filter_t * filter = celix_filter_create(filter_str);
    celix_properties_t *props = celix_properties_create();
    char * key = my_strdup("test_attr1");
    char * val = my_strdup("attr1");
    char * key2 = my_strdup("test_attr2");
    char * val2 = my_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    bool result = false;
    filter_match(filter, props, &result);
    CHECK(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);

}

TEST(filter, match_false){
    char * filter_str = my_strdup("(&(test_attr1=attr1)(&(test_attr2=attr2)(test_attr3=attr3)))");
    celix_filter_t * filter = celix_filter_create(filter_str);
    celix_properties_t *props = celix_properties_create();
    char * key = my_strdup("test_attr1");
    char * val = my_strdup("attr1");
    char * key2 = my_strdup("test_attr2");
    char * val2 = my_strdup("attr2");
    celix_properties_set(props, key, val);
    celix_properties_set(props, key2, val2);

    bool result = true;
    filter_match(filter, props, &result);
    CHECK_FALSE(result);

    //cleanup
    celix_properties_destroy(props);
    celix_filter_destroy(filter);
    free(filter_str);
    free(key);
    free(key2);
    free(val);
    free(val2);

}

TEST(filter, match_filter){

    celix_filter_t *filter = celix_filter_create("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))");
    celix_filter_t *compareTo = celix_filter_create("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))");

    bool result;
    filter_match_filter(filter, compareTo, &result); //equal same order
    CHECK_TRUE(result);
    //cleanup
    celix_filter_destroy(filter);
    celix_filter_destroy(compareTo);

    filter = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2)(test_attr3=attr3))");
    compareTo = celix_filter_create("(&(test_attr3=attr3)(test_attr2=attr2)(test_attr1=attr1))");
    CHECK(filter != NULL);
    CHECK(compareTo != NULL);
    filter_match_filter(filter, compareTo, &result); //equal not same order
    CHECK_TRUE(result);
    //cleanup
    celix_filter_destroy(filter);
    celix_filter_destroy(compareTo);

    filter = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2)(test_attr3=attr3))");
    compareTo = celix_filter_create("(&(test_attr1=attr1)(test_attr2=attr2)(test_attr4=attr4))");
    CHECK(filter != NULL);
    CHECK(compareTo != NULL);
    filter_match_filter(filter, compareTo, &result); //almost, but not equal
    CHECK_FALSE(result);
    //cleanup
    celix_filter_destroy(filter);
    celix_filter_destroy(compareTo);

    filter_match_filter(NULL, NULL, &result); //both null  -> equal
    CHECK_TRUE(result);

    filter = celix_filter_create("(attr1=)");
    filter_match_filter(filter, NULL, &result); //one null  -> not equal
    CHECK_FALSE(result);

    filter_match_filter(NULL, filter, &result); //one null  -> not equal
    CHECK_FALSE(result);

    celix_filter_destroy(filter);
}

TEST(filter, getString){
    char * filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))");
    celix_filter_t * filter = celix_filter_create(filter_str);

    const char * get_str;
    filter_getString(filter, &get_str);

    //cleanup
    celix_filter_destroy(filter);
    free(filter_str);
}


