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

# This Project contains some example how to use Apache Celix,
# including CMake files, C examples and C++ examples.

# To build these example:
# - build and install celix
# - Create a new build project dir and cd to it
# - set the MAKE_FIND_ROOT_PATH to the install location
#   of Celix (e.g. /opt/local) if this is different from
#   /usr or /usr/local
# - cmake <loc_to_celix_src>/examples
# - make -j all

cmake_minimum_required (VERSION 3.14)
cmake_policy(SET CMP0068 NEW)
project (CelixUse C CXX ASM)
set(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_STANDARD 17)
find_package(Celix REQUIRED)
add_subdirectory(celix-examples examples)
