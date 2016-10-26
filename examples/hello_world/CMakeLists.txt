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

if(NOT APPLE)
#Importing and exporting libraries not (yet) work under OSX.

include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("public/include")

add_library(hello_testlib SHARED
    private/src/test
)
set_library_version(hello_testlib "4.3.2") # sets target propery VERSION to 4.3.2 and SOVERSION to 4


add_library(hello_test2lib SHARED
        private/src/test2
)
set_library_version(hello_test2lib "3.3.3")

add_bundle(hello
    VERSION "1.2"
    SOURCES
        private/src/activator.c
    IMPORT_LIBRARIES hello_test2lib
)

add_bundle(hello_export
    VERSION "1.0"
    NO_ACTIVATOR
    EXPORT_LIBRARIES hello_test2lib
)

bundle_private_libs(hello
    hello_testlib
)

add_deploy(helloworld_byref
    GROUP hello
    BUNDLES hello_export hello shell shell_tui
)

add_deploy(helloworld_withcopy
    GROUP hello
    COPY #Ensures that bundles are copied in the deploy location
    BUNDLES hello_export hello shell shell_tui
)

endif()
