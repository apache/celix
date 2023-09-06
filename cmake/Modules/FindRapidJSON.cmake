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

find_path(RapidJSON_INCLUDE_DIR rapidjson.h
        PATH_SUFFIXES rapidjson)

get_filename_component(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR}/.. ABSOLUTE)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZeroMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(RapidJSON DEFAULT_MSG
        RapidJSON_INCLUDE_DIR)

mark_as_advanced(RapidJSON_INCLUDE_DIR)

if(RapidJSON_FOUND AND NOT TARGET rapidjson)
    add_library(rapidjson INTERFACE IMPORTED)
    set_target_properties(rapidjson PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
            "${RapidJSON_INCLUDE_DIRS}")
    if (NOT TARGET RapidJSON::RapidJSON)
        add_library(RapidJSON::RapidJSON ALIAS rapidjson)
    endif ()
endif()
