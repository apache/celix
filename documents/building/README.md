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

* Ubuntu Focal Fossa (20.04)
  * GCC
  * CLang
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
    * build-essentials (gcc/g++ or clang/clang++) 
    * java or zip (for packaging bundles)
    * make (3.14 or higher)
    * git
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

Configure conan default profile using automatic detection of the system
```bash
conan profile detect
```

Create Apache Celix package - and build the dependencies - in the Conan cache:
```bash
cd <celix_source_dir>
conan create . --build missing -o build_all=True   
#Optionally build with CMake->Ninja, instead of CMake->Make. Note this includes building dependencies with Ninja. 
conan create . --build missing -o build_all=True  -c tools.cmake.cmaketoolchain:generator=Ninja 
```

Note installing Apache Celix is not required when using Conan, because Conan will install the Apache Celix package
in the local Conan cache.

It is also possible to only build a selection of the Apache Celix bundles and/or libs. This can be done by providing
build options for the required parts instead of the `build_all=True` option. For example to only build the Apache Celix
framework library and the Apache Celix utils library use the following command:
```bash
conan create . --build missing -o build_framework=True -o build_utils=True
```

To see a complete overview of the available build options use the following command:
```bash
conan inspect . | grep build_
```

#### CMake Private Linking Workaround

When using Celix via Conan, you may encounter an [issue](https://github.com/apache/celix/issues/642) where libzip.so is not found by linker.
This is due to a [bug in Conan](https://github.com/conan-io/conan/issues/7192).

A workaround we adopt in Celix is adding the following to conanfile.py:

```python
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
    * java or zip (for packaging bundles)
    * make (3.19 or higher)
    * git
* Apache Celix Dependencies
    * libzip
    * uuid
    * zlib
    * curl (only initialized in the Celix framework)
    * jansson (for serialization in libdfi)
    * libffi (for libdfi)
    * libxml2 (for remote services and bonjour shell)
    * rapidjson (for C++ remote service discovery)
	

For Ubuntu 22.04, use the following commands:
```bash
#### get dependencies
sudo apt-get install -yq --no-install-recommends \
    build-essential \
    cmake \
    git \
    curl \
    uuid-dev \
    libjansson-dev \
    libcurl4-openssl-dev \
    default-jdk \
    libffi-dev \
    libzip-dev \
    libxml2-dev \
    libcpputest-dev \
    rapidjson-dev
```

For OSX systems with brew installed, use the following commands:
```bash
brew update && \
brew install lcov libffi libzip rapidjson libxml2 cmake jansson && \
brew link --force libffi
``` 

Use CMake and make to build Apache Celix
```bash
cd celix
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. 
make -j
```

## Editing Build options
With use of CMake, Apache Celix makes it possible to edit build options. This enabled users, among other options, to configure a install location and select additional bundles.
To edit the options use ccmake or cmake-gui. For cmake-gui an additional package install can be necessary (Fedora: `dnf install cmake-gui`). 

```bash
cd celix/build
ccmake .
#Edit options, e.g. enable BUILD_REMOTE_SHELL to build the remote (telnet) shell
#Edit the CMAKE_INSTALL_PREFIX config to set the install location
```

For this guide we assume the CMAKE_INSTALL_PREFIX is `/usr/local`.

## Installing Apache Celix

```bash
cd celix/build
make -j
sudo make install
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
mkdir celix/build
cd celix/build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../libs/etcdlib
make -j
sudo make install
```

# Building Celix Promises library standalone
```bash
#bash
git clone git@github.com:apache/celix.git
mkdir celix/build
cd celix/build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../libs/promises
make -j
sudo make install
```

# Building Celix Push Streams library standalone
```bash
#bash
git clone git@github.com:apache/celix.git
mkdir celix/build
cd celix/build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../libs/pushstreams
make -j
sudo make install
```

# Further Reading

- [Building with CLion](dev_celix_with_clion.md)
- [Building and Running Tests](testing.md)
- [Fuzz Testing](fuzz_testing.md)
- [Building and Running Benchmarks](benchmarks.md)
