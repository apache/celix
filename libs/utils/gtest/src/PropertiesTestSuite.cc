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

#include <climits>

#include "celix_err.h"
#include "celix_properties.h"
#include "celix_properties_internal.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"

using ::testing::MatchesRegex;

class PropertiesTestSuite : public ::testing::Test {
  public:
    PropertiesTestSuite() { celix_err_resetErrors(); }

    void printStats(const celix_properties_statistics_t* stats) {
        printf("Properties statistics:\n");
        printf("|- nr of entries: %zu\n", stats->mapStatistics.nrOfEntries);
        printf("|- nr of buckets: %zu\n", stats->mapStatistics.nrOfBuckets);
        printf("|- average nr of entries in bucket: %f\n", stats->mapStatistics.averageNrOfEntriesPerBucket);
        printf("|- stddev nr of entries in bucket: %f\n", stats->mapStatistics.stdDeviationNrOfEntriesPerBucket);
        printf("|- resize count: %zu\n", stats->mapStatistics.resizeCount);
        printf("|- size of keys and string values: %zu bytes\n", stats->sizeOfKeysAndStringValues);
        printf("|- average size of keys and string values: %f bytes\n", stats->averageSizeOfKeysAndStringValues);
        printf("|- fill string optimization buffer percentage: %f\n", stats->fillStringOptimizationBufferPercentage);
        printf("|- fill entries optimization buffer percentage: %f\n", stats->fillEntriesOptimizationBufferPercentage);
    }
};

TEST_F(PropertiesTestSuite, CreateTest) {
    auto* properties = celix_properties_create();
    EXPECT_TRUE(properties);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, LoadTest) {
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

TEST_F(PropertiesTestSuite, LoadFromStringTest) {
    const char* string = "key1=value1\nkey2=value2";
    auto* props = celix_properties_loadFromString(string);
    EXPECT_EQ(2, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_get(props, "key1", ""));
    EXPECT_STREQ("value2", celix_properties_get(props, "key2", ""));
    celix_properties_destroy(props);
}


TEST_F(PropertiesTestSuite, StoreTest) {
    const char* propertiesFile = "resources-test/properties_out.txt";
    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    celix_properties_set(properties, "keyA", "valueA");
    celix_properties_set(properties, "keyB", "valueB");
    celix_properties_store(properties, propertiesFile, nullptr);

    celix_autoptr(celix_properties_t) properties2 = celix_properties_load(propertiesFile);
    EXPECT_EQ(celix_properties_size(properties), celix_properties_size(properties2));
    EXPECT_STREQ(celix_properties_get(properties, "keyA", ""), celix_properties_get(properties2, "keyA", ""));
    EXPECT_STREQ(celix_properties_get(properties, "keyB", ""), celix_properties_get(properties2, "keyB", ""));
}

TEST_F(PropertiesTestSuite, StoreWithHeaderTest) {
    const char* propertiesFile = "resources-test/properties_with_header_out.txt";
    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    celix_properties_set(properties, "keyA", "valueA");
    celix_properties_set(properties, "keyB", "valueB");
    celix_properties_store(properties, propertiesFile, "header");

    celix_autoptr(celix_properties_t) properties2 = celix_properties_load(propertiesFile);
    EXPECT_EQ(celix_properties_size(properties), celix_properties_size(properties2));
    EXPECT_STREQ(celix_properties_get(properties, "keyA", ""), celix_properties_get(properties2, "keyA", ""));
    EXPECT_STREQ(celix_properties_get(properties, "keyB", ""), celix_properties_get(properties2, "keyB", ""));

    //check if provided header text is present in file
    FILE *f = fopen(propertiesFile, "r");
    char line[1024];
    fgets(line, sizeof(line), f);
    EXPECT_STREQ("#header\n", line);
    fclose(f);
}

TEST_F(PropertiesTestSuite, GetAsLongTest) {
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, "t1", "42");
    celix_properties_set(props, "t2", "-42");
    celix_properties_set(props, "t3", "");
    celix_properties_set(props, "t4", "42 bla"); //does not convert to 42
    celix_properties_set(props, "t5", "bla");

    long v = celix_properties_getAsLong(props, "t1", -1);
    EXPECT_EQ(42, v);

    v = celix_properties_getAsLong(props, "t2", -1);
    EXPECT_EQ(-42, v);

    v = celix_properties_getAsLong(props, "t3", -1);
    EXPECT_EQ(-1, v);

    v = celix_properties_getAsLong(props, "t4", -1);
    EXPECT_EQ(-1, v);

    v = celix_properties_getAsLong(props, "t5", -1);
    EXPECT_EQ(-1, v);

    v = celix_properties_getAsLong(props, "non-existing", -1);
    EXPECT_EQ(-1, v);

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, GetSetTest) {
    auto* properties = celix_properties_create();
    char keyA[] = "x";
    char keyB[] = "y";
    char keyC[] = "z";
    char* keyD = strndup("a", 1);
    const char* keyE = "e";
    char valueA[] = "1";
    char valueB[] = "2";
    char valueC[] = "3";
    char *valueD = strndup("4", 1);
    char *valueE = strdup("5");
    celix_properties_set(properties, keyA, valueA);
    celix_properties_set(properties, keyB, valueB);
    celix_properties_assign(properties, keyD, valueD);
    celix_properties_assignString(properties, keyE, valueE);

    EXPECT_STREQ(valueA, celix_properties_get(properties, keyA, nullptr));
    EXPECT_STREQ(valueB, celix_properties_get(properties, keyB, nullptr));
    EXPECT_STREQ(valueC, celix_properties_get(properties, keyC, valueC));
    EXPECT_STREQ(valueD, celix_properties_get(properties, keyD, nullptr));
    EXPECT_STREQ(valueE, celix_properties_get(properties, keyE, nullptr));

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, GetSetWithNullTest) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, nullptr, "value");
    EXPECT_EQ(celix_properties_size(properties), 0); //NULL key will be ignored

    celix_properties_set(properties, nullptr, nullptr);
    EXPECT_EQ(celix_properties_size(properties), 0); //NULL key will be ignored

    celix_properties_set(properties, "key", nullptr); //NULL value will result in empty string value
    EXPECT_STREQ("", celix_properties_get(properties, "key", "not found"));
    EXPECT_EQ(celix_properties_size(properties), 1);

    celix_properties_destroy(properties);
}


TEST_F(PropertiesTestSuite, SetUnsetTest) {
    auto* properties = celix_properties_create();
    char keyA[] = "x";
    char *keyD = strndup("a", 1);
    char valueA[] = "1";
    char *valueD = strndup("4", 1);
    celix_properties_set(properties, keyA, valueA);
    celix_properties_assign(properties, keyD, valueD);
    EXPECT_STREQ(valueA, celix_properties_get(properties, keyA, nullptr));
    EXPECT_STREQ(valueD, celix_properties_get(properties, keyD, nullptr));

    celix_properties_unset(properties, keyA);
    celix_properties_unset(properties, keyD);
    EXPECT_EQ(nullptr, celix_properties_get(properties, keyA, nullptr));
    EXPECT_EQ(nullptr, celix_properties_get(properties, "a", nullptr));
    EXPECT_EQ(0, celix_properties_size(properties));

    celix_properties_set(properties, keyA, nullptr);
    EXPECT_EQ(1, celix_properties_size(properties));
    celix_properties_unset(properties, keyA);
    EXPECT_EQ(0, celix_properties_size(properties));

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, GetLongTest) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, "a", "2");
    celix_properties_set(properties, "b", "-10032");
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

TEST_F(PropertiesTestSuite, GetAsDoubleTest) {
    auto* properties = celix_properties_create();

    celix_properties_set(properties, "a", "2");
    celix_properties_set(properties, "b", "-10032");
    celix_properties_set(properties, "c", "1.2");
    celix_properties_setDouble(properties, "d", 1.4);
    celix_properties_set(properties, "e", "");
    celix_properties_set(properties, "f", "garbage");

    double a = celix_properties_getAsDouble(properties, "a", -1);
    double b = celix_properties_getAsDouble(properties, "b", -1);
    double c = celix_properties_getAsDouble(properties, "c", -1);
    double d = celix_properties_getAsDouble(properties, "d", -1);
    double e = celix_properties_getAsDouble(properties, "e", -1);
    double f = celix_properties_getAsDouble(properties, "f", -1);
    double g = celix_properties_getAsDouble(properties, "g", -1); //does not exist

    EXPECT_EQ(2, a);
    EXPECT_EQ(-10032L, b);
    EXPECT_EQ(1.2, c);
    EXPECT_EQ(1.4, d);
    EXPECT_EQ(-1L, e);
    EXPECT_EQ(-1L, f);
    EXPECT_EQ(-1L, g);

    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, GetBoolTest) {
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

TEST_F(PropertiesTestSuite, GetFillTest) {
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

TEST_F(PropertiesTestSuite, GetSetOverwrite) {
    auto* props = celix_properties_create();
    auto* version = celix_version_createEmptyVersion();

    {
        char* key = celix_utils_strdup("key");
        celix_properties_set(props, key, "str1");
        free(key);
    }
    EXPECT_STREQ("str1", celix_properties_get(props, "key", ""));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_set(props, "key", "str2"));
    EXPECT_STREQ("str2", celix_properties_get(props, "key", ""));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setLong(props, "key", 1));
    EXPECT_EQ(1, celix_properties_getAsLong(props, "key", -1L));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setDouble(props, "key", 2.0));
    EXPECT_EQ(2.0, celix_properties_getAsLong(props, "key", -2.0));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setBool(props, "key", false));
    EXPECT_EQ(false, celix_properties_getAsBool(props, "key", true));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_assignVersion(props, "key", version));
    EXPECT_EQ(version, celix_properties_getVersion(props, "key"));
    celix_properties_set(props, "key", "last");

    celix_properties_destroy(props);
}



TEST_F(PropertiesTestSuite, SizeAndIteratorTest) {
    celix_properties_t *props = celix_properties_create();
    EXPECT_EQ(0, celix_properties_size(props));
    celix_properties_set(props, "a", "1");
    celix_properties_set(props, "b", "2");
    EXPECT_EQ(2, celix_properties_size(props));
    celix_properties_set(props, "c", "  3  ");
    celix_properties_set(props, "d", "4");
    EXPECT_EQ(4, celix_properties_size(props));

    int count = 0;
    CELIX_PROPERTIES_ITERATE(props, entry) {
        EXPECT_NE(entry.key, nullptr);
        count++;
    }
    EXPECT_EQ(4, count);

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, GetTypeAndCopyTest) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "string", "value");
    celix_properties_setLong(props, "long", 123);
    celix_properties_setDouble(props, "double", 3.14);
    celix_properties_setBool(props, "bool", true);
    auto* version = celix_version_create(1, 2, 3, nullptr);
    celix_properties_setVersion(props, "version", version);
    celix_autoptr(celix_array_list_t) longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 1);
    celix_arrayList_addLong(longList, 2);
    celix_arrayList_addLong(longList, 3);
    celix_properties_setArrayList(props, "array", longList);

    EXPECT_EQ(6, celix_properties_size(props));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, celix_properties_getType(props, "string"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, celix_properties_getType(props, "long"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(props, "double"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, celix_properties_getType(props, "bool"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "version"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, celix_properties_getType(props, "missing"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST, celix_properties_getType(props, "array"));
    EXPECT_TRUE(celix_arrayList_equals(longList, celix_properties_getArrayList(props, "array")));

    auto* copy = celix_properties_copy(props);
    EXPECT_EQ(6, celix_properties_size(copy));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, celix_properties_getType(copy, "string"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, celix_properties_getType(copy, "long"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(copy, "double"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, celix_properties_getType(copy, "bool"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(copy, "version"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST, celix_properties_getType(copy, "array"));
    EXPECT_TRUE(celix_arrayList_equals(longList, celix_properties_getArrayList(copy, "array")));

    celix_version_destroy(version);
    celix_properties_destroy(props);
    celix_properties_destroy(copy);
}

TEST_F(PropertiesTestSuite, GetEntryTest) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_setLong(props, "key2", 123);
    celix_properties_setDouble(props, "key3", 123.456);
    celix_properties_setBool(props, "key4", true);
    auto* version = celix_version_create(1, 2, 3, nullptr);
    celix_properties_setVersion(props, "key5", version);

    auto* entry = celix_properties_getEntry(props, "key1");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, entry->valueType);
    EXPECT_STREQ("value1", entry->value);
    EXPECT_STREQ("value1", entry->typed.strValue);

    entry = celix_properties_getEntry(props, "key2");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, entry->valueType);
    EXPECT_STREQ("123", entry->value);
    EXPECT_EQ(123, entry->typed.longValue);

    entry = celix_properties_getEntry(props, "key3");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, entry->valueType);
    EXPECT_NE(strstr(entry->value, "123.456"), nullptr);
    EXPECT_DOUBLE_EQ(123.456, entry->typed.doubleValue);

    entry = celix_properties_getEntry(props, "key4");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, entry->valueType);
    EXPECT_STREQ("true", entry->value);
    EXPECT_TRUE(entry->typed.boolValue);

    entry = celix_properties_getEntry(props, "key5");
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, entry->valueType);
    EXPECT_STREQ("1.2.3", entry->value);
    EXPECT_EQ(1, celix_version_getMajor(entry->typed.versionValue));
    EXPECT_EQ(2, celix_version_getMinor(entry->typed.versionValue));
    EXPECT_EQ(3, celix_version_getMicro(entry->typed.versionValue));

    entry = celix_properties_getEntry(props, "key6");
    EXPECT_EQ(nullptr, entry);

    celix_version_destroy(version);
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, IteratorNextTest) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "key3", "value3");

    int count = 0;
    auto iter = celix_properties_begin(props);
    while (!celix_propertiesIterator_isEnd(&iter)) {
        EXPECT_NE(strstr(iter.key, "key"), nullptr);
        EXPECT_NE(strstr(iter.entry.value, "value"), nullptr);
        count++;
        celix_propertiesIterator_next(&iter);
    }
    EXPECT_EQ(count, 3);
    celix_propertiesIterator_next(&iter);
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, iter.entry.valueType);

    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, IterateOverPropertiesTest) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");

    int outerCount = 0;
    int innerCount = 0;
    CELIX_PROPERTIES_ITERATE(props, outerIter) {
        outerCount++;
        EXPECT_NE(strstr(outerIter.key, "key"), nullptr);

        // Inner loop to test nested iteration
        CELIX_PROPERTIES_ITERATE(props, innerIter) {
            innerCount++;
            EXPECT_NE(strstr(innerIter.key, "key"), nullptr);
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

TEST_F(PropertiesTestSuite, GetVersionTest) {
    auto* properties = celix_properties_create();
    auto* emptyVersion = celix_version_createEmptyVersion();

    // Test getting a version property
    auto* expected = celix_version_create(1, 2, 3, "test");
    celix_properties_setVersion(properties, "key", expected);
    const auto* actual = celix_properties_getVersion(properties, "key");
    EXPECT_EQ(celix_version_getMajor(expected), celix_version_getMajor(actual));
    EXPECT_EQ(celix_version_getMinor(expected), celix_version_getMinor(actual));
    EXPECT_EQ(celix_version_getMicro(expected), celix_version_getMicro(actual));
    EXPECT_STREQ(celix_version_getQualifier(expected), celix_version_getQualifier(actual));

    // Test getting a non-version property
    celix_properties_set(properties, "key2", "value");
    actual = celix_properties_getVersion(properties, "key2");
    EXPECT_EQ(nullptr, actual);
    actual = celix_properties_getVersion(properties, "non-existent");
    EXPECT_EQ(nullptr, actual);
    celix_version_destroy(expected);

    // Test setting without copy
    celix_properties_assignVersion(properties, "key3", celix_version_create(3, 3, 3, ""));
    actual = celix_properties_getVersion(properties, "key3");
    EXPECT_EQ(celix_version_getMajor(actual), 3);
    EXPECT_EQ(celix_version_getMinor(actual), 3);
    EXPECT_EQ(celix_version_getMicro(actual), 3);
    EXPECT_STREQ(celix_version_getQualifier(actual), "");


    // Test getAsVersion
    celix_properties_set(properties, "string_version", "1.1.1");
    celix_version_t* ver1;
    celix_version_t* ver2;
    celix_version_t* ver3;
    celix_version_t* ver4;
    celix_status_t status = celix_properties_getAsVersion(properties, "non-existing", emptyVersion, &ver1);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_properties_getAsVersion(properties, "non-existing", nullptr, &ver2);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_properties_getAsVersion(properties, "string_version", emptyVersion, &ver3);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_properties_getAsVersion(properties, "key", emptyVersion, &ver4);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(ver1, nullptr);
    EXPECT_EQ(ver2, nullptr);
    EXPECT_EQ(celix_version_getMajor(ver3), 1);
    EXPECT_EQ(celix_version_getMinor(ver3), 1);
    EXPECT_EQ(celix_version_getMicro(ver3), 1);
    EXPECT_EQ(celix_version_getMajor(ver4), 1);
    EXPECT_EQ(celix_version_getMinor(ver4), 2);
    EXPECT_EQ(celix_version_getMicro(ver4), 3);
    celix_version_destroy(ver1);
    celix_version_destroy(ver3);
    celix_version_destroy(ver4);

    celix_version_destroy(emptyVersion);
    celix_properties_destroy(properties);
}

TEST_F(PropertiesTestSuite, EndOfPropertiesTest) {
    auto* props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_iterator_t endIter = celix_properties_end(props);
    EXPECT_TRUE(celix_propertiesIterator_isEnd(&endIter));
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, EndOfEmptyPropertiesTest) {
    auto* props = celix_properties_create();
    celix_properties_iterator_t endIter = celix_properties_end(props);
    EXPECT_TRUE(celix_propertiesIterator_isEnd(&endIter));
    celix_properties_iterator_t beginIter = celix_properties_begin(props);
    EXPECT_TRUE(celix_propertiesIterator_isEnd(&beginIter)); //empty properties: begin == end
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, SetWithCopyTest) {
    auto* props = celix_properties_create();
    celix_properties_assign(props, celix_utils_strdup("key"), celix_utils_strdup("value2"));
    //replace, should free old value and provided key
    celix_properties_assign(props, celix_utils_strdup("key"), celix_utils_strdup("value2"));
    EXPECT_EQ(1, celix_properties_size(props));
    celix_properties_destroy(props);

    EXPECT_EQ(CELIX_SUCCESS, celix_properties_assign(nullptr, celix_utils_strdup("key"), celix_utils_strdup("value2")));
}

TEST_F(PropertiesTestSuite, HasKeyTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    EXPECT_FALSE(celix_properties_hasKey(props, "strKey"));
    celix_properties_set(props, "strKey", "value");
    EXPECT_TRUE(celix_properties_hasKey(props, "strKey"));
    celix_properties_unset(props, "strKey");
    EXPECT_FALSE(celix_properties_hasKey(props, "strKey"));

    EXPECT_FALSE(celix_properties_hasKey(props, "longKey"));
    celix_properties_setLong(props, "longKey", 42L);
    EXPECT_TRUE(celix_properties_hasKey(props, "longKey"));
    celix_properties_unset(props, "longKey");
    EXPECT_FALSE(celix_properties_hasKey(props, "longKey"));
}

TEST_F(PropertiesTestSuite, SetEntryTest) {
    auto* props1 = celix_properties_create();
    auto* props2 = celix_properties_create();
    celix_properties_setString(props1, "key1", "value1");
    celix_properties_setLong(props1, "key2", 123);
    celix_properties_setBool(props1, "key3", true);
    celix_properties_setDouble(props1, "key4", 3.14);
    auto* version = celix_version_create(1, 2, 3, nullptr);
    celix_properties_setVersion(props1, "key5", version);
    celix_version_destroy(version);
    celix_autoptr(celix_array_list_t) longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 1);
    celix_arrayList_addLong(longList, 2);
    celix_arrayList_addLong(longList, 3);
    celix_properties_setArrayList(props1, "key6", longList);

    CELIX_PROPERTIES_ITERATE(props1, visit) {
        celix_properties_setEntry(props2, visit.key, &visit.entry);
        celix_properties_setEntry(nullptr, visit.key, &visit.entry);
    }
    celix_properties_setEntry(props2, "key7", nullptr);
    EXPECT_EQ(6, celix_properties_size(props2));
    EXPECT_STREQ("value1", celix_properties_getAsString(props2, "key1", nullptr));
    EXPECT_EQ(123, celix_properties_getAsLong(props2, "key2", -1L));
    EXPECT_EQ(true, celix_properties_getAsBool(props2, "key3", false));
    EXPECT_EQ(3.14, celix_properties_getAsDouble(props2, "key4", -1.0));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props2, "key5"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST, celix_properties_getType(props2, "key6"));
    EXPECT_TRUE(celix_properties_equals(props1, props2));

    celix_properties_destroy(props1);
    celix_properties_destroy(props2);
}

TEST_F(PropertiesTestSuite, SetEntryWithLargeStringValueTest) {
    //Test if the version and double with a large string representation are correctly set
    //(whitebox test to check if the fallback to string allocation works)
    celix_autoptr(celix_properties_t) props1 = celix_properties_create();

    double doubleVal = 1.23456789E+307;
    celix_properties_setDouble(props1, "key1", doubleVal);
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(props1, "key1"));
    EXPECT_DOUBLE_EQ(doubleVal, celix_properties_getAsDouble(props1, "key1", 0.0));

    celix_autoptr(celix_version_t) version =
        celix_version_create(1, 2, 3, "a-qualifier-that-is-longer-than-20-characters");
    celix_properties_setVersion(props1, "key2", version);
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props1, "key2"));
    EXPECT_EQ(0, celix_version_compareTo(version, celix_properties_getVersion(props1, "key2")));
}


TEST_F(PropertiesTestSuite, PropertiesAutoCleanupTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
}

TEST_F(PropertiesTestSuite, NullArgumentsTest) {
    auto props = celix_properties_loadWithStream(nullptr);
    EXPECT_EQ(nullptr, props);
}

TEST_F(PropertiesTestSuite, PropertiesEqualsTest) {
    EXPECT_TRUE(celix_properties_equals(nullptr, nullptr));

    celix_autoptr(celix_properties_t) prop1 = celix_properties_create();
    EXPECT_FALSE(celix_properties_equals(prop1, nullptr));
    EXPECT_TRUE(celix_properties_equals(prop1, prop1));

    celix_autoptr(celix_properties_t) prop2 = celix_properties_create();
    EXPECT_TRUE(celix_properties_equals(prop1, prop2));

    celix_properties_set(prop1, "key1", "value1");
    celix_properties_setLong(prop1, "key2", 42);
    celix_properties_set(prop2, "key1", "value1");
    celix_properties_setLong(prop2, "key2", 42);
    EXPECT_TRUE(celix_properties_equals(prop1, prop2));

    celix_properties_setBool(prop1, "key3", false);
    EXPECT_FALSE(celix_properties_equals(prop1, prop2));
    celix_properties_setBool(prop2, "key3", false);
    EXPECT_TRUE(celix_properties_equals(prop1, prop2));

    celix_properties_assignVersion(prop1, "key4", celix_version_create(1, 2, 3, nullptr));
    EXPECT_FALSE(celix_properties_equals(prop1, prop2));
    celix_properties_assignVersion(prop2, "key4", celix_version_create(1, 2, 3, nullptr));
    EXPECT_TRUE(celix_properties_equals(prop1, prop2));

    celix_properties_setLong(prop1, "key5", 42);
    celix_properties_setDouble(prop2, "key5", 42.0);
    EXPECT_FALSE(celix_properties_equals(prop1, prop2)); //different types
}

TEST_F(PropertiesTestSuite, PropertiesNullArgumentsTest) {
    celix_autoptr(celix_version_t) version = celix_version_create(1,2,3, nullptr);
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();

    //Silently ignore nullptr properties arguments for set* and copy functions
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_set(nullptr, "key", "value"));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setLong(nullptr, "key", 1));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setDouble(nullptr, "key", 1.0));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setBool(nullptr, "key", true));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setVersion(nullptr, "key", version));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_assignVersion(nullptr, "key", celix_version_copy(version)));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setArrayList(nullptr, "key", list));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_assignArrayList(nullptr, "key", celix_arrayList_copy(list)));
    celix_autoptr(celix_properties_t) copy = celix_properties_copy(nullptr);
    EXPECT_NE(nullptr, copy);
}

TEST_F(PropertiesTestSuite, InvalidArgumentsTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_autoptr(celix_version_t) version = celix_version_create(1,2,3, nullptr);

    //Key cannot be nullptr and set functions should fail
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_set(props, nullptr, "value"));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setLong(props, nullptr, 1));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setDouble(props, nullptr, 1.0));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setBool(props, nullptr, true));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setVersion(props, nullptr, version));
    EXPECT_EQ(5, celix_err_getErrorCount());

    celix_err_resetErrors();
    //Set without copy should fail if a key or value is nullptr
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assign(props, nullptr, strdup("value")));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assign(props, strdup("key"), nullptr));
    EXPECT_EQ(2, celix_err_getErrorCount());

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assignString(props, nullptr, strdup("value")));

    celix_autoptr(celix_array_list_t) list1 = celix_arrayList_create();
    celix_autoptr(celix_array_list_t) list2 = celix_arrayList_createStringArray();
    celix_autoptr(celix_array_list_t) list3 = celix_arrayList_createPointerArray();

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, nullptr, list2));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, "list", nullptr));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, "list", list1));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, "list", list3));

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assignArrayList(props, nullptr, celix_steal_ptr(list2)));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assignArrayList(props, "list", nullptr));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assignArrayList(props, "list", celix_steal_ptr(list1)));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_assignArrayList(props, "list", celix_steal_ptr(list3)));
}

TEST_F(PropertiesTestSuite, GetStatsTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    for (int i = 0; i < 200; ++i) {
        celix_properties_set(props, std::to_string(i).c_str(), std::to_string(i).c_str());
    }

    auto stats = celix_properties_getStatistics(props);
    EXPECT_EQ(stats.mapStatistics.nrOfEntries, 200);
    printStats(&stats);
}

TEST_F(PropertiesTestSuite, SetLongMaxMinTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_setLong(props, "max", LONG_MAX));
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_setLong(props, "min", LONG_MIN));
    EXPECT_EQ(LONG_MAX, celix_properties_getAsLong(props, "max", 0));
    EXPECT_EQ(LONG_MIN, celix_properties_getAsLong(props, "min", 0));
}

TEST_F(PropertiesTestSuite, SetDoubleWithLargeStringRepresentationTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_setDouble(props, "large_str_value", 12345678901234567890.1234567890));
}

TEST_F(PropertiesTestSuite, GetLongDoubleBoolVersionAndStringTest) {

    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, "long", 42);
    celix_properties_setDouble(props, "double", 3.14);
    celix_properties_setBool(props, "bool", true);
    celix_properties_set(props, "str", "value");
    celix_version_t* version = celix_version_create(1, 2, 3, nullptr);
    celix_properties_assignVersion(props, "version", version);

    // check if the values are correctly returned
    EXPECT_STREQ("value", celix_properties_getString(props, "str"));
    EXPECT_EQ(42, celix_properties_getLong(props, "long", -1L));
    EXPECT_DOUBLE_EQ(3.14, celix_properties_getDouble(props, "double", -1.0));
    EXPECT_EQ(true, celix_properties_getBool(props, "bool", false));
    EXPECT_EQ(version, celix_properties_getVersion(props, "version"));

    // check if the values are correctly returned if value is not found
    EXPECT_EQ(nullptr, celix_properties_getString(props, "non-existing"));
    EXPECT_EQ(-1L, celix_properties_getLong(props, "non-existing", -1L));
    EXPECT_DOUBLE_EQ(-1.0, celix_properties_getDouble(props, "non-existing", -1.0));
    EXPECT_EQ(false, celix_properties_getBool(props, "non-existing", false));
    EXPECT_EQ(nullptr, celix_properties_getVersion(props, "non-existing"));

    // check if the values are correctly returned if the found value is not of the correct type
    EXPECT_EQ(nullptr, celix_properties_getString(props, "long"));
    EXPECT_EQ(-1L, celix_properties_getLong(props, "str", -1L));
    EXPECT_DOUBLE_EQ(-1.0, celix_properties_getDouble(props, "str", -1.0));
    EXPECT_EQ(false, celix_properties_getBool(props, "str", false));
    EXPECT_EQ(nullptr, celix_properties_getVersion(props, "str"));

    // check if a default ptr is correctly returned if value is not found for string and version
    EXPECT_EQ("default", celix_properties_get(props, "non-existing", "default"));
}

TEST_F(PropertiesTestSuite, LongArrayListTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_array_list_t* longList1 = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList1, 1);
    celix_arrayList_addLong(longList1, 2);
    celix_arrayList_addLong(longList1, 3);
    celix_arrayList_addLong(longList1, 4);
    celix_arrayList_addLong(longList1, 5);

    ASSERT_EQ(CELIX_SUCCESS, celix_properties_assignArrayList(props, "array1", longList1));
    EXPECT_EQ(1, celix_properties_size(props));
    celix_autoptr(celix_array_list_t) retrievedList1;
    celix_status_t status = celix_properties_getAsLongArrayList(props, "array1", nullptr, &retrievedList1);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(retrievedList1 != nullptr);
    EXPECT_EQ(5, celix_arrayList_size(retrievedList1));
    EXPECT_EQ(1, celix_arrayList_getLong(retrievedList1, 0));
    EXPECT_EQ(5, celix_arrayList_getLong(retrievedList1, 4));

    celix_autoptr(celix_array_list_t) longList2 = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList2, 1);
    celix_arrayList_addLong(longList2, 2);
    celix_arrayList_addLong(longList2, 3);

    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setArrayList(props, "array2", longList2));
    EXPECT_EQ(2, celix_properties_size(props));
    celix_autoptr(celix_array_list_t) retrievedList2;
    status = celix_properties_getAsLongArrayList(props, "array2", nullptr, &retrievedList2);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(retrievedList2 != nullptr);
    EXPECT_NE(longList2, retrievedList2);
    EXPECT_EQ(3, celix_arrayList_size(retrievedList2));
    EXPECT_EQ(1, celix_arrayList_getLong(retrievedList2, 0));
    EXPECT_EQ(3, celix_arrayList_getLong(retrievedList2, 2));

    auto* longList3 = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList3, 4);
    celix_arrayList_addLong(longList3, 5);
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_assignArrayList(props, "array3", longList3));
    EXPECT_EQ(3, celix_properties_size(props));
    celix_autoptr(celix_array_list_t) retrievedList3;
    status = celix_properties_getAsLongArrayList(props, "array3", nullptr, &retrievedList3);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(retrievedList3 != nullptr);
    EXPECT_NE(longList3, retrievedList3);
    EXPECT_EQ(2, celix_arrayList_size(retrievedList3));
    EXPECT_EQ(4, celix_arrayList_getLong(retrievedList3, 0));
    EXPECT_EQ(5, celix_arrayList_getLong(retrievedList3, 1));

    celix_array_list* retrievedList4;
    celix_autoptr(celix_array_list_t) defaultList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(defaultList, 6);
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsLongArrayList(props, "non-existing", defaultList, &retrievedList4));
    ASSERT_NE(nullptr, retrievedList4);
    ASSERT_EQ(1, celix_arrayList_size(retrievedList4));
    EXPECT_EQ(6, celix_arrayList_getLong(retrievedList4, 0));
    celix_arrayList_destroy(retrievedList4);

    auto* getList = celix_properties_getLongArrayList(props, "array2");
    EXPECT_NE(longList2, getList);
    getList = celix_properties_getLongArrayList(props, "array3");
    EXPECT_EQ(longList3, getList);
}

TEST_F(PropertiesTestSuite, GetTypedArrayListTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //Given a string, long, double, bool and version array list with 2 elements each
    auto* stringList = celix_arrayList_createStringArray();
    celix_arrayList_addString(stringList, "a");
    celix_arrayList_addString(stringList, "b");
    auto* longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 1);
    celix_arrayList_addLong(longList, 2);
    auto* doubleList = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleList, 1.1);
    celix_arrayList_addDouble(doubleList, 2.2);
    auto* boolList = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolList, true);
    celix_arrayList_addBool(boolList, false);
    celix_autoptr(celix_version_t) version = celix_version_create(1, 2, 3, nullptr);
    auto* versionList = celix_arrayList_createVersionArray();
    celix_arrayList_addVersion(versionList, version);
    celix_arrayList_assignVersion(versionList, celix_version_create(4, 5, 6, nullptr));
    EXPECT_EQ(2, celix_arrayList_size(stringList));
    EXPECT_EQ(2, celix_arrayList_size(longList));
    EXPECT_EQ(2, celix_arrayList_size(doubleList));
    EXPECT_EQ(2, celix_arrayList_size(boolList));
    EXPECT_EQ(2, celix_arrayList_size(versionList));

    //Given the array list are assigned to the properties
    celix_properties_assignArrayList(props, "stringList", stringList);
    celix_properties_assignArrayList(props, "longList", longList);
    celix_properties_assignArrayList(props, "doubleList", doubleList);
    celix_properties_assignArrayList(props, "boolList", boolList);
    celix_properties_assignArrayList(props, "versionList", versionList);
    celix_properties_setString(props, "string", "Hello world");

    //When the celix_properties_getAs<Type>ArrayList is called with the array lists
    celix_autoptr(celix_array_list_t) retrievedStringList = nullptr;
    celix_autoptr(celix_array_list_t) retrievedLongList = nullptr;
    celix_autoptr(celix_array_list_t) retrievedDoubleList = nullptr;
    celix_autoptr(celix_array_list_t) retrievedBoolList = nullptr;
    celix_autoptr(celix_array_list_t) retrievedVersionList = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsStringArrayList(props, "stringList", nullptr, &retrievedStringList));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsLongArrayList(props, "longList", nullptr, &retrievedLongList));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsDoubleArrayList(props, "doubleList", nullptr, &retrievedDoubleList));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsBoolArrayList(props, "boolList", nullptr, &retrievedBoolList));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsVersionArrayList(props, "versionList", nullptr, &retrievedVersionList));

    //Then the retrieved array lists should be the same as the original array lists
    EXPECT_TRUE(celix_arrayList_equals(stringList, retrievedStringList));
    EXPECT_TRUE(celix_arrayList_equals(longList, retrievedLongList));
    EXPECT_TRUE(celix_arrayList_equals(doubleList, retrievedDoubleList));
    EXPECT_TRUE(celix_arrayList_equals(boolList, retrievedBoolList));
    EXPECT_TRUE(celix_arrayList_equals(versionList, retrievedVersionList));

    //When the celix_properties_get<Type>ArrayList is called with the array lists
    const auto* retrievedStringList2 = celix_properties_getStringArrayList(props, "stringList");
    const auto* retrievedLongList2 = celix_properties_getLongArrayList(props, "longList");
    const auto* retrievedDoubleList2 = celix_properties_getDoubleArrayList(props, "doubleList");
    const auto* retrievedBoolList2 = celix_properties_getBoolArrayList(props, "boolList");
    const auto* retrievedVersionList2 = celix_properties_getVersionArrayList(props, "versionList");

    //Then the retrieved array lists pointers should be the same as the original array lists
    EXPECT_EQ(stringList, retrievedStringList2);
    EXPECT_EQ(longList, retrievedLongList2);
    EXPECT_EQ(doubleList, retrievedDoubleList2);
    EXPECT_EQ(boolList, retrievedBoolList2);
    EXPECT_EQ(versionList, retrievedVersionList2);

    //When using the celix_properties_getArrayList function to retrieve the array lists
    const auto* retrievedStringList3 = celix_properties_getArrayList(props, "stringList");
    const auto* retrievedLongList3 = celix_properties_getArrayList(props, "longList");
    const auto* retrievedDoubleList3 = celix_properties_getArrayList(props, "doubleList");
    const auto* retrievedBoolList3 = celix_properties_getArrayList(props, "boolList");
    const auto* retrievedVersionList3 = celix_properties_getArrayList(props, "versionList");
    EXPECT_EQ(nullptr, celix_properties_getArrayList(props, "missing"));

    //Then the retrieved array lists should be the same as the original array lists
    EXPECT_TRUE(celix_arrayList_equals(stringList, retrievedStringList3));
    EXPECT_TRUE(celix_arrayList_equals(longList, retrievedLongList3));
    EXPECT_TRUE(celix_arrayList_equals(doubleList, retrievedDoubleList3));
    EXPECT_TRUE(celix_arrayList_equals(boolList, retrievedBoolList3));
    EXPECT_TRUE(celix_arrayList_equals(versionList, retrievedVersionList3));

    celix_autoptr(celix_array_list_t) longList4 = celix_arrayList_copy(longList);
    celix_autoptr(celix_array_list_t) retrievedLongList4 = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_getAsLongArrayList(props, "string", longList4, &retrievedLongList4));
    EXPECT_TRUE(celix_arrayList_equals(longList, retrievedLongList4));

}

TEST_F(PropertiesTestSuite, GetAsTypedArrayListWithInvalidDefault) {
    celix_autoptr(celix_array_list_t) ptrList = celix_arrayList_createPointerArray();
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_array_list_t* outList = nullptr;

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_getAsStringArrayList(nullptr, "aKey", ptrList, &outList));
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_getAsLongArrayList(nullptr, "aKey", ptrList, &outList));
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_getAsDoubleArrayList(nullptr, "aKey", ptrList, &outList));
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_getAsBoolArrayList(nullptr, "aKey", ptrList, &outList));
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_getAsVersionArrayList(nullptr, "aKey", ptrList, &outList));
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();
}


TEST_F(PropertiesTestSuite, SetArrayListWithIllegalArgumentsTest) {
    //Given a properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //And an array list
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();

    //When the celix_properties_setArrayList is called with a nullptr properties values object a ILLEGAL_ARGUMENT
    // error is returned
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, "key", nullptr));

    //And when an NULL key is used, a ILLEGAL_ARGUMENT error is returned
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setArrayList(props, nullptr, list));
}

TEST_F(PropertiesTestSuite, EmptyStringKeyTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "", "value"); // "" is a valid key (nullptr is not)
    EXPECT_EQ(1, celix_properties_size(props));
    EXPECT_STREQ("value", celix_properties_getString(props, ""));
}
