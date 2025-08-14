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

if (ENABLE_BENCHMARKING AND CELIX_CXX17)
    set(CMAKE_CXX_STANDARD 17)
    find_package(benchmark REQUIRED)

    add_executable(celix_framework_benchmark
            src/BenchmarkMain.cc
            src/RegisterServicesBenchmark.cc
            src/LookupServicesBenchmark.cc
            src/DependencyManagerBenchmark.cc
    )
    target_link_libraries(celix_framework_benchmark PRIVATE Celix::framework benchmark::benchmark)
    celix_deprecated_utils_headers(celix_framework_benchmark)
endif ()
