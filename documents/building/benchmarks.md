---
title: Benchmarks in Apache Celix
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->


# Benchmarks in Apache Celix

This document describes how to build and run benchmarks for Apache Celix.

## Building Benchmarks

Benchmarks can be built using the CMake option `ENABLE_BENCHMARKING`
The Apache Celix benchmarks uses Google benchmark library. 

To build benchmarks run:

```sh
cmake -B build -DENABLE_BENCHMARKING=ON
cmake --build build
```

## Benchmarks

The following benchmark executables are available after building the utils and framework benchmarks:

**Utils Benchmarks:**
- `build/libs/utils/benchmark/celix_filter_benchmark`
- `build/libs/utils/benchmark/celix_long_hashmap_benchmark`
- `build/libs/utils/benchmark/celix_string_hashmap_benchmark`
- `build/libs/utils/benchmark/celix_utils_benchmark`

**Framework Benchmarks:**
- `build/libs/framework/benchmark/celix_framework_benchmark`

Paths may vary depending on your configuration and enabled options.

## Running Benchmarks

Benchmark executables are located in the `build` directory, typically under the relevant bundle or library subdirectory. To run a benchmark:

```sh
./build/libs/utils/benchmarks/celix_utils_benchmark
## Command-Line Options
The benchmark executables accept standard Google Benchmark command-line options. 
For example, to run only benchmarks matching a specific pattern and output results in JSON format:

```bash
./build/libs/utils/benchmark/celix_filter_benchmark --benchmark_filter=complexFilter --benchmark_format=json
```

Replace `celix_utils_benchmark` and the filter pattern as needed. To see a list of supported command-line flags, run the benchmark executable with the `--help` option:

```bash
./build/libs/utils/benchmarks/./celix_filter_benchmark --help
```

This will display all available Google Benchmark options.
