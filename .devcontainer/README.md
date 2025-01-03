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

Although Apache Celix can be built using CMake with APT-installed dependencies or Conan with Conan-installed/built
dependencies, this DevContainer setup currently only supports Conan.

Please note, the DevContainer setup is not broadly tested and might not work on all systems.
It has been tested on Ubuntu 23.10 and Fedora 40.

## VSCode Usage

VSCode has built-in support for DevContainers.
Simply launch VSCode using the Celix workspace folder, and you will be prompted to open the workspace in a container.

VSCode ensures that your host `.gitconfig` file, `.gnupg` directory, and SSH agent forwarding are available in the
container.

## CLion Usage

At the time of writing this readme, CLion does not fully support DevContainers,
but there is some support focusing on Docker. Using a container and remote development via SSH with CLion works.

To start developing in a container with build a CevContainer image using the `build-devcontainer-image.sh` script
and then start a container with SSHD running and interactively set up `.gitconfig`, `.gnupg`, and SSH agent
forwarding, using the `.devcontainer/run-dev-container.sh` script:

```bash
cd ${CELIX_ROOT}
./.devcontainer/build-devcontainer-image.sh
./.devcontainer/run-devcontainer.sh
ssh -p 2233 celixdev@localhost
```

In CLion, open the Remote Development window by navigating to "File -> Remote Development..." and add a new
configuration. When a new configuration is added, you can start a new project using `/home/celixdev/workspace` as the
project root and selecting CLion as the IDE.

Also ensure the CMake profile from the - conan generated - `CMakeUserPresets.json` is used: Enable the profile in the 
Settings -> "Build, Execution, Deployment" -> CMake menu.

## Running tests
Tests can be run using ctest.
When building with conan, the conanrun.sh script will setup the environment for the
built dependencies. To run the tests, execute the following commands:

```shell
cd build
ctest --output-on-failure --test-command ./workspaces/celix/build/conanrun.sh
```
