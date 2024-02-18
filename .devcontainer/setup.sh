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

BUILD_TYPE=${1:-Debug}
LC_BUILD_TYPE=$(echo ${BUILD_TYPE} | tr '[:upper:]' '[:lower:]')

mkdir -p cmake-build-${LC_BUILD_TYPE}

cmake -S . \
      -B cmake-build-${LC_BUILD_TYPE} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DCMAKE_INSTALL_PREFIX=/tmp/celix-install \
      -DENABLE_TESTING=ON \
      -DENABLE_ADDRESS_SANITIZER=ON \
      -DRSA_JSON_RPC=ON \
      -DRSA_SHM=ON \
      -DRSA_REMOTE_SERVICE_ADMIN_SHM_V2=ON
