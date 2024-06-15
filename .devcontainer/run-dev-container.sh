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
CELIX_CONTAINER_ROOT="/home/celixdev/workspace"

CONTAINER_SSH_PORT=2233
CONTAINER_COMMAND_DEFAULT="sudo /usr/sbin/sshd -D -e -p ${CONTAINER_SSH_PORT}"
CONTAINER_COMMAND=${1:-${CONTAINER_COMMAND_DEFAULT}}
CONTAINER_NAME="celixdev"

# Check which container engine is available.
# Check for podman first, because the 'podman-docker' package might be installed providing a dummy 'docker' command.
if command -v podman > /dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
else
    CONTAINER_ENGINE="docker"
fi

ask_and_execute() {
  local QUESTION="$1"
  local FUNC_NAME="$2"

  # Prompt the user with the question
  while true; do
    read -p "${QUESTION} (yes/no): " yn
    case $yn in
        [Yy]* )
            # Call the provided function if the answer is yes
            ${FUNC_NAME}
            echo ""
            break;;
        [Nn]* )
            # Exit if the answer is no
            break;;
        * )
            echo "Please answer yes or no.";;
    esac
  done
}

remove_celixdev_container() {
  echo "Removing container '${CONTAINER_NAME}'"
  ${CONTAINER_ENGINE} rm -f ${CONTAINER_NAME}
}
# Check if container celixdev already exists
if [ "$(${CONTAINER_ENGINE} ps -a --format '{{.Names}}' | grep ${CONTAINER_NAME})" ]; then
    ask_and_execute "Container '${CONTAINER_NAME}' already exists. Do you want to remove it?" remove_celixdev_container
fi

# Check again if container celixdev already exists and exit if it does
if [ "$(${CONTAINER_ENGINE} ps -a --format '{{.Names}}' | grep ${CONTAINER_NAME})" ]; then
    echo "Container '${CONTAINER_NAME}' already exists. Exiting."
    exit 1
fi

ADDITIONAL_ARGS=""

GNUPG_DIR_MOUNTED=false
add_gnupg_mount() {
  echo "Adding .gnupg directory mount arguments"
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} --volume ${HOME}/.gnupg:/home/celixdev/.gnupg:O"
  GNUPG_DIR_MOUNTED=true
}
if [ -e "${HOME}/.gnupg" ]; then
  ask_and_execute "Do you want to mount the .gnupg directory to the container (as an overlayfs)?" add_gnupg_mount
fi

add_gitconfig_mount() {
  echo "Adding .gitconfig file mount arguments"
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} --volume ${HOME}/.gitconfig:/home/celixdev/.gitconfig:ro"
}
if [ -e "${HOME}/.gitconfig" ]; then
  ask_and_execute "Do you want to mount the .gitconfig file to the container (as read-only)?" add_gitconfig_mount
fi

add_sshagent_forward()
{
  echo "Adding SSH agent forwarding"
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} --volume ${SSH_AUTH_SOCK}:/ssh-agent --env SSH_AUTH_SOCK=/ssh-agent"
}
if [ -n "${SSH_AUTH_SOCK}" ]; then
  ask_and_execute "Do you want to forward the SSH agent to the container?" add_sshagent_forward
fi

# Start a container with all the Celix dependencies pre-installed
#   --userns=keep-id        is used to keep the user id the same in the container as on the host
#   --privileged            to allow the unit tests to change thread priorities
#   -d                      runs the container in detached mode
#   -it                     starts the container in interactive mode
#   --rm                    removes the container when it is stopped
#   --net=host              is used to allow e.g. communication with etcd
#   --volume & --workdir    are set to the Celix repo root (to allow building and editing of the Celix repo)
#   --security-opt          disables SELinux for the container
echo "Starting container '${CONTAINER_NAME}' with command: ${CONTAINER_COMMAND}"
${CONTAINER_ENGINE} run -it --rm --privileged -d \
                        --name ${CONTAINER_NAME} \
                        --userns=keep-id \
                        --net=host \
                        ${ADDITIONAL_ARGS} \
                        --volume "${CELIX_REPO_ROOT}":${CELIX_CONTAINER_ROOT} \
                        --workdir "${CELIX_CONTAINER_ROOT}" \
                        --security-opt label=disable \
                        apache/celix-conan-dev:latest bash -c "${CONTAINER_COMMAND}"
echo ""

build_conan_deps() {
  echo "Building Celix dependencies with Conan"
  ${CONTAINER_ENGINE} exec -it ${CONTAINER_NAME} bash -c "cd ${CELIX_CONTAINER_ROOT} && .devcontainer/setup-project-with-conan.sh"
}
ask_and_execute "Do you want to build Celix dependencies with Conan?" build_conan_deps

echo "Done."
