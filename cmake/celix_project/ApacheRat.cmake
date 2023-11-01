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


#Check if Apache RAT is enabled.
#Apache RAT can be used to check for missing license text in the project files.
option(ENABLE_APACHE_RAT "Enables Apache Release Audit tool" FALSE)
if (ENABLE_APACHE_RAT)
    find_package(Java COMPONENTS Runtime)
    if(Java_Runtime_FOUND)
        include(FetchContent)
        set(APACHE_RAT_URL "http://archive.apache.org/dist/creadur/apache-rat-0.14/apache-rat-0.14-bin.tar.gz")
        set(APACHE_RAT_HASH "17119289839dc573dd29039ca09bd86f729f1108308f6681292125418fd7bceeaf7d1a40b83eb01daf7d3dd66fbcc0a68d5431741314e748f7b878e8967459e9") #matching hash for the apache rat download

        FetchContent_Declare(download_apache_rat
            URL ${APACHE_RAT_URL}
            URL_HASH SHA512=${APACHE_RAT_HASH}
        )
        FetchContent_MakeAvailable(download_apache_rat)
        set(APACHE_RAT "${download_apache_rat_SOURCE_DIR}/apache-rat-*.jar")

        add_custom_target(rat
            COMMAND ${Java_JAVA_EXECUTABLE} -jar ${APACHE_RAT} -E rat-excludes.txt -d .
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

    else(Java_Runtime_FOUND)
        MESSAGE(STATUS "Java not found, cannot execute Apache RAT checks")
    endif(Java_Runtime_FOUND)
endif ()
