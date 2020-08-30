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

find_package(Java COMPONENTS Runtime)

if(Java_Runtime_FOUND)
    set(APACHE_RAT "NOT_FOUND" CACHE FILEPATH "Full path to the Apache RAT JAR file")
    
    add_custom_target(rat
        COMMAND ${Java_JAVA_EXECUTABLE} -jar ${APACHE_RAT} -E rat-excludes.txt -d .
        
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else(Java_Runtime_FOUND)
    MESSAGE(STATUS "Java not found, cannot execute Apache RAT checks")
endif(Java_Runtime_FOUND)