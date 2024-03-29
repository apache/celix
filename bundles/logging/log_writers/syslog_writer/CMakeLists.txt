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

add_celix_bundle(syslog_writer
		SYMBOLIC_NAME "apache_celix_syslog_writer"
		NAME "Apache Celix Syslog Writer"
		FILENAME celix_syslog_writer
		GROUP "Celix/Logging"
		VERSION "1.0.0"
		SOURCES
		src/celix_syslog_writer_activator.c
)
target_link_libraries(syslog_writer PRIVATE Celix::log_service_api)
install_celix_bundle(syslog_writer EXPORT celix COMPONENT logging)

#Setup target aliases to match external usage
add_library(Celix::syslog_writer ALIAS syslog_writer)

if (ENABLE_TESTING)
	add_subdirectory(gtest)
endif()
