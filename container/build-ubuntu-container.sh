#!/bin/bash
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Build a Celix dev container with all needed dependencies pre-installed.

SCRIPT_LOCATION=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
CELIX_REPO_ROOT=$(realpath "${SCRIPT_LOCATION}/..")

# Check which container engine is available.
# Check for podman first, because the 'podman-docker' package might be installed providing a dummy 'docker' command.
if command -v podman > /dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
else
    CONTAINER_ENGINE="docker"
fi

cd "${SCRIPT_LOCATION}"
${CONTAINER_ENGINE} build -t apache/celix-dev:ubuntu-latest -f Containerfile.ubuntu .

