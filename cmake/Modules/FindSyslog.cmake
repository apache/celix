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

# - Try to find Syslog
# Once done this will define
#  SYSLOG_FOUND - System has Syslog
#  JANSSON_INCLUDE_DIRS - The Syslog include directories
#  SYSLOG::lib - Imported target for Syslog

find_path(SYSLOG_INCLUDE_DIR syslog.h /usr/include)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SYSLOG DEFAULT_MSG
                                  SYSLOG_INCLUDE_DIR)

set(Syslog_INCLUDE_DIRS ${SYSLOG_INCLUDE_DIR})
mark_as_advanced(SYSLOG_INCLUDE_DIR)

if (SYSLOG_FOUND AND NOT TARGET SYSLOG::lib)
    add_library(SYSLOG::lib SHARED IMPORTED)
    set_target_properties(SYSLOG::lib PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SYSLOG_INCLUDE_DIR}"
    )
endif ()
