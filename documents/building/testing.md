---
title: Testing Apache Celix
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

# Testing Apache Celix

This document describes how to build and run tests for Apache Celix.

## Building Tests

Celix uses CMake and Google Test for its unit and integration tests. To build the tests, ensure you have all dependencies installed, then run:

```sh
cmake -B build -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

```sh
#conan

To enable AddressSanitizer (ASAN) when building tests, configure CMake with the `ENABLE_ASAN` option:

```sh
cmake -B build -DENABLE_TESTING=ON -DENABLE_ADDRESS_SANITIZER=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

This will build Apache Celix and its tests with ASAN enabled, helping to detect memory errors during test execution.

## Running Tests

After building, you can run all tests using CTest:

```sh
ctest --output-on-failure --test-dir build
```

Or run a test for a specific subdir, e.g.: 

```sh
ctest --output-on-failure --test-dir build/bundles/shell
```

Or run a specific test binary directly from the `build` directory, e.g.:

```sh
./build/bundles/components_ready_check/tests/components_ready_check_test
```
