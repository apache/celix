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

#############################################################################
## NOTE Examples of using add_bundle and add_deploy and assorted commands ##
#############################################################################

add_bundle(hellotest1
    VERSION "1.2"
    SOURCES
        private/src/activator
)

target_include_directories(hellotest1 PRIVATE
        ${PROJECT_SOURCE_DIR}/utils/public/include
        public/include
)


bundle_headers(hellotest1
    "X-WebContent: web"
    "X-MediaType: application/json"
)

add_library(tlib SHARED
    private/src/test
)
set_library_version(tlib "4.3.2") # sets target propery VERSION to 4.3.2 and SOVERSION to 4

#bundle_private_libs(hellotest1
#    #/usr/local/lib/libjansson.4.dylib
#    /usr/lib64/libjansson.so.4
#)
bundle_private_libs(hellotest1
    tlib
    #/usr/lib/libcurl.4.dylib 
    #    /usr/lib64/libcurl.so.4
)


add_deploy(hello_test_deploy
    BUNDLES hellotest1 hellotest2 #NOTE hellotest2 is still a unknown target
    PROPERTIES 
        "Test=true"
)

#add_deploy(hello_test_deploy_fail
#    BUNDLES nonexisting
#)

#NOTE: Gives error:
#Error evaluating generator expression:
#
#    $<TARGET_PROPERTY:nonexisting,BUNDLE_FILE>
#
#  Target "nonexisting" not found.



add_library(hello2act SHARED
    private/src/activator
)
set_library_version(hello2act "3.2.4") #sets VERSION prop to 3.2.4 and SOVERSION to 3
target_link_libraries(hello2act PRIVATE Celix::framework )
add_bundle(hellotest2
    VERSION "1.0.0" #can be the same as activator lib, but does not have to be
    ACTIVATOR hello2act
)

#add_bundle(hellotest3
#    VERSION "2.1.2"
    #ACTIVATOR /usr/local/lib/libjansson.4.dylib
    #ACTIVATOR /usr/lib64/libjansson.so.4
    #)


