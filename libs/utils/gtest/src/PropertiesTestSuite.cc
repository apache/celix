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
#include "celix_utils.h"

using ::testing::MatchesRegex;

class PropertiesTestSuite : public ::testing::Test {
public:
  PropertiesTestSuite() {
      celix_err_resetErrors();
  }

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
    celix_properties_setWithoutCopy(properties, keyD, valueD);
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
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setVersionWithoutCopy(props, "key", version));
    EXPECT_EQ(version, celix_properties_getVersion(props, "key", nullptr));
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

    EXPECT_EQ(5, celix_properties_size(props));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, celix_properties_getType(props, "string"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, celix_properties_getType(props, "long"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(props, "double"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, celix_properties_getType(props, "bool"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "version"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_UNSET, celix_properties_getType(props, "missing"));

    auto* copy = celix_properties_copy(props);
    EXPECT_EQ(5, celix_properties_size(copy));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, celix_properties_getType(copy, "string"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, celix_properties_getType(copy, "long"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_DOUBLE, celix_properties_getType(copy, "double"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_BOOL, celix_properties_getType(copy, "bool"));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(copy, "version"));

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
    celix_properties_setVersionWithoutCopy(properties, "key3", celix_version_create(3,3,3,""));
    actual = celix_properties_getVersion(properties, "key3", emptyVersion);
    EXPECT_EQ(celix_version_getMajor(actual), 3);
    EXPECT_EQ(celix_version_getMinor(actual), 3);
    EXPECT_EQ(celix_version_getMicro(actual), 3);
    EXPECT_STREQ(celix_version_getQualifier(actual), "");


    // Test getAsVersion
    celix_properties_set(properties, "string_version", "1.1.1");
    auto* ver1 = celix_properties_getAsVersion(properties, "non-existing", emptyVersion);
    auto* ver2 = celix_properties_getAsVersion(properties, "non-existing", nullptr);
    auto* ver3 = celix_properties_getAsVersion(properties, "string_version", emptyVersion);
    auto* ver4 = celix_properties_getAsVersion(properties, "key", emptyVersion);
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
    celix_properties_setWithoutCopy(props, celix_utils_strdup("key"), celix_utils_strdup("value2"));
    //replace, should free old value and provided key
    celix_properties_setWithoutCopy(props, celix_utils_strdup("key"), celix_utils_strdup("value2"));
    EXPECT_EQ(1, celix_properties_size(props));
    celix_properties_destroy(props);
}

TEST_F(PropertiesTestSuite, SetEntryTest) {
    auto* props1 = celix_properties_create();
    auto* props2 = celix_properties_create();
    celix_properties_set(props1, "key1", "value1");
    celix_properties_setLong(props1, "key2", 123);
    celix_properties_setBool(props1, "key3", true);
    celix_properties_setDouble(props1, "key4", 3.14);
    auto* version = celix_version_create(1, 2, 3, nullptr);
    celix_properties_setVersion(props1, "key5", version);
    celix_version_destroy(version);

    CELIX_PROPERTIES_ITERATE(props1, visit) {
        celix_properties_setEntry(props2, visit.key, &visit.entry);
    }

    EXPECT_EQ(5, celix_properties_size(props2));
    EXPECT_STREQ("value1", celix_properties_get(props2, "key1", nullptr));
    EXPECT_EQ(123, celix_properties_getAsLong(props2, "key2", -1L));
    EXPECT_EQ(true, celix_properties_getAsBool(props2, "key3", false));
    EXPECT_EQ(3.14, celix_properties_getAsDouble(props2, "key4", -1.0));
    EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props2, "key5"));

    celix_properties_destroy(props1);
    celix_properties_destroy(props2);
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

    celix_properties_setVersionWithoutCopy(prop1, "key4", celix_version_create(1,2,3, nullptr));
    EXPECT_FALSE(celix_properties_equals(prop1, prop2));
    celix_properties_setVersionWithoutCopy(prop2, "key4", celix_version_create(1,2,3, nullptr));
    EXPECT_TRUE(celix_properties_equals(prop1, prop2));

    celix_properties_setLong(prop1, "key5", 42);
    celix_properties_setDouble(prop2, "key5", 42.0);
    EXPECT_FALSE(celix_properties_equals(prop1, prop2)); //different types
}

TEST_F(PropertiesTestSuite, PropertiesNullArgumentsTest) {
    //Silently ignore nullptr properties arguments for set* and copy functions
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_set(nullptr, "key", "value"));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setLong(nullptr, "key", 1));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setDouble(nullptr, "key", 1.0));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setBool(nullptr, "key", true));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setVersion(nullptr, "key", nullptr));
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_setVersionWithoutCopy(nullptr, "key", nullptr));
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
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setWithoutCopy(props, nullptr, strdup("value")));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_setWithoutCopy(props, strdup("key"), nullptr));
    EXPECT_EQ(2, celix_err_getErrorCount());
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
