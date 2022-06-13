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
#include "celix_hash_map.h"

class HashMapTestSuite : public ::testing::Test {};

TEST_F(HashMapTestSuite, CreateDestroyStringHashMap) {
    auto* map = celix_stringHashMap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));
    celix_stringHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, CreateDestroyLongHashMap) {
    auto* map = celix_longHashMap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_longHashMap_size(map));
    celix_longHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, PutPointerEntriesInStringHashMap) {
    auto* map = celix_stringHashMap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));

    celix_stringHashMap_put(map, "key1", (void*)0x41);
    celix_stringHashMap_put(map, "key2", (void*)0x42);
    celix_stringHashMap_put(map, "key3", (void*)0x43);
    celix_stringHashMap_put(map, "key4", (void*)0x44);
    EXPECT_EQ(celix_stringHashMap_size(map), 4);

    EXPECT_TRUE(celix_stringHashMap_hasKey(map, "key1"));
    EXPECT_TRUE(celix_stringHashMap_hasKey(map, "key2"));
    EXPECT_TRUE(celix_stringHashMap_hasKey(map, "key3"));
    EXPECT_TRUE(celix_stringHashMap_hasKey(map, "key4"));
    EXPECT_FALSE(celix_stringHashMap_hasKey(map, "key5"));

    EXPECT_EQ(celix_stringHashMap_get(map, "key1"), (void*)0x41);
    EXPECT_EQ(celix_stringHashMap_get(map, "key2"), (void*)0x42);
    EXPECT_EQ(celix_stringHashMap_get(map, "key3"), (void*)0x43);
    EXPECT_EQ(celix_stringHashMap_get(map, "key4"), (void*)0x44);
    EXPECT_EQ(celix_stringHashMap_get(map, "key5"), nullptr);

    celix_stringHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, PutPointerEntriesInLongHashMap) {
    auto* map = celix_longHashMap_create();
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_longHashMap_size(map));

    celix_longHashMap_put(map, 1, (void*)0x41);
    celix_longHashMap_put(map, 2, (void*)0x42);
    celix_longHashMap_put(map, 3, (void*)0x43);
    celix_longHashMap_put(map, 4, (void*)0x44);
    EXPECT_EQ(celix_longHashMap_size(map), 4);

    EXPECT_TRUE(celix_longHashMap_hasKey(map, 1));
    EXPECT_TRUE(celix_longHashMap_hasKey(map, 2));
    EXPECT_TRUE(celix_longHashMap_hasKey(map, 3));
    EXPECT_TRUE(celix_longHashMap_hasKey(map, 4));
    EXPECT_FALSE(celix_longHashMap_hasKey(map, 5));

    EXPECT_EQ(celix_longHashMap_get(map, 1), (void*)0x41);
    EXPECT_EQ(celix_longHashMap_get(map, 2), (void*)0x42);
    EXPECT_EQ(celix_longHashMap_get(map, 3), (void*)0x43);
    EXPECT_EQ(celix_longHashMap_get(map, 4), (void*)0x44);
    EXPECT_EQ(celix_longHashMap_get(map, 5), nullptr);

    celix_longHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, StringKeyNullEntry) {
    auto* map = celix_stringHashMap_create();
    EXPECT_FALSE(celix_stringHashMap_hasKey(map, nullptr));
    celix_stringHashMap_put(map, nullptr, (void*)0x42);
    EXPECT_TRUE(celix_stringHashMap_hasKey(map, nullptr));
    EXPECT_EQ(1, celix_stringHashMap_size(map));
    celix_stringHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, DestroyStringHashMapWithDestroyCallback) {
    celix_string_hash_map_create_options_t opts{};
    opts.simpleRemovedCallback = free;
    auto* map = celix_stringHashMap_createWithOptions(&opts);
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(map));

    celix_stringHashMap_put(map, "key1", celix_utils_strdup("41"));
    celix_stringHashMap_put(map, "key2", celix_utils_strdup("42"));
    celix_stringHashMap_put(map, "key3", celix_utils_strdup("43"));
    celix_stringHashMap_put(map, "key4", celix_utils_strdup("44"));
    celix_stringHashMap_put(map, nullptr, celix_utils_strdup("null"));
    EXPECT_EQ(celix_stringHashMap_size(map), 5);
    EXPECT_STREQ((char*)celix_stringHashMap_get(map, nullptr), "null");

    void* ptr = celix_stringHashMap_removeAndReturn(map, "key3");
    EXPECT_NE(ptr, nullptr);
    ptr = celix_stringHashMap_removeAndReturn(map, "key3"); //double remove
    EXPECT_EQ(ptr, nullptr);

    ptr = celix_stringHashMap_removeAndReturn(map, nullptr);
    EXPECT_NE(ptr, nullptr);
    ptr = celix_stringHashMap_removeAndReturn(map, nullptr); //double remove
    EXPECT_EQ(ptr, nullptr);

    EXPECT_EQ(celix_stringHashMap_size(map), 3);

    celix_stringHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, DestroyLongHashMapWithDestroyCallback) {
    celix_long_hash_map_create_options_t opts{};
    opts.simpledRemoveCallback = free;
    auto* map = celix_longHashMap_createWithOptions(&opts);
    EXPECT_TRUE(map != nullptr);
    EXPECT_EQ(0, celix_longHashMap_size(map));

    celix_longHashMap_put(map, 1, celix_utils_strdup("41"));
    celix_longHashMap_put(map, 2, celix_utils_strdup("42"));
    celix_longHashMap_put(map, 3, celix_utils_strdup("43"));
    celix_longHashMap_put(map, 4, celix_utils_strdup("44"));
    celix_longHashMap_put(map, 0, celix_utils_strdup("null"));
    EXPECT_EQ(celix_longHashMap_size(map), 5);
    EXPECT_STREQ((char*)celix_longHashMap_get(map, 0), "null");

    celix_longHashMap_removeAndReturn(map, 3);
    celix_longHashMap_removeAndReturn(map, 0);
    EXPECT_EQ(celix_longHashMap_size(map), 3);

    celix_longHashMap_destroy(map);
}

TEST_F(HashMapTestSuite, HashMapClearTests) {
    auto* sMap = celix_stringHashMap_create();
    celix_stringHashMap_putLong(sMap, "key1", 1);
    celix_stringHashMap_putLong(sMap, "key2", 2);
    celix_stringHashMap_putLong(sMap, "key3", 3);
    EXPECT_EQ(celix_stringHashMap_size(sMap), 3);
    celix_stringHashMap_clear(sMap);
    EXPECT_EQ(celix_stringHashMap_size(sMap), 0);
    celix_stringHashMap_destroy(sMap);


    auto* lMap = celix_longHashMap_create();
    celix_longHashMap_putLong(lMap, 1, 1);
    celix_longHashMap_putLong(lMap, 2, 2);
    celix_longHashMap_putLong(lMap, 3, 3);
    EXPECT_EQ(celix_longHashMap_size(lMap), 3);
    celix_longHashMap_clear(lMap);
    EXPECT_EQ(celix_longHashMap_size(lMap), 0);
    celix_longHashMap_destroy(lMap);
}


template<typename T>
void testMapsWithValues(const std::vector<std::tuple<std::string, long, T>>& values) {
    auto* sMap = celix_stringHashMap_create();
    auto* lMap = celix_longHashMap_create();

    //test hashmap put
    for (const auto& tuple : values) {
        if constexpr (std::is_same_v<void*, T>) {
            void* prev = celix_stringHashMap_put(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_EQ(prev, nullptr);
            prev = celix_stringHashMap_put(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_EQ(prev, std::get<2>(tuple));

            prev = celix_longHashMap_put(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_EQ(prev, nullptr);
            prev = celix_longHashMap_put(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_EQ(prev, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<long, T>) {
            bool replaced = celix_stringHashMap_putLong(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_stringHashMap_putLong(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_TRUE(replaced);

            replaced = celix_longHashMap_putLong(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_longHashMap_putLong(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_TRUE(replaced);
        } else if constexpr (std::is_same_v<double, T>) {
            bool replaced = celix_stringHashMap_putDouble(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_stringHashMap_putDouble(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_TRUE(replaced);

            replaced = celix_longHashMap_putDouble(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_longHashMap_putDouble(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_TRUE(replaced);
        } else if constexpr (std::is_same_v<bool, T>) {
            bool replaced = celix_stringHashMap_putBool(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_stringHashMap_putBool(sMap, std::get<0>(tuple).c_str(), std::get<2>(tuple));
            EXPECT_TRUE(replaced);

            replaced = celix_longHashMap_putBool(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_FALSE(replaced);
            replaced = celix_longHashMap_putBool(lMap, std::get<1>(tuple), std::get<2>(tuple));
            EXPECT_TRUE(replaced);
        } else {
            FAIL() << "expected of the following value types: void*, long, double or bool";
        }
    }
    ASSERT_EQ(values.size(), celix_stringHashMap_size(sMap));
    ASSERT_EQ(values.size(), celix_longHashMap_size(lMap));

    //test hashmap get
    for (const auto& tuple : values) {
        if constexpr (std::is_same_v<void*, T>) {
            auto* value = celix_stringHashMap_get(sMap, std::get<0>(tuple).c_str());
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_get(lMap, std::get<1>(tuple));
            EXPECT_EQ(value, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<long, T>) {
            auto value = celix_stringHashMap_getLong(sMap, std::get<0>(tuple).c_str());
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getLong(lMap, std::get<1>(tuple));
            EXPECT_EQ(value, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<double, T>) {
            auto value = celix_stringHashMap_getDouble(sMap, std::get<0>(tuple).c_str());
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getDouble(lMap, std::get<1>(tuple));
            EXPECT_EQ(value, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<bool, T>) {
            auto value = celix_stringHashMap_getBool(sMap, std::get<0>(tuple).c_str());
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getBool(lMap, std::get<1>(tuple));
            EXPECT_EQ(value, std::get<2>(tuple));
        } else {
            FAIL() << "expected of the following value types: void*, long, double or bool";
        }
    }

    //test hashmap remove
    for (const auto& tuple : values) {
        celix_stringHashMap_remove(sMap, std::get<0>(tuple).c_str());
        celix_longHashMap_remove(lMap, std::get<1>(tuple));
    }
    ASSERT_EQ(0, celix_stringHashMap_size(sMap));
    ASSERT_EQ(0, celix_longHashMap_size(lMap));

    celix_stringHashMap_destroy(sMap);
    celix_longHashMap_destroy(lMap);
}

TEST_F(HashMapTestSuite, PutAndGetWithDifferentValueTypes) {
    std::vector<std::tuple<std::string, long, void*>> values1{
            {"key1", 1, (void*)0x42},
            {"key2", 2, (void*)0x43},
            {"key3", 3, (void*)0x44},
    };
    testMapsWithValues(values1);
}

TEST_F(HashMapTestSuite, IterateTest) {
    auto* sMap = celix_stringHashMap_create();
    celix_stringHashMap_putLong(sMap, "key1", 1);
    celix_stringHashMap_putLong(sMap, "key2", 2);

    size_t count = 0;
    //TODO replace with CELIX_STRING_HASH_MAP_ITERATE macro
    for (auto iter = celix_stringHashMap_iterate(sMap); !celix_stringHasMapIterator_isEnd(&iter); celix_stringHasMapIterator_next(&iter)) {
        EXPECT_EQ(count++, iter.index);
        if (iter.value.longValue == 1) {
            EXPECT_STREQ(iter.key, "key1");
        } else if (iter.value.longValue == 2) {
            EXPECT_STREQ(iter.key, "key2");
        } else {
            FAIL() << "Unexpected value " << iter.value.longValue;
        }
        ++count;
    }
    EXPECT_EQ(count, 2);
    celix_stringHashMap_destroy(sMap);

    auto* lMap = celix_longHashMap_create();
    celix_longHashMap_putLong(lMap, 1, 1);
    celix_longHashMap_putLong(lMap, 2, 2);

    /*
    count = 0;
    for (auto iter = celix_longHashMap_iterate(lMap); celix_longHasMapIterator_hasNext(&iter); celix_longHasMapIterator_next(&iter)) {
        if (iter.iterIndex == 0) {
            EXPECT_STREQ(iter.key, 1);
            EXPECT_EQ(iter.value.longValue, 1);
        } else {
            EXPECT_STREQ(iter.key, 2);
            EXPECT_EQ(iter.value.longValue, 2);
        }
        ++count;
    }
    EXPECT_EQ(count, 2);
     */
}

TEST_F(HashMapTestSuite, IterateStressTest) {
    int testCount = 1000;
    auto* sMap = celix_stringHashMap_create();

    for (int i = 0; i < testCount; ++i) {
        auto key = std::string{"key"} + std::to_string(i);
        celix_stringHashMap_putLong(sMap, key.c_str(), i);
    }
    EXPECT_EQ(testCount, celix_stringHashMap_size(sMap));

    //TODO replace with CELIX_STRING_HASH_MAP_ITERATE macro
    int count = 0;
    for (auto iter = celix_stringHashMap_iterate(sMap); !celix_stringHasMapIterator_isEnd(&iter); celix_stringHasMapIterator_next(&iter)) {
        EXPECT_EQ(iter.index, count++);
    }
    EXPECT_EQ(testCount, count);
    celix_stringHashMap_destroy(sMap);
}
