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
/**
 * properties_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include <string.h>

#include "properties.h"
#include "celix_properties.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(properties) {
    celix_properties_t *properties;

    void setup(void) {
    }

    void teardown() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(properties, create) {
    properties = celix_properties_create();
    CHECK(properties);

    celix_properties_destroy(properties);
}

TEST(properties, load) {
    char propertiesFile[] = "resources-test/properties.txt";
    properties = celix_properties_load(propertiesFile);
    LONGS_EQUAL(4, hashMap_size(properties));

    const char keyA[] = "a";
    const char *valueA = celix_properties_get(properties, keyA, NULL);
    STRCMP_EQUAL("b", valueA);

    const char keyNiceA[] = "nice_a";
    const char *valueNiceA = celix_properties_get(properties, keyNiceA, NULL);
    STRCMP_EQUAL("nice_b", valueNiceA);

    const char keyB[] = "b";
    const char *valueB = celix_properties_get(properties, keyB, NULL);
    STRCMP_EQUAL("c \t d", valueB);

    celix_properties_destroy(properties);
}

TEST(properties, asLong) {
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, "t1", "42");
    celix_properties_set(props, "t2", "-42");
    celix_properties_set(props, "t3", "");
    celix_properties_set(props, "t4", "42 bla"); //converts to 42
    celix_properties_set(props, "t5", "bla");

    long v = celix_properties_getAsLong(props, "t1", -1);
    LONGS_EQUAL(42, v);

    v = celix_properties_getAsLong(props, "t2", -1);
    LONGS_EQUAL(-42, v);

    v = celix_properties_getAsLong(props, "t3", -1);
    LONGS_EQUAL(-1, v);

    v = celix_properties_getAsLong(props, "t4", -1);
    LONGS_EQUAL(42, v);

    v = celix_properties_getAsLong(props, "t5", -1);
    LONGS_EQUAL(-1, v);

    v = celix_properties_getAsLong(props, "non-existing", -1);
    LONGS_EQUAL(-1, v);

    celix_properties_destroy(props);
}

TEST(properties, store) {
    char propertiesFile[] = "resources-test/properties_out.txt";
    properties = celix_properties_create();
    char keyA[] = "x";
    char keyB[] = "y";
    char valueA[] = "1";
    char valueB[] = "2";
    celix_properties_set(properties, keyA, valueA);
    celix_properties_set(properties, keyB, valueB);
    celix_properties_store(properties, propertiesFile, NULL);

    celix_properties_destroy(properties);
}

TEST(properties, copy) {
    char propertiesFile[] = "resources-test/properties.txt";
    properties = celix_properties_load(propertiesFile);
    LONGS_EQUAL(4, hashMap_size(properties));

    celix_properties_t *copy = celix_properties_copy(properties);

    char keyA[] = "a";
    const char *valueA = celix_properties_get(copy, keyA, NULL);
    STRCMP_EQUAL("b", valueA);
    const char keyB[] = "b";
    STRCMP_EQUAL("c \t d", celix_properties_get(copy, keyB, NULL));

    celix_properties_destroy(properties);
    celix_properties_destroy(copy);
}

TEST(properties, getSet) {
    properties = celix_properties_create();
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

    STRCMP_EQUAL(valueA, celix_properties_get(properties, keyA, NULL));
    STRCMP_EQUAL(valueB, celix_properties_get(properties, keyB, NULL));
    STRCMP_EQUAL(valueC, celix_properties_get(properties, keyC, valueC));
    STRCMP_EQUAL(valueD, celix_properties_get(properties, keyD, NULL));

    celix_properties_destroy(properties);
}

TEST(properties, setUnset) {
    properties = celix_properties_create();
    char keyA[] = "x";
    char *keyD = strndup("a", 1);
    char valueA[] = "1";
    char *valueD = strndup("4", 1);
    celix_properties_set(properties, keyA, valueA);
    celix_properties_setWithoutCopy(properties, keyD, valueD);
    STRCMP_EQUAL(valueA, celix_properties_get(properties, keyA, NULL));
    STRCMP_EQUAL(valueD, celix_properties_get(properties, keyD, NULL));

    celix_properties_unset(properties, keyA);
    celix_properties_unset(properties, keyD);
    POINTERS_EQUAL(NULL, celix_properties_get(properties, keyA, NULL));
    POINTERS_EQUAL(NULL, celix_properties_get(properties, "a", NULL));
    celix_properties_destroy(properties);
}

TEST(properties, longTest) {
    properties = celix_properties_create();

    celix_properties_set(properties, "a", "2");
    celix_properties_set(properties, "b", "-10032L");
    celix_properties_set(properties, "c", "");
    celix_properties_set(properties, "d", "garbage");

    long a = celix_properties_getAsLong(properties, "a", -1L);
    long b = celix_properties_getAsLong(properties, "b", -1L);
    long c = celix_properties_getAsLong(properties, "c", -1L);
    long d = celix_properties_getAsLong(properties, "d", -1L);
    long e = celix_properties_getAsLong(properties, "e", -1L);

    CHECK_EQUAL(2, a);
    CHECK_EQUAL(-10032L, b);
    CHECK_EQUAL(-1L, c);
    CHECK_EQUAL(-1L, d);
    CHECK_EQUAL(-1L, e);

    celix_properties_setLong(properties, "a", 3L);
    celix_properties_setLong(properties, "b", -4L);
    a = celix_properties_getAsLong(properties, "a", -1L);
    b = celix_properties_getAsLong(properties, "b", -1L);
    CHECK_EQUAL(3L, a);
    CHECK_EQUAL(-4L, b);

    celix_properties_destroy(properties);
}

TEST(properties, boolTest) {
    properties = celix_properties_create();

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

    CHECK_EQUAL(true, a);
    CHECK_EQUAL(false, b);
    CHECK_EQUAL(true, c);
    CHECK_EQUAL(true, d);
    CHECK_EQUAL(false, e);
    CHECK_EQUAL(true, f);

    celix_properties_setBool(properties, "a", true);
    celix_properties_setBool(properties, "b", false);
    a = celix_properties_getAsBool(properties, "a", false);
    b = celix_properties_getAsBool(properties, "b", true);
    CHECK_EQUAL(true, a);
    CHECK_EQUAL(false, b);

    celix_properties_destroy(properties);
}

TEST(properties, sizeAndIteratorTest) {
    celix_properties_t *props = celix_properties_create();
    CHECK_EQUAL(0, celix_properties_size(props));
    celix_properties_set(props, "a", "1");
    celix_properties_set(props, "b", "2");
    CHECK_EQUAL(2, celix_properties_size(props));
    celix_properties_set(props, "c", "  3  ");
    celix_properties_set(props, "d", "4");
    CHECK_EQUAL(4, celix_properties_size(props));

    int count = 0;
    const char *_key = NULL;
    CELIX_PROPERTIES_FOR_EACH(props, _key) {
        count++;
    }
    CHECK_EQUAL(4, count);

    celix_properties_destroy(props);
}
