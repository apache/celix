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
#include <unordered_map>
#include <random>
#include <iostream>
#include <climits>

#include "hash_map.h"
#include "celix_long_hash_map.h"
#include "celix_hash_map_internal.h"

class LongHashmapBenchmark {
public:
    explicit LongHashmapBenchmark(int64_t _nrOfEntries) : testVectorsMap{createRandomMap(_nrOfEntries)} {}

    ~LongHashmapBenchmark() {
        celix_longHashMap_destroy(celixHashMap);
        hashMap_destroy(deprecatedHashMap, false, false);
    }

    LongHashmapBenchmark(LongHashmapBenchmark&&) = delete;
    LongHashmapBenchmark& operator=(LongHashmapBenchmark&&) = delete;
    LongHashmapBenchmark(const LongHashmapBenchmark&) = delete;
    LongHashmapBenchmark& operator=(const LongHashmapBenchmark&) = delete;

    void fillStdMap() {
        for (const auto& pair : testVectorsMap) {
            stdMap.emplace(pair.first, pair.second);
        }
    }

    void fillCelixHashMap() {
        for (const auto& pair : testVectorsMap) {
            celix_longHashMap_putLong(celixHashMap, pair.first, pair.second);
        }
    }

    void fillDeprecatedCelixHashMap() {
        for (const auto& pair : testVectorsMap) {
            hashMap_put(deprecatedHashMap, reinterpret_cast<void*>(pair.first), reinterpret_cast<void*>(pair.second));
        }
    }

    std::unordered_map<long, int> createRandomMap(int64_t nrOfEntries) {
        std::unordered_map<long, int> result{};
        while (result.size() < (size_t)nrOfEntries) {
            auto key = createRandomKey();
            if (result.size() == (size_t)nrOfEntries/2) {
                midEntryKey = key;
            }
            result[key] = createRandomValue();
        }
        return result;
    }

    long createRandomKey() {
        return keyDistribution(generator);
    }

    int createRandomValue() {
        return valDistribution(generator);
    }

    std::default_random_engine generator{};
    std::uniform_int_distribution<long> keyDistribution{LONG_MIN,LONG_MAX};
    std::uniform_int_distribution<int> valDistribution{INT_MIN,INT_MAX};

    long midEntryKey{0};
    const std::unordered_map<long, int> testVectorsMap;
    std::unordered_map<long, int> stdMap{};
    hash_map_t* deprecatedHashMap{hashMap_create(nullptr, nullptr, nullptr, nullptr)};
    celix_long_hash_map_t* celixHashMap{celix_longHashMap_create()};
};

static void LongHashmapBenchmark_addEntryToStdMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillStdMap();
    assert(benchmark.stdMap.size() == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        benchmark.stdMap[42]  = 42;
    }
    state.SetItemsProcessed(state.iterations());
}

static void LongHashmapBenchmark_addEntryToCelixHashmap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixHashMap();
    assert(celix_longHashMap_size(benchmark.celixHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        celix_longHashMap_putLong(benchmark.celixHashMap, 42, 42);
    }
    state.SetItemsProcessed(state.iterations());
}

static void LongHashmapBenchmark_addEntryToDeprecatedHashmap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillDeprecatedCelixHashMap();
    assert((size_t)hashMap_size(benchmark.deprecatedHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        hashMap_put(benchmark.deprecatedHashMap, (void*)42, (void*)42);
    }
    state.SetItemsProcessed(state.iterations());
}

static void LongHashmapBenchmark_findEntryFromStdMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillStdMap();
    assert(benchmark.stdMap.size() == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        auto it = benchmark.stdMap.find(benchmark.midEntryKey);
        if (it == benchmark.stdMap.end()) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << " for std unordered_map." << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void LongHashmapBenchmark_findEntryFromCelixMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillCelixHashMap();
    assert(celix_longHashMap_size(benchmark.celixHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        bool hasKey = celix_longHashMap_hasKey(benchmark.celixHashMap, benchmark.midEntryKey);
        if (!hasKey) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << " for celix hash map." <<std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());

    auto stats = celix_longHashMap_getStatistics(benchmark.celixHashMap);
    state.counters["nrOfBuckets"] = (double)stats.nrOfBuckets;
    state.counters["resizeCount"] = (double)stats.resizeCount;
    state.counters["averageNrOfEntriesPerBucket"] = stats.averageNrOfEntriesPerBucket;
    state.counters["stdDeviationNrOfEntriesPerBucket"] = stats.stdDeviationNrOfEntriesPerBucket;
}

static void LongHashmapBenchmark_findEntryFromDeprecatedMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    benchmark.fillDeprecatedCelixHashMap();
    assert((size_t)hashMap_size(benchmark.deprecatedHashMap) == (size_t)state.range(0));
    for (auto _ : state) {
        // This code gets timed
        void* entry = hashMap_get(benchmark.deprecatedHashMap, reinterpret_cast<void*>(benchmark.midEntryKey));
        if (entry == nullptr) {
            std::cerr << "Cannot find entry " << benchmark.midEntryKey << " for deprecated hash map." << std::endl;
            abort();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void LongHashmapBenchmark_fillStdMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillStdMap();
        state.PauseTiming();
        benchmark.stdMap.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

static void LongHashmapBenchmark_fillCelixHashMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillCelixHashMap();
        state.PauseTiming();
        celix_longHashMap_clear(benchmark.celixHashMap);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

static void LongHashmapBenchmark_fillDeprecatedHashMap(benchmark::State& state) {
    LongHashmapBenchmark benchmark{state.range(0)};
    for (auto _ : state) {
        benchmark.fillDeprecatedCelixHashMap();
        state.PauseTiming();
        hashMap_clear(benchmark.deprecatedHashMap, false, false);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * benchmark.testVectorsMap.size());
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kNanosecond) \
        ->RangeMultiplier(10)->Range(10, 100000)

CELIX_BENCHMARK(LongHashmapBenchmark_addEntryToStdMap); //reference
CELIX_BENCHMARK(LongHashmapBenchmark_addEntryToCelixHashmap);
CELIX_BENCHMARK(LongHashmapBenchmark_addEntryToDeprecatedHashmap);

CELIX_BENCHMARK(LongHashmapBenchmark_findEntryFromStdMap); //reference
CELIX_BENCHMARK(LongHashmapBenchmark_findEntryFromCelixMap);
CELIX_BENCHMARK(LongHashmapBenchmark_findEntryFromDeprecatedMap);

CELIX_BENCHMARK(LongHashmapBenchmark_fillStdMap); //reference
CELIX_BENCHMARK(LongHashmapBenchmark_fillCelixHashMap);
CELIX_BENCHMARK(LongHashmapBenchmark_fillDeprecatedHashMap);
