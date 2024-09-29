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

set(LOG_ADMIN_SRC
	src/celix_log_admin.c
	src/celix_log_admin_activator.c
)

set(LOG_ADDMIN_PRIVATE_DEPS Celix::log_service_api Celix::shell_api)

add_celix_bundle(log_admin
	SYMBOLIC_NAME "apache_celix_log_admin"
	NAME "Apache Celix Log Admin"
	GROUP "Celix/Logging"
	VERSION "1.1.0"
	SOURCES  ${LOG_ADMIN_SRC}
	FILENAME celix_log_admin
)
target_link_libraries(log_admin PRIVATE ${LOG_ADDMIN_PRIVATE_DEPS})
target_include_directories(log_admin PRIVATE src)
celix_deprecated_utils_headers(log_admin)
install_celix_bundle(log_admin EXPORT celix COMPONENT logging)

#Setup target aliases to match external usage
add_library(Celix::log_admin ALIAS log_admin)

if (ENABLE_TESTING)
	add_library(log_admin_cut STATIC ${LOG_ADMIN_SRC})
	target_link_libraries(log_admin_cut PRIVATE ${LOG_ADDMIN_PRIVATE_DEPS} Celix::framework)
	add_subdirectory(gtest)
endif()
