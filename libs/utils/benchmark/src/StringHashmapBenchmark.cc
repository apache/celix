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

#include <benchmark/benchmark.h>
#include <memory>
#include <celix_utils_api.h>
#include <unordered_map>
#include <random>
#include <iostream>
#include <climits>

#include "hash_map.h"
#include "celix_properties.h"
#include "celix_hash_map.h"


class StringHashmapBenchmark {
public:
    explicit StringHashmapBenchmark(int64_t _nrOfEntries, bool fillCHashMap = false, bool fillCProperties = false, bool fillDeprecatedHashMap = false) : stdMap{createRandomMap(_nrOfEntries)} {
        deprecatedHashMap = hashMap_create(utils_stringHash, nullptr, utils_stringEquals, nullptr);
        celix_string_hash_map_create_options_t opts{};
        opts.storeKeysWeakly = true; //ensure that the celix hash map does not copy strings (we don't want to measure that).
        celixHashMap = celix_stringHashMap_createWithOptions(&opts);
        if (fillCHashMap) {
            for (const auto& pair : stdMap) {
                celix_stringHashMap_putLong(celixHashMap, pair.first.c_str(), pair.second);
            }
        }
        if (fillCProperties) {
            for (const auto& pair : stdMap) {
                celix_properties_set(celixProperties, pair.first.c_str(), std::to_string(pair.second).c_str()); //note adding entries to properties will always copy the strings.
            }
        }
        if (fillDeprecatedHashMap) {
            for (const auto& pair : stdMap) {
                hashMap_put(deprecatedHashMap, (void*)pair.first.c_str(), reinterpret_cast<void*>(pair.second));
            }
        }
    }

    ~StringHashmapBenchmark() {
        celix_stringHashMap_destroy(celixHashMap);
        hashMap_destroy(deprecatedHashMap, false, false);
        celix_properties_destroy(celixProperties);
    }

    StringHashmapBenchmark(StringHashmapBenchmark&&) = delete;
    StringHashmapBenchmark& operator=(StringHashmapBenchmark&&) = delete;
    StringHashmapBenchmark(const StringHashmapBenchmark&) = delete;
    StringHashmapBenchmark& operator=(const StringHashmapBenchmark&) = delete;

    std::unordered_map<std::string, int> createRandomMap(int64_t nrOfEntries) {
        std::unordered_map<std::string, int> result{};
        while (result.size() < (size_t)nrOfEntries) {
            auto key = createRandomString();
            if (result.size() == (size_t)nrOfEntries/2) {
                midEntryKey = key;
            }
            stdMap[std::move(key)] = createRandomValue();
        }
        return result;
    }

    std::string createRandomString() {
        char buf[MAX_LEN+1];
        int len = lenDistribution(generator);
        for (int i = 0; i < len; ++i) {
            buf[i] = charDistribution(generator);
        }
        buf[len] = '\0';
        return std::string{buf};
    }

    int createRandomValue() {
        return valDistribution(generator);
    }

    const int MAX_LEN = 100;
    std::default_random_engine generator{};
    std::uniform_int_distribution<int> lenDistribution{1,MAX_LEN};
    std::uniform_int_distribution<int> charDistribution{'a','z'};
    std::uniform_int_distribution<int> valDistribution{INT_MIN,INT_MAX};

    std::string midEntryKey{};
    std::unordered_map<std::string, int> stdMap;
    celix_string_hash_map_t* celixHashMap{nullptr};
    celix_properties_t* celixProperties{celix_properties_create()};
    hash_map_t* deprecatedHashMap{nullptr};
};

static void StringHashmapBenchmark_addEntryToStdMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        // This code gets timed
        benchmark.stdMap["latest_entry"]  = 42;
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_addEntryToCelixHashmap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), true};
    for (auto _ : state) {
        // This code gets timed
        celix_stringHashMap_putLong(benchmark.celixHashMap, "latest_entry", 42);
    }
    state.SetItemsProcessed(state.iterations());
}
static void StringHashmapBenchmark_addEntryToDeprecatedHashmap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), true};
    for (auto _ : state) {
        // This code gets timed
        hashMap_put(benchmark.deprecatedHashMap, (void*)"latest_entry", (void*)42);
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_addEntryToCelixProperties(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), false, true};
    for (auto _ : state) {
        // This code gets timed
        celix_properties_set(benchmark.celixProperties, "latest_entry", "42");
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromStdMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        // This code gets timed
        auto it = benchmark.stdMap.find(benchmark.midEntryKey);
        if (it == benchmark.stdMap.end()) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromCelixMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), true};
    for (auto _ : state) {
        // This code gets timed
        bool hasKey = celix_stringHashMap_hasKey(benchmark.celixHashMap, benchmark.midEntryKey.c_str());
        if (!hasKey) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
        }
    }
    state.SetItemsProcessed(state.iterations());
}
static void StringHashmapBenchmark_findEntryFromDeprecatedMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), false, false, true};
    for (auto _ : state) {
        // This code gets timed
        void* entry = hashMap_get(benchmark.deprecatedHashMap, benchmark.midEntryKey.c_str());
        if (entry == nullptr) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromCelixProperties(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0), false, true};
    for (auto _ : state) {
        // This code gets timed
        const char* entry = celix_properties_get(benchmark.celixProperties, benchmark.midEntryKey.c_str(), nullptr);
        if (entry == nullptr) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
        }
    }
    state.SetItemsProcessed(state.iterations());
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kMicrosecond)

CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToStdMap)->RangeMultiplier(10)->Range(100, 10000); //reference
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToCelixHashmap)->RangeMultiplier(10)->Range(100, 10000);
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToDeprecatedHashmap)->RangeMultiplier(10)->Range(100, 10000);
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToCelixProperties)->RangeMultiplier(10)->Range(100, 10000);

CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromStdMap)->RangeMultiplier(10)->Range(100, 10000); //reference
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromCelixMap)->RangeMultiplier(10)->Range(100, 10000);
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromDeprecatedMap)->RangeMultiplier(10)->Range(100, 10000);
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromCelixProperties)->RangeMultiplier(10)->Range(100, 10000);
