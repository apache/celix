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


find_path(UUID_INCLUDE_DIR uuid/uuid.h
          /usr/include
          /usr/local/include )

find_library(UUID_LIBRARY NAMES uuid
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 /lib/i386-linux-gnu /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu)

include(FindPackageHandleStandardArgs)
if (APPLE)
find_package_handle_standard_args(UUID  DEFAULT_MSG
                                  UUID_INCLUDE_DIR)
else ()
find_package_handle_standard_args(UUID  DEFAULT_MSG
                                  UUID_LIBRARY UUID_INCLUDE_DIR)
endif ()


mark_as_advanced(UUID_INCLUDE_DIR UUID_LIBRARY)
set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
set(UUID_LIBRARIES ${UUID_LIBRARY})

