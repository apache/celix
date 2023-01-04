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

TEST_F(PropertiesTestSuite, getSetWithNULL) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, nullptr, "value");
    EXPECT_EQ(celix_properties_size(properties), 0); //NULL key will be ignored

    celix_properties_set(properties, nullptr, nullptr);
    EXPECT_EQ(celix_properties_size(properties), 0); //NULL key will be ignored

    celix_properties_set(properties, "key", nullptr);
    EXPECT_EQ(celix_properties_size(properties), 0); //NULL value will be ignored

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

TEST_F(PropertiesTestSuite, fillTest) {
    celix_properties_t *props = celix_properties_create();
    int testCount = 1000;
    for (int i = 0; i < 1000; ++i) {
        char k[5];
        snprintf(k, sizeof(k), "%i", i);
        const char* v = "a";
        celix_properties_set(props, k, v);
    }
    EXPECT_EQ(celix_properties_size(props), testCount);
    celix_properties_destroy(props);
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

TEST_F(PropertiesTestSuite, getType) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "string", "value");
    celix_properties_setLong(props, "long", 123);
    celix_properties_setDouble(props, "double", 3.14);
    celix_properties_setBool(props, "bool", true);
    auto* version = celix_version_createVersion(1, 2, 3, nullptr);
    celix_properties_setVersion(props, "version", version);

    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, celix_properties_getType(props, "string"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, celix_properties_getType(props, "long"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(props, "double"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, celix_properties_getType(props, "bool"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "version"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, celix_properties_getType(props, "missing"));

    celix_version_destroy(version);
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, getEntry) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_setLong(props, "key2", 123);
    celix_properties_setDouble(props, "key3", 123.456);
    celix_properties_setBool(props, "key4", true);
    auto* version = celix_version_createVersion(1, 2, 3, nullptr);
    celix_properties_setVersion(props, "key5", version);

    auto entry = celix_properties_getEntry(props, "key1");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, entry.valueType);
    EXPECT_STREQ("value1", entry.value);
    EXPECT_STREQ("value1", entry.typed.strValue);

    entry = celix_properties_getEntry(props, "key2");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, entry.valueType);
    EXPECT_STREQ("123", entry.value);
    EXPECT_EQ(123, entry.typed.longValue);

    entry = celix_properties_getEntry(props, "key3");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, entry.valueType);
    EXPECT_NE(strstr(entry.value, "123.456"), nullptr);
    EXPECT_DOUBLE_EQ(123.456, entry.typed.doubleValue);

    entry = celix_properties_getEntry(props, "key4");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, entry.valueType);
    EXPECT_STREQ("true", entry.value);
    EXPECT_TRUE(entry.typed.boolValue);

    entry = celix_properties_getEntry(props, "key5");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, entry.valueType);
    EXPECT_STREQ("1.2.3", entry.value);
    EXPECT_EQ(1, celix_version_getMajor(entry.typed.versionValue));
    EXPECT_EQ(2, celix_version_getMinor(entry.typed.versionValue));
    EXPECT_EQ(3, celix_version_getMicro(entry.typed.versionValue));

    entry = celix_properties_getEntry(props, "key6");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, entry.valueType);

    celix_version_destroy(version);
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, iteratorNextKey) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "key3", "value3");
    auto iter = celix_propertiesIterator_construct(props);
    const char* key;
    int count = 0;
    while (celix_propertiesIterator_hasNext(&iter)) {
        key = celix_propertiesIterator_nextKey(&iter);
        EXPECT_NE(strstr(key, "key"), nullptr);
        count++;
    }
    EXPECT_EQ(count, 3);
    key = celix_propertiesIterator_nextKey(&iter);
    EXPECT_EQ(nullptr, key) << "got key: " << key;

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, iteratorNext) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "key3", "value3");

    int count = 0;
    auto iter = celix_properties_begin(props);
    while (!celix_propertiesIterator_isEnd(&iter)) {
        EXPECT_NE(strstr(iter.entry.key, "key"), nullptr);
        EXPECT_NE(strstr(iter.entry.value, "value"), nullptr);
        count++;
        celix_propertiesIterator_next(&iter);
    }
    EXPECT_EQ(count, 3);
    celix_propertiesIterator_next(&iter);
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, iter.entry.valueType);

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, iterateOverProperties) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");

    int outerCount = 0;
    int innerCount = 0;
    CELIX_PROPERTIES_ITERATE(props, outerIter) {
        outerCount++;
        EXPECT_NE(strstr(outerIter.entry.key, "key"), nullptr);

        // Inner loop to test nested iteration
        CELIX_PROPERTIES_ITERATE(props, innerIter) {
            innerCount++;
            EXPECT_NE(strstr(innerIter.entry.key, "key"), nullptr);
        }
    }
    // Check that both entries were iterated over
    EXPECT_EQ(outerCount, 2);
    EXPECT_EQ(innerCount, 4);

    celix_properties_destroy(props);


    props = celix_properties_create();
    int count = 0;
    CELIX_PROPERTIES_ITERATE(props, outerIter) {
        count++;
    }
    EXPECT_EQ(count, 0);
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, getVersion) {
    auto* properties = celix_properties_create();
    auto* emptyVersion = celix_version_createEmptyVersion();

    // Test getting a version property
    auto* expected = celix_version_createVersion(1, 2, 3, "test");
    celix_properties_setVersion(properties, "key", expected);
    const auto* actual = celix_properties_getVersion(properties, "key", nullptr);
    EXPECT_EQ(celix_version_getMajor(expected), celix_version_getMajor(actual));
    EXPECT_EQ(celix_version_getMinor(expected), celix_version_getMinor(actual));
    EXPECT_EQ(celix_version_getMicro(expected), celix_version_getMicro(actual));
    EXPECT_STREQ(celix_version_getQualifier(expected), celix_version_getQualifier(actual));

    // Test getting a non-version property
    celix_properties_set(properties, "key2", "value");
    actual = celix_properties_getVersion(properties, "key2", emptyVersion);
    EXPECT_EQ(celix_version_getMajor(actual), 0);
    EXPECT_EQ(celix_version_getMinor(actual), 0);
    EXPECT_EQ(celix_version_getMicro(actual), 0);
    EXPECT_STREQ(celix_version_getQualifier(actual), "");
    EXPECT_EQ(celix_properties_getVersion(properties, "non-existent", nullptr), nullptr);
    celix_version_destroy(expected);

    // Test setting without copy
    celix_properties_setVersionWithoutCopy(properties, "key3", celix_version_createVersion(3,3,3,""));
    actual = celix_properties_getVersion(properties, "key3", emptyVersion);
    EXPECT_EQ(celix_version_getMajor(actual), 3);
    EXPECT_EQ(celix_version_getMinor(actual), 3);
    EXPECT_EQ(celix_version_getMicro(actual), 3);
    EXPECT_STREQ(celix_version_getQualifier(actual), "");

    celix_version_destroy(emptyVersion);
    celix_properties_destroy(properties);
}
