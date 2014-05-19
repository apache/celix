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

SET(BUNDLE_SYMBOLICNAME "apache_celix_examples_hello_world")
SET(BUNDLE_VERSION "0.0.1")
SET(BUNDLE_NAME "Apache Celix Shell TUI")

include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("public/include")

BUNDLE_LIBRARY(org.apache.incubator.celix.helloworld.sub
    SOURCES private/src/test
    LINK_LIBRARIES celix_framework
    )

BUNDLE_PRIVATE_LIBRARY(private
    SOURCES private/src/activator
    LINK_LIBRARIES celix_framework org.apache.incubator.celix.helloworld.sub
    )
    
BUNDLE(org.apache.incubator.celix.helloworld
    ACTIVATOR "apache_celix_examples_hello_world-private"
    PRIVATE_LIBRARIES "private"
    EXPORT_LIBRARIES org.apache.incubator.celix.helloworld.sub
    )


