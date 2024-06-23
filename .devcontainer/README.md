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

While Apache Celix can be built using CMake with APT-installed dependencies or using Conan with Conan-installed/built
dependencies, this DevContainer setup currently only supports building Celix using Conan.

Please note, the DevContainer setup is not broadly tested and might not work on all systems. 
It has been tested on Ubuntu 23.10.

## VSCode Usage

VSCode has built-in support for development containers. Simply start VSCode using the Celix workspace folder, and you
will be prompted to open the workspace in a container.

VSCode will ensure that your host `.gitconfig` file, `.gnupg` directory, and SSH agent forwarding are available in the
container.

## CLion Usage

CLion does not have built-in support for DevContainers and is not fully supported at this time.
Instead, you can use remote development via SSH with CLion.

To start a container with SSHD running and interactively set up `.gitconfig`, `.gnupg`, and SSH agent forwarding, use
the `.devcontainer/run-dev-container.sh` script. 
Before using this script, you must build a `celix-conan-dev` image.

```bash
cd ${CELIX_ROOT}
./.devcontainer/build-dev-container.sh
./.devcontainer/run-dev-container.sh
ssh -p 2233 celixdev@localhost
```

In CLion, open the Remote Development window by navigating to "File -> Remote Development..." and add a new
configuration. When a new configuration is added, you can start a new project using `/home/celixdev/workspace` as the
project root and
selecting CLion as the IDE.

## Running tests
Tests can be runned using ctest. When building with conan, the conanrun.sh script will setup the environment for the
built dependencies. To run the tests, execute the following commands:

```shell
cd build
ctest --output-on-failure --test-command ./workspaces/celix/build/conanrun.sh
```
