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
#include "celix_string_hash_map.h"
#include "celix_long_hash_map.h"
#include <random>
#include <atomic>

class HashMapTestSuite : public ::testing::Test {
public:
    /**
     * Create and fill string hash map with nrEntries entries.
     */
    celix_string_hash_map_t* createStringHashMap(int nrEntries) {
        auto* map = celix_stringHashMap_create();
        fillStringHashMap(map, nrEntries);
        return map;
    }

    /**
     * Fill string hash map with nrOfEntries long value entries where the key and value are based on the
     * i index value of the for loop.
     * This way the expected hash map entry value can be deducted from the key.
     */
    void fillStringHashMap(celix_string_hash_map_t* map, int nrEntries) {
        for (int i = 0; i < nrEntries; ++i) {
            std::string key = "key" + std::to_string(i);
            celix_stringHashMap_putLong(map, key.c_str(), i);
            EXPECT_EQ(i, celix_stringHashMap_getLong(map, key.c_str(), 0));
            EXPECT_EQ(celix_stringHashMap_size(map), i+1);
        }
    }

    /**
     * Randomly test nrOfEntriesToTest entries in a string hash map.
     * This assumes that the map is filled using fillStringHashMap.
     */
    void testGetEntriesFromStringMap(celix_string_hash_map_t* map, int nrOfEntriesToTest) {
        std::uniform_int_distribution<long> keyDistribution{0, (long)celix_stringHashMap_size(map)-1};
        for (int i = 0; i < nrOfEntriesToTest; ++i) {
            long rand = keyDistribution(generator);
            auto key = std::string{"key"} + std::to_string(rand);
            EXPECT_EQ(celix_stringHashMap_getLong(map, key.c_str(), 0), rand) << "got wrong result for key " << key;
        }
    }

    /**
     * Create and fill long hash map with nrEntries entries.
     */
    celix_long_hash_map_t * createLongHashMap(int nrEntries) {
        auto* map = celix_longHashMap_create();
        fillLongHashMap(map, nrEntries);
        return map;
    }

    /**
     * Fill long hash map with nrOfEntries long value entries where the key and value are based on the
     * i index value of the for loop.
     * This way the expected hash map entry value can be deducted from the key.
     */
    void fillLongHashMap(celix_long_hash_map_t* map, int nrEntries) {
        for (int i = 0; i < nrEntries; ++i) {
            celix_longHashMap_putLong(map, i, i);
            EXPECT_EQ(i, celix_longHashMap_getLong(map, i, 0));
            EXPECT_EQ(celix_longHashMap_size(map), i+1);
        }
    }

    /**
     * Randomly test nrOfEntriesToTest entries in a long hash map.
     * This assumes that the map is filled using fillLongHashMap.
     */
    void testGetEntriesFromLongMap(celix_long_hash_map_t* map, int nrOfEntriesToTest) {
        std::uniform_int_distribution<long> keyDistribution{0, (long)celix_longHashMap_size(map)-1};
        for (int i = 0; i< nrOfEntriesToTest; ++i) {
            long rand = keyDistribution(generator);
            EXPECT_EQ(celix_longHashMap_getLong(map, rand, 0), rand);
        }
    }
private:
    std::default_random_engine generator{};
};

TEST_F(HashMapTestSuite, CreateDestroyHashMap) {
    auto* sMap = celix_stringHashMap_create();
    EXPECT_TRUE(sMap != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(sMap));
    celix_stringHashMap_destroy(sMap);

    auto* lMap = celix_longHashMap_create();
    EXPECT_TRUE(lMap != nullptr);
    EXPECT_EQ(0, celix_longHashMap_size(lMap));
    celix_longHashMap_destroy(lMap);
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

TEST_F(HashMapTestSuite, DestroyHashMapWithSimpleRemovedCallback) {
    celix_string_hash_map_create_options_t sOpts{};
    sOpts.simpleRemovedCallback = free;
    auto* sMap = celix_stringHashMap_createWithOptions(&sOpts);
    EXPECT_TRUE(sMap != nullptr);
    EXPECT_EQ(0, celix_stringHashMap_size(sMap));

    celix_stringHashMap_put(sMap, "key1", celix_utils_strdup("41"));
    celix_stringHashMap_put(sMap, "key2", celix_utils_strdup("42"));
    celix_stringHashMap_put(sMap, "key3", celix_utils_strdup("43"));
    celix_stringHashMap_put(sMap, "key4", celix_utils_strdup("44"));
    celix_stringHashMap_put(sMap, nullptr, celix_utils_strdup("null"));
    EXPECT_EQ(celix_stringHashMap_size(sMap), 5);
    EXPECT_STREQ((char*)celix_stringHashMap_get(sMap, nullptr), "null");

    bool removed = celix_stringHashMap_remove(sMap, "key3");
    EXPECT_TRUE(removed);
    removed = celix_stringHashMap_remove(sMap, "key3"); //double remove
    EXPECT_FALSE(removed);

    removed = celix_stringHashMap_remove(sMap, nullptr);
    EXPECT_TRUE(removed);
    removed = celix_stringHashMap_remove(sMap, nullptr); //double remove
    EXPECT_FALSE(removed);

    EXPECT_EQ(celix_stringHashMap_size(sMap), 3);
    celix_stringHashMap_destroy(sMap);

    celix_long_hash_map_create_options_t lOpts{};
    lOpts.simpleRemovedCallback = free;
    auto* lMap = celix_longHashMap_createWithOptions(&lOpts);
    EXPECT_TRUE(lMap != nullptr);
    EXPECT_EQ(0, celix_longHashMap_size(lMap));

    celix_longHashMap_put(lMap, 1, celix_utils_strdup("41"));
    celix_longHashMap_put(lMap, 2, celix_utils_strdup("42"));
    celix_longHashMap_put(lMap, 3, celix_utils_strdup("43"));
    celix_longHashMap_put(lMap, 4, celix_utils_strdup("44"));
    celix_longHashMap_put(lMap, 0, celix_utils_strdup("null"));
    EXPECT_EQ(celix_longHashMap_size(lMap), 5);
    EXPECT_STREQ((char*)celix_longHashMap_get(lMap, 0), "null");

    celix_longHashMap_remove(lMap, 3);
    celix_longHashMap_remove(lMap, 0);
    EXPECT_EQ(celix_longHashMap_size(lMap), 3);
    celix_longHashMap_destroy(lMap);
}

TEST_F(HashMapTestSuite, ClearHashMapWithRemovedCallback) {
    std::atomic<int> count{0};
    celix_string_hash_map_create_options_t sOpts{};
    sOpts.removedCallbackData = &count;
    sOpts.removedCallback = [](void *data, const char* key, celix_hash_map_value_t value) {
        auto* c = static_cast<std::atomic<int>*>(data);
        if (celix_utils_stringEquals(key, "key1")) {
            c->fetch_add(1);
            EXPECT_EQ(value.longValue, 1);
        } else if (celix_utils_stringEquals(key, "key2")) {
            c->fetch_add(1);
            EXPECT_EQ(value.longValue, 2);
        }
    };
    auto* sMap = celix_stringHashMap_createWithOptions(&sOpts);
    celix_stringHashMap_putLong(sMap, "key1", 1);
    celix_stringHashMap_putLong(sMap, "key2", 2);
    celix_stringHashMap_clear(sMap);
    EXPECT_EQ(count.load(), 2);
    EXPECT_EQ(celix_stringHashMap_size(sMap), 0);
    celix_stringHashMap_destroy(sMap);

    count = 0;
    celix_long_hash_map_create_options_t lOpts{};
    lOpts.removedCallbackData = &count;
    lOpts.removedCallback = [](void *data, long key, celix_hash_map_value_t value) {
        auto* c = static_cast<std::atomic<int>*>(data);
        if (key == 1) {
            c->fetch_add(1);
            EXPECT_EQ(value.longValue, 1);
        } else if (key == 2) {
            c->fetch_add(1);
            EXPECT_EQ(value.longValue, 2);
        }
    };
    auto* lMap = celix_longHashMap_createWithOptions(&lOpts);
    celix_longHashMap_putLong(lMap, 1, 1);
    celix_longHashMap_putLong(lMap, 2, 2);
    celix_longHashMap_clear(lMap);
    EXPECT_EQ(count.load(), 2);
    EXPECT_EQ(celix_longHashMap_size(lMap), 0);
    celix_longHashMap_destroy(lMap);
}


TEST_F(HashMapTestSuite, HashMapClearTests) {
    auto* sMap = createStringHashMap(3);
    EXPECT_EQ(celix_stringHashMap_size(sMap), 3);
    celix_stringHashMap_clear(sMap);
    EXPECT_EQ(celix_stringHashMap_size(sMap), 0);
    celix_stringHashMap_destroy(sMap);

    auto* lMap = createLongHashMap(3);
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
            auto value = celix_stringHashMap_getLong(sMap, std::get<0>(tuple).c_str(), 0);
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getLong(lMap, std::get<1>(tuple), 0);
            EXPECT_EQ(value, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<double, T>) {
            auto value = celix_stringHashMap_getDouble(sMap, std::get<0>(tuple).c_str(), 0.0);
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getDouble(lMap, std::get<1>(tuple), 0.0);
            EXPECT_EQ(value, std::get<2>(tuple));
        } else if constexpr (std::is_same_v<bool, T>) {
            auto value = celix_stringHashMap_getBool(sMap, std::get<0>(tuple).c_str(), false);
            EXPECT_EQ(value, std::get<2>(tuple));

            value = celix_longHashMap_getBool(lMap, std::get<1>(tuple), false);
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

    std::vector<std::tuple<std::string, long, long>> values2{
            {"key1", 1, 1},
            {"key2", 2, 2},
            {"key3", 3, 3},
    };
    testMapsWithValues(values2);

    std::vector<std::tuple<std::string, long, double>> values3{
            {"key1", 1, 1.0},
            {"key2", 2, 2.0},
            {"key3", 3, 3.0},
    };
    testMapsWithValues(values3);

    std::vector<std::tuple<std::string, long, bool>> values4{
            {"key1", 1, true},
            {"key2", 2, false},
            {"key3", 3, true},
    };
    testMapsWithValues(values4);
}

TEST_F(HashMapTestSuite, GetWithFallbackValues) {
    auto* sMap = celix_stringHashMap_create();
    EXPECT_EQ(celix_stringHashMap_get(sMap, "not-present"), nullptr);
    EXPECT_EQ(celix_stringHashMap_getLong(sMap, "not-present", -1), -1);
    EXPECT_EQ(celix_stringHashMap_getDouble(sMap, "not-present", 1.0), 1.0);
    EXPECT_EQ(celix_stringHashMap_getBool(sMap, "not-present", true), true);
    celix_stringHashMap_destroy(sMap);

    auto* lMap = celix_longHashMap_create();
    EXPECT_EQ(celix_longHashMap_get(lMap, 33), nullptr);
    EXPECT_EQ(celix_longHashMap_getLong(lMap, 33, 1), 1);
    EXPECT_EQ(celix_longHashMap_getDouble(lMap, 33, -1.0), -1.0);
    EXPECT_EQ(celix_longHashMap_getBool(lMap, 33, false), false);
    celix_longHashMap_destroy(lMap);
}


TEST_F(HashMapTestSuite, IterateTest) {
    auto* sMap = createStringHashMap(2);
    size_t count = 0;
    CELIX_STRING_HASH_MAP_ITERATE(sMap, iter) {
        EXPECT_EQ(count++, iter.index);
        if (iter.value.longValue == 0) {
            EXPECT_STREQ(iter.key, "key0");
        } else if (iter.value.longValue == 1) {
            EXPECT_STREQ(iter.key, "key1");
        } else {
            FAIL() << "Unexpected value " << iter.value.longValue;
        }
    }
    EXPECT_EQ(count, 2);
    celix_stringHashMap_destroy(sMap);

    auto* lMap = createLongHashMap(2);
    count = 0;
    CELIX_LONG_HASH_MAP_ITERATE(lMap, iter) {
        EXPECT_EQ(count++, iter.index);
        if (iter.value.longValue == 0) {
            EXPECT_EQ(iter.key, 0);
        } else if (iter.value.longValue == 1) {
            EXPECT_EQ(iter.key, 1);
        } else {
            FAIL() << "Unexpected value " << iter.value.longValue;
        }
    }
    EXPECT_EQ(count, 2);
    celix_longHashMap_destroy(lMap);
}

TEST_F(HashMapTestSuite, IterateStressTest) {
    int testCount = 1000;
    auto* sMap = createStringHashMap(testCount);
    EXPECT_EQ(testCount, celix_stringHashMap_size(sMap));
    int count = 0;
    CELIX_STRING_HASH_MAP_ITERATE(sMap, iter) {
        EXPECT_EQ(iter.index, count++);
    }
    EXPECT_EQ(testCount, count);
    testGetEntriesFromStringMap(sMap, 100);
    celix_stringHashMap_destroy(sMap);

    auto *lMap = createLongHashMap(testCount);
    EXPECT_EQ(testCount, celix_longHashMap_size(lMap));
    count = 0;
    CELIX_LONG_HASH_MAP_ITERATE(lMap, iter) {
        EXPECT_EQ(iter.index, count++);
    }
    EXPECT_EQ(testCount, count);
    testGetEntriesFromLongMap(lMap, 100);
    celix_longHashMap_destroy(lMap);
}

TEST_F(HashMapTestSuite, IterateStressCapacityAndLoadFactorTest) {
    celix_string_hash_map_create_options sOpts{};
    sOpts.loadFactor = 10; //high load factor to ensure buckets have multiple entries for testing
    sOpts.initialCapacity = 5;
    auto* sMap = celix_stringHashMap_createWithOptions(&sOpts);
    fillStringHashMap(sMap, 100);
    long value = celix_stringHashMap_getLong(sMap, "key50", 0);
    EXPECT_EQ(50, value);
    //remove last 50 entries
    for (long i = 50; i < 100; ++i) {
        auto key = std::string{"key"} + std::to_string(i);
        celix_stringHashMap_remove(sMap, key.c_str());
    }
    testGetEntriesFromStringMap(sMap, 50); //test entry 0-49
    celix_stringHashMap_destroy(sMap);


    celix_long_hash_map_create_options lOpts{};
    lOpts.loadFactor = 10; //high load factor to ensure buckets have multiple entries for testing
    lOpts.initialCapacity = 5;
    auto* lMap = celix_longHashMap_createWithOptions(&lOpts);
    fillLongHashMap(lMap, 100);
    value = celix_longHashMap_getLong(lMap, 50, 0);
    EXPECT_EQ(50, value);
    //remove last 50 entries
    for (long i = 50; i < 100; ++i) {
        celix_longHashMap_remove(lMap, i);
    }
    testGetEntriesFromLongMap(lMap, 50);
    celix_longHashMap_destroy(lMap);

}

TEST_F(HashMapTestSuite, IterateWithRemoveTest) {
    auto* sMap = createStringHashMap(6);
    auto iter1 = celix_stringHashMap_begin(sMap);
    while (!celix_stringHashMapIterator_isEnd(&iter1)) {
        if (iter1.index % 2 == 0) {
            //note only removing entries where the iter index is even
            celix_stringHashMapIterator_remove(&iter1);
        } else {
            celix_stringHashMapIterator_next(&iter1);
        }
    }
    EXPECT_EQ(3, celix_stringHashMap_size(sMap));
    EXPECT_TRUE(celix_stringHashMapIterator_isEnd(&iter1));
    celix_stringHashMapIterator_next(&iter1);
    EXPECT_TRUE(celix_stringHashMapIterator_isEnd(&iter1));
    celix_stringHashMap_destroy(sMap);

    auto* lMap = createLongHashMap(6);
    auto iter2 = celix_longHashMap_begin(lMap);
    while (!celix_longHashMapIterator_isEnd(&iter2)) {
        if (iter2.index % 2 == 0) {
            //note only removing entries where the iter index is even
            celix_longHashMapIterator_remove(&iter2);
        } else {
            celix_longHashMapIterator_next(&iter2);
        }
    }
    EXPECT_EQ(3, celix_longHashMap_size(lMap));
    EXPECT_TRUE(celix_longHashMapIterator_isEnd(&iter2));
    celix_longHashMapIterator_next(&iter2); //calling next on end iter, does nothing
    EXPECT_TRUE(celix_longHashMapIterator_isEnd(&iter2));
    celix_longHashMap_destroy(lMap);
}
