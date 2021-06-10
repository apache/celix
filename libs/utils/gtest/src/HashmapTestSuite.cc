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

#include "celix_utils.h"
#include "celix_string_hashmap.h"

class HashmapTestSuite : public ::testing::Test {};

TEST_F(HashmapTestSuite, CreateDestroy) {
    auto* map = celix_stringHashmap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));
    celix_stringHashmap_destroy(map, false);
}

TEST_F(HashmapTestSuite, PutStringEntries) {
    auto* map = celix_stringHashmap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));

    celix_stringHashMap_put(map, "key1", (void*)0x41);
    celix_stringHashMap_put(map, "key2", (void*)0x42);
    celix_stringHashMap_put(map, "key3", (void*)0x43);
    celix_stringHashMap_put(map, "key4", (void*)0x44);
    EXPECT_EQ(celix_stringHashMap_size(map), 4);

    EXPECT_TRUE(celix_stringHashmap_hasKey(map, "key1"));
    EXPECT_TRUE(celix_stringHashmap_hasKey(map, "key2"));
    EXPECT_TRUE(celix_stringHashmap_hasKey(map, "key3"));
    EXPECT_TRUE(celix_stringHashmap_hasKey(map, "key4"));
    EXPECT_FALSE(celix_stringHashmap_hasKey(map, "key5"));

    EXPECT_EQ(celix_stringHashmap_get(map, "key1"), (void*)0x41);
    EXPECT_EQ(celix_stringHashmap_get(map, "key2"), (void*)0x42);
    EXPECT_EQ(celix_stringHashmap_get(map, "key3"), (void*)0x43);
    EXPECT_EQ(celix_stringHashmap_get(map, "key4"), (void*)0x44);
    EXPECT_EQ(celix_stringHashmap_get(map, "key5"), nullptr);

    celix_stringHashmap_destroy(map, false);
}

TEST_F(HashmapTestSuite, StringNullEntry) {
    auto* map = celix_stringHashmap_create();
    EXPECT_FALSE(celix_stringHashmap_hasKey(map, NULL));
    celix_stringHashMap_put(map, NULL, (void*)0x42);
    EXPECT_TRUE(celix_stringHashmap_hasKey(map, NULL));
    EXPECT_EQ(1, celix_stringHashMap_size(map));
    celix_stringHashmap_destroy(map, false);
}

TEST_F(HashmapTestSuite, RemoveStringEntries) {
    auto* map = celix_stringHashmap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));

    celix_stringHashMap_put(map, "key1", celix_utils_strdup("41"));
    celix_stringHashMap_put(map, "key2", celix_utils_strdup("42"));
    celix_stringHashMap_put(map, "key3", celix_utils_strdup("43"));
    celix_stringHashMap_put(map, "key4", celix_utils_strdup("44"));
    celix_stringHashMap_put(map, NULL, celix_utils_strdup("null"));
    EXPECT_EQ(celix_stringHashMap_size(map), 5);
    EXPECT_STREQ((char*)celix_stringHashmap_get(map, NULL), "null");

    char* val = (char*)celix_stringHashMap_remove(map, "key3");
    free(val);
    val = (char*)celix_stringHashMap_remove(map, NULL);
    free(val);
    EXPECT_EQ(celix_stringHashMap_size(map), 3);

    celix_stringHashmap_destroy(map, true);
}