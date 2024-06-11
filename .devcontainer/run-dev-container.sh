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

CONTAINER_COMMAND_DEFAULT="sudo /usr/sbin/sshd -D -e -p 2233"
CONTAINER_COMMAND=${1:-${CONTAINER_COMMAND_DEFAULT}}

# Check which container engine is available.
# Check for podman first, because the 'podman-docker' package might be installed providing a dummy 'docker' command.
if command -v podman > /dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
else
    CONTAINER_ENGINE="docker"
fi

# Check if container celix-dev already exists
if [ "$(${CONTAINER_ENGINE} ps -a --format '{{.Names}}' | grep celix-dev)" ]; then
    echo "Container 'celix-dev' already exists. Do you want to remove it?"
    select yn in "Yes" "No"; do
        case $yn in
            Yes ) echo "Removing container celix-dev"; ${CONTAINER_ENGINE} rm -f celix-dev; break;;
            No ) exit;;
        esac
    done
    echo ""
fi

ADDITIONAL_ARGS=""
echo "Do you want to mount the .ssh directory to the container (as an overlayfs)?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) echo "Add .ssh directory mount arguments"; ADDITIONAL_ARGS="--volume ${HOME}/.ssh:/home/celixdev/.ssh:O"; break;;
        No ) break;;
    esac
done
echo ""

if [ -e "${HOME}/.gnupg" ]; then
  echo "Do you want to mount the .gnupg directory to the container (as an overlayfs)?"
  select yn in "Yes" "No"; do
      case $yn in
          Yes ) echo "Add .gnupg directory mount arguments"; ADDITIONAL_ARGS="--volume ${HOME}/.gnupg:/home/celixdev/.gnupg:O"; break;;
          No ) break;;
      esac
  done
  echo ""
fi

# Start a container with all the Celix dependencies pre-installed
#   --userns=keep-id        is used to keep the user id the same in the container as on the host
#   --privileged            to allow the unit tests to change thread priorities
#   --net=host              is used to allow e.g. communication with etcd
#   --volume & --workdir    are set to the Celix repo root (to allow building and editing of the Celix repo)
#   --security-opt          disables SELinux for the container
#   -d                      runs the container in detached mode
echo "Starting container 'celix-dev' with command: ${CONTAINER_COMMAND}"
${CONTAINER_ENGINE} run -it --rm --privileged -d \
                        --name celix-dev \
                        --userns keep-id \
                        --net=host \
                        ${ADDITIONAL_ARGS} \
                        --volume "${CELIX_REPO_ROOT}":"${CELIX_REPO_ROOT}" \
                        --workdir "${CELIX_REPO_ROOT}" \
                        --security-opt label=disable \
                        apache/celix-conan-dev:latest bash -c "${CONTAINER_COMMAND}"
echo ""

echo "Do you want to setup the git user and email in the container?"
USER_NAME=$(git config user.name)
USER_EMAIL=$(git config user.email)
select yn in "Yes" "No"; do
    case $yn in
        Yes ) echo "Setting up git user and email"; ${CONTAINER_ENGINE} exec celix-dev bash -c "git config --global user.email '${USER_EMAIL}' && git config --global user.name '${USER_NAME}'"; break;;
        No ) break;;
    esac
done
echo ""

echo "Do you want to copy the ssh key to the container?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) echo "Copying ssh key (password: celixdev)"; ssh-copy-id -p 2233 celixdev@localhost; break;;
        No ) break;;
    esac
done
echo ""

echo "Done."
