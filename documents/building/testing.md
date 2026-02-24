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
Building and testing can be done with Conan 2 (recommended) or with system CMake (manual dependencies).

## Build & test using Conan 2 (recommended)

If the [building](README.md) instructions for conan where followed, there should be a `build/Debug` build directory
and a `conan-debug` cmake preset. This preset can be used to run the tests using ctest:

```bash
ctest --preset conan-debug --output-on-failure -j1
```

Notes:
  - `-j1` is needed because the Apache Celix tests are using cache directories and are not prepared for parallel testing.
  - Use `-o "celix/*:enable_address_sanitizer=True"` during `conan install` to enable AddressSanitizer (ASAN).
  - Use `-o "celix/*:enable_undefined_sanitizer"` during `conan install` to enable UndefinedBehaviorSanitizer (UBSAN).

## Running specific tests

- Run tests matching a regex (useful to run a single test case or test binary):

```bash
# Run tests whose names match "MyTestName"
ctest --preset conan-debug --test-dir build/Debug -R shell --output-on-failure -j1
```

- Run a test binary directly (executable path depends on your build layout):

```bash
source build/Debug/generators/conanrun.sh 
./build/Debug/bundles/components_ready_check/tests/components_ready_check_test
source build/Debug/generators/deactivate_conanrun.sh
```

## Build & test using plain CMake (system dependencies)

If you prefer to use your system packages rather than Conan, install the required dev packages
(see `documents/building/README.md` for package lists) then configure CMake with testing enabled.

Example (configure & build only):
```bash
cmake -DENABLE_TESTING=ON -DENABLE_ADDRESS_SANITIZER=ON -DENABLE_UNDEFINED_SANITIZER=ON -DCMAKE_BUILD_TYPE=Debug -G Ninja -S . -B build
cmake --build . --parallel
ctest --test-dir . --output-on-failure
```

- with apt & cmake you can also run only tests in a certain build-tree subdirectory:

```bash
ctest --preset conan-debug --output-on-failure --test-dir build/Debug/bundles/shell
```
