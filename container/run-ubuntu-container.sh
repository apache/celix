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
# Start a Celix dev container with all needed dependencies
# pre-installed already.

SCRIPT_LOCATION=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
CELIX_REPO_ROOT=$(realpath "${SCRIPT_LOCATION}/..")

CONTAINER_COMMAND_DEFAULT="/usr/sbin/sshd -D -e -f /etc/ssh/sshd_config_celix"
CONTAINER_COMMAND=${1:-${CONTAINER_COMMAND_DEFAULT}}

# Check which container engine is available.
# Check for podman first, because the 'podman-docker' package might be installed providing a dummy 'docker' command.
if command -v podman > /dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
else
    CONTAINER_ENGINE="docker"
fi

# Start a container with all the Celix dependencies pre-installed
#   --privileged            to allow the unit tests to change thread priorities
#   --net=host              is used to allow e.g. communication with etcd
#   --volume & --workdir    are set to the Celix repo root (to allow building and editing of the Celix repo)
#   --security-opt          disables SELinux for the container
${CONTAINER_ENGINE} run -it --rm --privileged \
                        --net=host \
                        --volume "${CELIX_REPO_ROOT}":"${CELIX_REPO_ROOT}" \
                        --workdir "${CELIX_REPO_ROOT}" \
                        --security-opt label=disable \
                        apache/celix-dev:ubuntu-latest ${CONTAINER_COMMAND}

