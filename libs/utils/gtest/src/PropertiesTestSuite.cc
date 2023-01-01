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

#include "celix_properties.h"

using ::testing::MatchesRegex;

class PropertiesTestSuite : public ::testing::Test {
public:
};


TEST_F(PropertiesTestSuite, create) {
    auto* properties = celix_properties_create();
    EXPECT_TRUE(properties);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, load) {
    char propertiesFile[] = "resources-test/properties.txt";
    auto* properties = celix_properties_load(propertiesFile);
    EXPECT_EQ(4, celix_properties_size(properties));

    const char keyA[] = "a";
    const char *valueA = celix_properties_get(properties, keyA, nullptr);
    EXPECT_STREQ("b", valueA);

    const char keyNiceA[] = "nice_a";
    const char *valueNiceA = celix_properties_get(properties, keyNiceA, nullptr);
    EXPECT_STREQ("nice_b", valueNiceA);

    const char keyB[] = "b";
    const char *valueB = celix_properties_get(properties, keyB, nullptr);
    EXPECT_STREQ("c \t d", valueB);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, asLong) {
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, "t1", "42");
    celix_properties_set(props, "t2", "-42");
    celix_properties_set(props, "t3", "");
    celix_properties_set(props, "t4", "42 bla"); //converts to 42
    celix_properties_set(props, "t5", "bla");

    long v = celix_properties_getAsLong(props, "t1", -1);
    EXPECT_EQ(42, v);

    v = celix_properties_getAsLong(props, "t2", -1);
    EXPECT_EQ(-42, v);

    v = celix_properties_getAsLong(props, "t3", -1);
    EXPECT_EQ(-1, v);

    v = celix_properties_getAsLong(props, "t4", -1);
    EXPECT_EQ(42, v);

    v = celix_properties_getAsLong(props, "t5", -1);
    EXPECT_EQ(-1, v);

    v = celix_properties_getAsLong(props, "non-existing", -1);
    EXPECT_EQ(-1, v);

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, store) {
    char propertiesFile[] = "resources-test/properties_out.txt";
    auto* properties = celix_properties_create();
    char keyA[] = "x";
    char keyB[] = "y";
    char valueA[] = "1";
    char valueB[] = "2";
    celix_properties_set(properties, keyA, valueA);
    celix_properties_set(properties, keyB, valueB);
    celix_properties_store(properties, propertiesFile, nullptr);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, copy) {
    char propertiesFile[] = "resources-test/properties.txt";
    auto* properties = celix_properties_load(propertiesFile);
    EXPECT_EQ(4, celix_properties_size(properties));

    celix_properties_t *copy = celix_properties_copy(properties);

    char keyA[] = "a";
    const char *valueA = celix_properties_get(copy, keyA, nullptr);
    EXPECT_STREQ("b", valueA);
    const char keyB[] = "b";
    EXPECT_STREQ("c \t d", celix_properties_get(copy, keyB, nullptr));

    celix_properties_destroy(properties);
    celix_properties_destroy(copy);
}

TEST_F(PropertiesTestSuite, getSet) {
    auto* properties = celix_properties_create();
    char keyA[] = "x";
    char keyB[] = "y";
    char keyC[] = "z";
    char *keyD = strndup("a", 1);
    char valueA[] = "1";
    char valueB[] = "2";
    char valueC[] = "3";
    char *valueD = strndup("4", 1);
    celix_properties_set(properties, keyA, valueA);
    celix_properties_set(properties, keyB, valueB);
    celix_properties_setWithoutCopy(properties, keyD, valueD);

    EXPECT_STREQ(valueA, celix_properties_get(properties, keyA, nullptr));
    EXPECT_STREQ(valueB, celix_properties_get(properties, keyB, nullptr));
    EXPECT_STREQ(valueC, celix_properties_get(properties, keyC, valueC));
    EXPECT_STREQ(valueD, celix_properties_get(properties, keyD, nullptr));

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, setUnset) {
    auto* properties = celix_properties_create();
    char keyA[] = "x";
    char *keyD = strndup("a", 1);
    char valueA[] = "1";
    char *valueD = strndup("4", 1);
    celix_properties_set(properties, keyA, valueA);
    celix_properties_setWithoutCopy(properties, keyD, valueD);
    EXPECT_STREQ(valueA, celix_properties_get(properties, keyA, nullptr));
    EXPECT_STREQ(valueD, celix_properties_get(properties, keyD, nullptr));

    celix_properties_unset(properties, keyA);
    celix_properties_unset(properties, keyD);
    EXPECT_EQ(nullptr, celix_properties_get(properties, keyA, nullptr));
    EXPECT_EQ(nullptr, celix_properties_get(properties, "a", nullptr));
    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, longTest) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, "a", "2");
    celix_properties_set(properties, "b", "-10032L");
    celix_properties_set(properties, "c", "");
    celix_properties_set(properties, "d", "garbage");

    long a = celix_properties_getAsLong(properties, "a", -1L);
    long b = celix_properties_getAsLong(properties, "b", -1L);
    long c = celix_properties_getAsLong(properties, "c", -1L);
    long d = celix_properties_getAsLong(properties, "d", -1L);
    long e = celix_properties_getAsLong(properties, "e", -1L);

    EXPECT_EQ(2, a);
    EXPECT_EQ(-10032L, b);
    EXPECT_EQ(-1L, c);
    EXPECT_EQ(-1L, d);
    EXPECT_EQ(-1L, e);

    celix_properties_setLong(properties, "a", 3L);
    celix_properties_setLong(properties, "b", -4L);
    a = celix_properties_getAsLong(properties, "a", -1L);
    b = celix_properties_getAsLong(properties, "b", -1L);
    EXPECT_EQ(3L, a);
    EXPECT_EQ(-4L, b);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, boolTest) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, "a", "true");
    celix_properties_set(properties, "b", "false");
    celix_properties_set(properties, "c", "  true  ");
    celix_properties_set(properties, "d", "garbage");

    bool a = celix_properties_getAsBool(properties, "a", false);
    bool b = celix_properties_getAsBool(properties, "b", true);
    bool c = celix_properties_getAsBool(properties, "c", false);
    bool d = celix_properties_getAsBool(properties, "d", true);
    bool e = celix_properties_getAsBool(properties, "e", false);
    bool f = celix_properties_getAsBool(properties, "f", true);

    EXPECT_EQ(true, a);
    EXPECT_EQ(false, b);
    EXPECT_EQ(true, c);
    EXPECT_EQ(true, d);
    EXPECT_EQ(false, e);
    EXPECT_EQ(true, f);

    celix_properties_setBool(properties, "a", true);
    celix_properties_setBool(properties, "b", false);
    a = celix_properties_getAsBool(properties, "a", false);
    b = celix_properties_getAsBool(properties, "b", true);
    EXPECT_EQ(true, a);
    EXPECT_EQ(false, b);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, sizeAndIteratorTest) {
    celix_properties_t *props = celix_properties_create();
    EXPECT_EQ(0, celix_properties_size(props));
    celix_properties_set(props, "a", "1");
    celix_properties_set(props, "b", "2");
    EXPECT_EQ(2, celix_properties_size(props));
    celix_properties_set(props, "c", "  3  ");
    celix_properties_set(props, "d", "4");
    EXPECT_EQ(4, celix_properties_size(props));

    int count = 0;
    const char *key;
    CELIX_PROPERTIES_FOR_EACH(props, key) {
        EXPECT_NE(key, nullptr);
        count++;
    }
    EXPECT_EQ(4, count);

    celix_properties_destroy(props);
}
