---
title: Fuzz testing with libFuzzer
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

# Fuzz Testing with libFuzzer

The utilities library contains fuzz targets that can be built with
[LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html).  

Fuzzing is enabled when using the Clang compiler and the `UTILS_LIBFUZZER` CMake
option.

## Building

Configure CMake with Clang and enable the libFuzzer option:

```bash
cmake \
  -G Ninja \
  -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DENABLE_FUZZING=ON
```

Build the fuzzer executables:

```bash
cmake --build build --parallel --target celix_properties_fuzzer celix_version_fuzzer celix_filter_fuzzer
```

## Corpus

The `corpus` directories for the fuzzers contain a few seed inputs, which help guide the initial fuzzing process. 
More files can be added to these directories to improve coverage. The fuzzer will automatically use all files in the 
specified corpus directory as starting points for mutation and exploration.

## Running
The resulting fuzzers accept standard libFuzzer command line options. For example, to run each fuzzer for 30 seconds 
using the provided seed corpus and print coverage information:

```bash
./build/libs/utils/celix_filter_fuzzer -max_total_time=30 -print_coverage=1 ./build/libs/utils/filter_corpus
```

Replace `celix_filter_fuzzer` and `filter_corpus` with the appropriate fuzzer executable and corpus directory as needed.
To see a list of supported command-line flags, run the fuzzer executable with the `-help=1` option. For example:

```bash
./build/libs/utils/celix_filter_fuzzer -help=1
```

This will display all available LibFuzzer options.

## Continuous Fuzzing

Each Celix Fuzzing run attempts to download the latest fuzzing artifact from the same branch and unpack any 
existing corpora before executing the fuzzers so new inputs build on the most recent discoveries.

### Maintaining the seed corpus

The Celix Fuzzing workflow uploads the generated corpora files as a build artifact 
named `fuzzing-corpora-artifact`. 
The master version of the `fuzzing-corpora-artifactz` artifact is used to keep 
the seed corpus in `libs/utils/fuzzing/{filter,properties,version}_corpus` updated. 
