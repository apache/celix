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

## Intro

TODO add intro, explain difference build, dev images and apt/conan variant.

## VSCode Usage

Support for development containers in VSCode is built-in. Just startup VSCode using the celix workspace folder, 
and you will be prompted to open the workspace in a container.

VSCode will ensure that your host .gitconfig file, .gnupg dir, and ssh agent forwarding is available in the container.

## CLion Usage

Support for devcontainers in CLion is not built-in and currently not fully supported.
For CLion you can instead use remote development via ssh.
The `.devcontainer/run-dev-container.sh` script can be used to start a container with sshd running and interactively
setup .gitconfig, .gnupg, and ssh agent forwarding.
Before `.devcontainer/run-dev-container.sh` can be used, a `celix-conan-dev` image must be build.

```bash
cd ${CELIX_ROOT}
./.devcontainer/build-dev-container.sh
./.devcontainer/run-dev-container.sh
ssh -p 2233 celixdev@localhost
```

In Clion open the Remote Development window using "File -> Remote Development ..." and add a new configuration.
