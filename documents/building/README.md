---
title: Building and Installing Apache Celix
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

# Building and Installing Apache Celix

Apache Celix aims to be support a broad range of UNIX platforms.
 
Currently, the [continuous integration build server] builds and tests Apache Celix for:

* Ubuntu Jammy Jellyfish (22.04)
  * GCC 
  * CLang 
* OSX
  * CLang


### Download the Apache Celix sources
To get started you first have to download the Apache Celix sources. This can be done by cloning the Apache Celix git repository:

```bash
#clone the repro
git clone --single-branch --branch master https://github.com/apache/celix.git
```

## Building and installing

Apache Celix can be build using [Conan](https://conan.io) as package manager/build system or by directly using
[CMake](https://cmake.org).

### Building Apache Celix using Conan
The following packages (libraries + headers) should be installed on your system:

* Development Environment
    * build-essentials 
    * zip (for packaging bundles)
    * cmake (3.19 or higher)
    * Conan (2 or higher)

For Ubuntu 22.04, use the following commands:
```bash
sudo apt-get install -yq --no-install-recommends \
    build-essential \
    cmake \
    git \
    default-jdk \
    python3 \
    python3-pip \
    ninja-build
        
#Install conan
pip3 install -U conan
```

Configure the Conan default profile using automatic detection of the system:
```bash
conan profile detect
```

Conan 2 uses a two-step workflow: first generate the CMake presets and toolchain files with `conan install`, 
then configure and build with CMake. The project provides convenient presets when you run `conan install`.

Create a build directory and install dependencies (this will generate CMakePresets.json / CMakeUserPresets.json in the source or build folder):

```bash
cd <celix_source_dir>
# Create an output folder for the conan-generated files
conan install . --build=missing --profile:build default --profile:host debug \
    -o "celix/*:build_all=True" \
    -o "celix/*:enable_testing=True" \
    -o "celix/*:enable_ccache=True" \
    --conf tools.cmake.cmaketoolchain:generator=Ninja
```

Notes:
- Use `--profile:host debug` or `--profile:host default` depending on the host (target) profile you want to generate builds for.
- Replace or add `-o` options to selectively enable/disable bundles (see below).

Configure and build using the generated CMake preset (Conan will create presets named like `conan-debug` when 
using `--profile:host debug`, assuming the build_type is `Debug`):

```bash
# Configure with a conan-generated preset (conan-debug in this case)
 cmake --build --preset conan-debug --parallel
```

When using Conan you typically do not "install" Celix system-wide; Conan places package artifacts in the local Conan 
cache and the generated build files allow you to produce executables and run tests.

It is also possible to only build a selection of the Apache Celix bundles and/or libraries. 
This can be done by passing per-package options instead of building everything. 
For example, to only build the framework and utils libraries:
```bash
conan install . --build=missing --profile:build default --profile:host debug \
    -o "celix/*:build_framework=True" \
    -o "celix/*:build_utils=True"
 cmake --build --preset conan-debug --parallel
```

To see a complete overview of the available build options in the recipe you can inspect the recipe metadata (this works for local recipes too):
```bash
conan inspect . | grep build_
```

#### CMake Private Linking Workaround (Conan)

When using Celix via Conan, you may encounter an [issue](https://github.com/apache/celix/issues/642) where `libzip.so` is not found by the linker. This is due to a [bug in Conan](https://github.com/conan-io/conan/issues/7192).

A workaround we adopt in Celix is adding the following to `conanfile.py` (the same approach applies for Conan 2's `generate()`):

```text
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        # the following is workaround for https://github.com/conan-io/conan/issues/7192
        if self.settings.os == "Linux":
            tc.cache_variables["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,--unresolved-symbols=ignore-in-shared-libs"
        elif self.settings.os == "Macos":
            tc.cache_variables["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,-undefined -Wl,dynamic_lookup"
        tc.generate()
```

### Building Apache Celix directly using CMake
The following packages (libraries + headers) should be installed on your system:

* Development Environment
    * build-essentials (gcc/g++ or clang/clang++) 
    * zip (for packaging bundles)
    * cmake (3.19 or higher)
    * ninja (to build with ninja instead of make)
* Apache Celix Dependencies
    * libzip
    * uuid
    * zlib
    * curl (only initialized in the Celix framework)
    * jansson (json properties and manifest handling)
    * libffi (for libdfi)
    * libxml2 (for remote services and bonjour shell)
    * rapidjson (for C++ remote service discovery)
    * libavahi (for remote service discovery)
    * libuv (for threading abstraction)
    * libcurl (used in framwork for setup and (among others) in remote services
	

For Ubuntu 22.04, use the following commands:

```bash
sudo apt-get update
sudo apt-get install --no-install-recommends \
  build-essential \
  ninja-build \
  curl \
  uuid-dev \
  libzip-dev \
  libjansson-dev \
  libcurl4-openssl-dev \
  libbenchmark-dev \
  libuv1-dev \
  cmake \
  libffi-dev \
  libxml2-dev \
  rapidjson-dev \
  libavahi-compat-libdnssd-dev \
  ccache
```

For OSX systems with brew installed, use the following commands:
```bash
brew update && \
brew install lcov jansson rapidjson libzip ccache ninja openssl@1.1 google-benchmark libuv 
``` 

Use CMake to configure and build Apache Celix (prefer CMake's --build and --install helper commands over raw make):
```bash
cd celix
# configure using CMake and Ninja generator
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja -S . -B build
# build using CMake
cmake --build build --parallel
# (optional) install
sudo cmake --install build
```

## Editing Build options
With use of CMake, Apache Celix makes it possible to edit build options. This enabled users, among other options, 
to configure a install location and select additional bundles. 

```bash
cd celix/build
ccmake .
#Edit options, e.g. enable BUILD_REMOTE_SHELL to build the remote (telnet) shell
#Edit the CMAKE_INSTALL_PREFIX config to set the install location
```

For this guide we assume the CMAKE_INSTALL_PREFIX is `/usr/local`.

## Installing Apache Celix

```bash
cmake --build build --parallel
sudo cmake --install build
```

## Running Apache Celix
If Apache Celix is successfully installed running
```bash
celix
```
should give the following output:
"Error: invalid or non-existing configuration file: 'config.properties'.No such file or directory".

For more info how to build your own projects and/or running the Apache Celix examples see [Celix Intro](../README.md).

# Building etcdlib library standalone
```bash
#bash
git clone git@github.com:apache/celix.git
cd celix
# Configure the build from the top-level source dir, but point CMake to the etcdlib source
cmake -S libs/etcdlib -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
sudo cmake --install build
```

# Building Celix Promises library standalone
```bash
#bash
git clone git@github.com:apache/celix.git
cd celix
# Configure the build from the top-level source dir, but point CMake to the promises source
cmake -S libs/promises -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
sudo cmake --install build
```

# Building Celix Push Streams library standalone
```bash
#bash
git clone git@github.com:apache/celix.git
cd celix
# Configure the build from the top-level source dir, but point CMake to the pushstreams source
cmake -S libs/pushstreams -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
sudo cmake --install build
```

# Further Reading

- [Building with CLion](dev_celix_with_clion.md)
- [Building and Running Tests](testing.md)
- [Fuzz Testing](fuzz_testing.md)
- [Building and Running Benchmarks](benchmarks.md)
