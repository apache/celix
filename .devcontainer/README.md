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

# Apache Celix Development Container

## Introduction

This directory contains a [DevContainer](https://containers.dev) setup for developing Apache Celix inside a container.

Although Apache Celix can be built using CMake with APT-installed dependencies or Conan 
with Conan-installed/built dependencies, this DevContainer setup is based on Conan.

Please note, the DevContainer setup is not broadly tested and might not work on all systems.
It has been tested on MacOS 26 and Fedora 43, using podman.

## Mounts

The devcontainer uses the following container volumes to ensure dependency build and project 
building is more efficient, even after recreating a devcontainer:

- celixdev-conan-package-cache
- celixdev-conan-download-cache
- celixdev-ccache-cache

Use `podman volume rm` / `docker volume rm` to remove the volumes if you do no longer use them.

## VSCode Usage

VSCode has built-in support for DevContainers.
Launch VSCode using the Celix workspace folder, and you will be prompted to 
open the workspace in a container.

VSCode ensures that your host `.gitconfig` file and SSH agent forwarding 
are available in the container.

## CLion Usage

CLion 2025.3.1 includes DevContainer support (including Podman), so you can open this repository directly
using the IDE's DevContainer workflow. Once the container is built, enable and select the conan-debug (generated) 
profile.

When running "All CTests" from the CLion UI (2025.3.2), ensure that the correct ctest args are used:
 - ALT-SHIFT-F10 (`Run...`)
 - Hover `All CTests`
 - Click `Edit...`
 - Configure CTest arguments to: `--preset conan-debug --extra-verbose -j1`
 - Configure working directory to: `$ProjectFileDir$`

When running gtests from the CLion UI (2025.3.2) directly from gtest sources, ensure that the environment is 
correctly configured:
 - Click "play" button next to the line numbers
 - Click `Modify Run Configuration...`
 - Click `Edit environment variables` (file icon in the `Environment variables` textfield) 
 - Click `Browse` from "Load variables from file"
 - Browse to `build/Debug/generators/conanrun.sh`
 - Click `OK`, Click `Apply`, Click `Ok`

## Conan Install

Run conan install to install required dependencies and tools and generate the needed cmake configuration files

```shell
conan install . --build missing --profile ${CONAN_PROFILE} ${CONAN_OPTS} ${CONAN_CONF}
```

Note: `CONAN_PROFILE`, `CONAN_OPTS` and `CONAN_CONF` are environments variables in the dev container. 

## CMake Configure

CMake configure can be done from the root workspace dir:

```shell
cmake --preset conan-debug
```

## Building

Build can be done from the root workspace dir:

```shell
cmake --build build/Debug --parallel
```

## Running tests

Tests can be run using ctest.
When building with Conan, run tests from the build directory after configuring/building:

```shell
ctest --preset conan-debug --output-on-failure -j1
```

Note `-j1` is needed to prevent ctest from running tests parallel; running tests in parallel is currently not supported
in Apache Celix. 
