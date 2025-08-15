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
#include <unordered_map>
#include <random>
#include <iostream>
#include <climits>

#include "hash_map.h"
#include "celix_properties.h"
#include "celix_string_hash_map.h"
#include "celix_hash_map_internal.h"
#include "celix_properties_internal.h"
#include "utils.h"

class StringHashmapBenchmark {
public:
    explicit StringHashmapBenchmark(int64_t _nrOfEntries) : testVectorsMap{createRandomMap(_nrOfEntries)} {
        deprecatedHashMap = hashMap_create(utils_stringHash, nullptr, utils_stringEquals, nullptr);
        celix_string_hash_map_create_options_t opts{};
        opts.storeKeysWeakly = true; //ensure that the celix hash map does not copy strings (we don't want to measure that).
        celixHashMap = celix_stringHashMap_createWithOptions(&opts);
    }

    ~StringHashmapBenchmark() {
        celix_stringHashMap_destroy(celixHashMap);
        hashMap_destroy(deprecatedHashMap, false, false);
        celix_properties_destroy(celixProperties);
    }

    void fillStdMap() {
        for (const auto& pair : testVectorsMap) {
            stdMap.emplace(pair.first.c_str(), pair.second);
        }
    }

    void fillCelixHashMap() {
        for (const auto& pair : testVectorsMap) {
            celix_stringHashMap_putLong(celixHashMap, pair.first.c_str(), pair.second);
        }
    }

    void fillDeprecatedCelixHashMap() {
        for (const auto& pair : testVectorsMap) {
            hashMap_put(deprecatedHashMap, (void*)pair.first.c_str(), reinterpret_cast<void*>(pair.second));
        }
    }

    void fillCelixProperties() {
        for (const auto& pair : testVectorsMap) {
            celix_properties_set(celixProperties, pair.first.c_str(), std::to_string(pair.second).c_str()); //note adding entries to properties will always copy the strings.
        }
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
            result[std::move(key)] = createRandomValue();
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

    static constexpr int MAX_LEN = 100;
    std::default_random_engine generator{};
    std::uniform_int_distribution<int> lenDistribution{1,MAX_LEN};
    std::uniform_int_distribution<int> charDistribution{'a','z'};
    std::uniform_int_distribution<int> valDistribution{INT_MIN,INT_MAX};

    std::string midEntryKey{};
    const std::unordered_map<std::string, int> testVectorsMap;
    std::unordered_map<std::string, int> stdMap{};
    celix_string_hash_map_t* celixHashMap{nullptr};
    celix_properties_t* celixProperties{celix_properties_create()};
    hash_map_t* deprecatedHashMap{nullptr};
};

static void StringHashmapBenchmark_addEntryToStdMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillStdMap();
    assert(benchmark.stdMap.size() == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        benchmark.stdMap["latest_entry"]  = 42;
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_addEntryToCelixHashmap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixHashMap();
    assert(celix_stringHashMap_size(benchmark.celixHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        celix_stringHashMap_putLong(benchmark.celixHashMap, "latest_entry", 42);
    }
    state.SetItemsProcessed(state.iterations());
}
static void StringHashmapBenchmark_addEntryToDeprecatedHashmap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillDeprecatedCelixHashMap();
    assert((size_t)hashMap_size(benchmark.deprecatedHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        hashMap_put(benchmark.deprecatedHashMap, (void*)"latest_entry", (void*)42);
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_addEntryToCelixProperties(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixProperties();
    assert((size_t)celix_properties_size(benchmark.celixProperties) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        celix_properties_set(benchmark.celixProperties, "latest_entry", "42");
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromStdMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillStdMap();
    assert(benchmark.stdMap.size() == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        auto it = benchmark.stdMap.find(benchmark.midEntryKey);
        if (it == benchmark.stdMap.end()) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromCelixMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixHashMap();
    assert(celix_stringHashMap_size(benchmark.celixHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        bool hasKey = celix_stringHashMap_hasKey(benchmark.celixHashMap, benchmark.midEntryKey.c_str());
        if (!hasKey) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());

    auto stats = celix_stringHashMap_getStatistics(benchmark.celixHashMap);
    state.counters["nrOfBuckets"] = (double)stats.nrOfBuckets;
    state.counters["resizeCount"] = (double)stats.resizeCount;
    state.counters["averageNrOfEntriesPerBucket"] = stats.averageNrOfEntriesPerBucket;
    state.counters["stdDeviationNrOfEntriesPerBucket"] = stats.stdDeviationNrOfEntriesPerBucket;
}
static void StringHashmapBenchmark_findEntryFromDeprecatedMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillDeprecatedCelixHashMap();
    assert((size_t)hashMap_size(benchmark.deprecatedHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        void* entry = hashMap_get(benchmark.deprecatedHashMap, benchmark.midEntryKey.c_str());
        if (entry == nullptr) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void StringHashmapBenchmark_findEntryFromCelixProperties(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixProperties();
    assert((size_t)celix_properties_size(benchmark.celixProperties) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        const char* entry = celix_properties_get(benchmark.celixProperties, benchmark.midEntryKey.c_str(), nullptr);
        if (entry == nullptr) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());

    auto stats = celix_properties_getStatistics(benchmark.celixProperties);
    state.counters["nrOfBuckets"] = (double)stats.mapStatistics.nrOfBuckets;
    state.counters["resizeCount"] = (double)stats.mapStatistics.resizeCount;
    state.counters["averageNrOfEntriesPerBucket"] = stats.mapStatistics.averageNrOfEntriesPerBucket;
    state.counters["stdDeviationNrOfEntriesPerBucket"] = stats.mapStatistics.stdDeviationNrOfEntriesPerBucket;
    state.counters["sizeOfKeysAndStringValues"] = (double)stats.sizeOfKeysAndStringValues;
    state.counters["averageSizeOfKeysAndStringValues"] = (double)stats.averageSizeOfKeysAndStringValues;
    state.counters["fillStringOptimizationBufferPercentage"] = stats.fillStringOptimizationBufferPercentage;
    state.counters["fillEntriesOptimizationBufferPercentage"] = stats.fillEntriesOptimizationBufferPercentage;
}

static void StringHashmapBenchmark_fillStdMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        for (auto& pair : benchmark.testVectorsMap) {
            benchmark.stdMap.emplace(pair.first, pair.second); //note ideal, because std::string != char*
        }
        state.PauseTiming();
        benchmark.stdMap.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

static void StringHashmapBenchmark_fillCelixHashMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillCelixHashMap();
        state.PauseTiming();
        celix_stringHashMap_clear(benchmark.celixHashMap);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

static void StringHashmapBenchmark_fillDeprecatedHashMap(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillDeprecatedCelixHashMap();
        state.PauseTiming();
        hashMap_clear(benchmark.deprecatedHashMap, false, false);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

static void StringHashmapBenchmark_fillProperties(benchmark::State& state) {
    StringHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillCelixProperties();
        state.PauseTiming();
        celix_properties_destroy(benchmark.celixProperties);
        benchmark.celixProperties = celix_properties_create();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kNanosecond) \
        ->RangeMultiplier(10)->Range(10, 100000)

CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToStdMap); //reference
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToCelixHashmap);
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToDeprecatedHashmap);
CELIX_BENCHMARK(StringHashmapBenchmark_addEntryToCelixProperties);

CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromStdMap); //reference
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromCelixMap);
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromDeprecatedMap);
CELIX_BENCHMARK(StringHashmapBenchmark_findEntryFromCelixProperties);

CELIX_BENCHMARK(StringHashmapBenchmark_fillStdMap); //reference
CELIX_BENCHMARK(StringHashmapBenchmark_fillCelixHashMap);
CELIX_BENCHMARK(StringHashmapBenchmark_fillDeprecatedHashMap);
CELIX_BENCHMARK(StringHashmapBenchmark_fillProperties);
