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

add_celix_bundle(driver_locator
	SYMBOLIC_NAME "apache_celix_driver_locator"
	VERSION "0.0.2"
	NAME "Apache Celix Device Access Driver Locator"
	GROUP "Celix/DeviceAccess"
	SOURCES
		src/activator.c
		src/driver_locator.c
)

target_include_directories(driver_locator PRIVATE src)
target_link_libraries(driver_locator PRIVATE Celix::device_access_api)

#Setup target aliases to match external usage
install_celix_bundle(driver_locator EPXORT celix COMPONENT device_access)
add_library(Celix::driver_locator ALIAS driver_locator)
