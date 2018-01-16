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

add_library(mongooselib STATIC private/src/mongoose.c)
SET_TARGET_PROPERTIES(mongooselib PROPERTIES COMPILE_FLAGS -fPIC)

SET(BUNDLE_SYMBOLICNAME "apache_celix_examples_mongoose")
SET(BUNDLE_VERSION "0.0.1")

include_directories("private/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")

if(WIN32)
	  set(LIBS wsock32)
endif(WIN32)

add_celix_bundle(mongoose
    VERSION 0.0.1
    SOURCES
        private/src/activator
        private/include/mongoose.h
)

celix_bundle_files(mongoose ${CMAKE_CURRENT_LIST_DIR}/root)

target_link_libraries(mongoose celix_framework mongooselib ${LIBS})

add_celix_container("mongoose_deploy" BUNDLES shell shell_tui log_service mongoose)
