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

# - Config file for the Apache Celix Promise library
# It defines the following variables
#  Celix::Promise CMake imported target

# relative install dir from lib/CMake/CelixPromise.
get_filename_component(REL_INSTALL_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)

include("${REL_INSTALL_DIR}/share/CelixPromises/cmake/Targets.cmake") #imports lib and exe targets (e.g. Celix::framework)