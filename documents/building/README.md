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

* Ubuntu Bionic Beaver (20.04)
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
Apache Celix uses [CMake](https://cmake.org) as build system. CMake can generate (among others) makefiles.

### Building and installing with preinstalled libraries
The following packages (libraries + headers) should be installed on your system:

* Development Environment
    * build-essentials (gcc/g++ or clang/clang++) 
    * java or zip (for packaging bundles)
    * make (3.14 or higher)
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
    * libczmq (for PubSubAdmin ZMQ)
	

For Ubuntu 20.04, use the following commands:
```bash
#### get dependencies
sudo apt-get install -yq --no-install-recommends \
    build-essential \
    git \
    curl \
    uuid-dev \
    libjansson-dev \
    libcurl4-openssl-dev \
    default-jdk \
    libffi-dev \
    libzip-dev \
    libxml2-dev \
    libczmq-dev \
    libcpputest-dev \
    rapidjson-dev

#The cmake version for Ubuntu 20 is older than 3.14,
#use snap to install the latest cmake version
snap install cmake
```

For OSX systems with brew installed, use the following commands:
```bash
    brew update && \
    brew install lcov libffi libzip czmq rapidjson libxml2 cmake && \
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