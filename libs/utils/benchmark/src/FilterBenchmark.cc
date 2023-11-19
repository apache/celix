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
#include <iostream>
#include <random>
#include <climits>

#include "celix/Filter.h"
#include "celix/Properties.h"
#include "celix_properties_internal.h"

class FilterBenchmark {
public:
    explicit FilterBenchmark(benchmark::State& state) : props{createTestProperties()} {
        int64_t additionalPropertyEntries = state.range(0);
        fillProperties(additionalPropertyEntries);
    }

    static celix::Properties createTestProperties() {
        celix::Properties props{};
        props.set("str_key1", "str_value1");
        props.set("long_key1", 1L);
        props.set("double_key1", 1.0);
        props.set("bool_key1", true);
        props.set("version_key1", celix::Version{1,2,3});
        props.set("version_key2", celix::Version{1,2,3, "qualifier"});
        return props;
    }

    void testFilter(benchmark::State& state, const celix::Filter& filter, bool expectedMatch) {
        for (auto _ : state) {
            // This code gets timed
            auto match = filter.match(props);
            if (match != expectedMatch) {
                std::cerr << "ERROR: unexpected match result" << std::endl;
            }
        }
        addStateCounters(state);
    }

    void testCFilter(benchmark::State& state, const celix::Filter& filter, bool expectedMatch) {
        auto* cFilter = filter.getCFilter();
        auto* cProps = props.getCProperties();
        for (auto _ : state) {
            // This code gets timed
            auto match = celix_filter_match(cFilter, cProps);
            if (match != expectedMatch) {
                std::cerr << "ERROR: unexpected match result" << std::endl;
            }

        }
        addStateCounters(state);
    }

    void addStateCounters(benchmark::State& state) {
        state.SetItemsProcessed(state.iterations());
        auto stats = celix_properties_getStatistics(props.getCProperties());
        //state.counters["nrOfProperties"] = (double)stats.mapStatistics.nrOfEntries;
        //state.counters["nrOfBuckets"] = (double)stats.mapStatistics.nrOfBuckets;
        //state.counters["resizeCount"] = (double)stats.mapStatistics.resizeCount;
        //state.counters["averageNrOfEntriesPerBucket"] = stats.mapStatistics.averageNrOfEntriesPerBucket;
        //state.counters["stdDeviationNrOfEntriesPerBucket"] = stats.mapStatistics.stdDeviationNrOfEntriesPerBucket;
        //state.counters["averageSizeOfKeysAndStringValues"] = (double)stats.averageSizeOfKeysAndStringValues;
        //state.counters["sizeOfKeysAndStringValues"] = (double)stats.sizeOfKeysAndStringValues;
        state.counters["fillStringOptimizationBufferPercentage"] = stats.fillStringOptimizationBufferPercentage;
        state.counters["fillEntriesOptimizationBufferPercentage"] = stats.fillEntriesOptimizationBufferPercentage;
    }

    std::string randomString() {
        std::string result{};
        int len = lenDistribution(generator);
        result.reserve(len);
        for (int i = 0; i < len-1; ++i) {
            result[i] = (char)charDistribution(generator);
        }
        result[len-1] = '\0';
        return result;
    }

    void fillProperties(int64_t additionalPropertyEntries) {
        for (auto i = 0; i < additionalPropertyEntries; ++i) {
            props.set(randomString(), randomString());
        }
    }

  private:
    celix::Properties props{};

    const int MAX_LEN = 100;
    std::default_random_engine generator{};
    std::uniform_int_distribution<int> lenDistribution{2,MAX_LEN};
    std::uniform_int_distribution<int> charDistribution{'a','z'};
};

static void FilterBenchmark_testStringCFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(str_key1=str_value1)"};
    benchmark.testCFilter(state, filter, true);
}

static void FilterBenchmark_testStringFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(str_key1=str_value1)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testStringGreaterEqualFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(str_key1>=str_value)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testLongFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(long_key1=1)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testLongGreaterEqualFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(long_key1>=1)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testDoubleGreaterEqualFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(double_key1>=0.9)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testBoolFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(bool_key1=true)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testVersionFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(version_key1=1.2.3)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_testVersionWithQualifierFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(version_key2=1.2.3.qualifier)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_versionGreaterEqualFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(version_key1>=1.0.0)"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_versionRangeFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(&(version_key1>=1.0.0)(version_key1<2.0.0))"};
    benchmark.testFilter(state, filter, true);
}

static void FilterBenchmark_substringFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(&(version_key1>=1.0.0)(version_key1<2.0.0))"};
    benchmark.testFilter(state, filter, true);
}


static void FilterBenchmark_complexFilter(benchmark::State& state) {
    FilterBenchmark benchmark{state};
    celix::Filter filter{"(str_key1=*value1)"};
    benchmark.testFilter(state, filter, true);
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kNanosecond) \
        ->RangeMultiplier(100)->Range(1, 10000)

CELIX_BENCHMARK(FilterBenchmark_testStringCFilter); //done on C API, to check if C++ api does not have a big overhead

//Operator =
CELIX_BENCHMARK(FilterBenchmark_testStringFilter);
CELIX_BENCHMARK(FilterBenchmark_testLongFilter);
//note no double = test
CELIX_BENCHMARK(FilterBenchmark_testBoolFilter);
CELIX_BENCHMARK(FilterBenchmark_testVersionFilter);
CELIX_BENCHMARK(FilterBenchmark_testVersionWithQualifierFilter);

//Operator >=
CELIX_BENCHMARK(FilterBenchmark_testStringGreaterEqualFilter);
CELIX_BENCHMARK(FilterBenchmark_testLongGreaterEqualFilter);
CELIX_BENCHMARK(FilterBenchmark_testDoubleGreaterEqualFilter);
//note no bool >= test
CELIX_BENCHMARK(FilterBenchmark_versionGreaterEqualFilter);

//Specials
CELIX_BENCHMARK(FilterBenchmark_versionRangeFilter);
CELIX_BENCHMARK(FilterBenchmark_substringFilter);
CELIX_BENCHMARK(FilterBenchmark_complexFilter);
