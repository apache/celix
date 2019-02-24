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


# - Try to find Gtest
# Once done this will define
#  GTEST_FOUND - System has GTest
#  GTEST_INCLUDE_DIRS - The GTest include directories
#  GTEST_LIBRARIES - The libraries needed to use GTest
#  GTEST_DEFINITIONS - Compiler switches required for using Jansson

set(GTEST_INCLUDE_DIR "/usr/include/gtest/")

find_library(GTEST_LIBRARY NAMES libgtest.a
             PATHS /usr/lib /usr/local/lib )

set(GTEST_LIBRARIES ${GTEST_LIBRARY} )
set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GTEST_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Gtest  DEFAULT_MSG
                                  GTEST_LIBRARY GTEST_INCLUDE_DIR)

mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARY )
