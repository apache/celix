# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

if (ENABLE_BENCHMARKING)
    find_package(benchmark REQUIRED)

    add_executable(celix_string_hashmap_benchmark
            src/BenchmarkMain.cc
            src/StringHashmapBenchmark.cc
    )
    target_link_libraries(celix_string_hashmap_benchmark PRIVATE Celix::utils benchmark::benchmark)
    target_compile_options(celix_string_hashmap_benchmark PRIVATE -Wno-unused-function)
    celix_deprecated_utils_headers(celix_string_hashmap_benchmark)

    add_executable(celix_long_hashmap_benchmark
            src/BenchmarkMain.cc
            src/LongHashmapBenchmark.cc
    )
    target_link_libraries(celix_long_hashmap_benchmark PRIVATE Celix::utils benchmark::benchmark)
    target_compile_options(celix_long_hashmap_benchmark PRIVATE -Wno-unused-function)
    celix_deprecated_utils_headers(celix_long_hashmap_benchmark)

    add_executable(celix_filter_benchmark
            src/BenchmarkMain.cc
            src/FilterBenchmark.cc
    )
    target_link_libraries(celix_filter_benchmark PRIVATE Celix::utils benchmark::benchmark)
    target_compile_options(celix_filter_benchmark PRIVATE -Wno-unused-function)
endif ()
