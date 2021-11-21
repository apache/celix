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

add_celix_bundle(char_refiningdriver
 	SYMBOLIC_NAME "apache_celix_char_refining_driver_example"
 	NAME "Apache Celix Device Access Char Refining Driver Example"
 	VERSION "0.0.1"
	SOURCES
		src/activator.c
		src/refining_driver.c
)

target_include_directories(char_refiningdriver PRIVATE src)
target_include_directories(char_refiningdriver PUBLIC include)
target_link_libraries(char_refiningdriver PRIVATE Celix::device_access_api base_driver)
target_compile_options(char_refiningdriver PRIVATE -Wno-deprecated-declarations) #char_refiningdriver still uses service listeners, refactor this

