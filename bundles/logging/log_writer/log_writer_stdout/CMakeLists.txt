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

add_celix_bundle(log_writer_stdout
	SYMBOLIC_NAME "apache_celix_log_writer"
	VERSION "1.1.0"
	NAME "Apache Celix Log Writer"
	GROUP "Celix/Logging"
	SOURCES
		src/log_writer_stdout
)

IF(APPLE)
	target_link_libraries(log_writer_stdout PRIVATE -Wl,-all_load log_writer_common)
else()
	if(ENABLE_ADDRESS_SANITIZER)
		#With asan there can be undefined symbols
		target_link_libraries(log_writer_stdout PRIVATE -Wl,--whole-archive log_writer_common -Wl,--no-whole-archive)
	else()
		target_link_libraries(log_writer_stdout PRIVATE -Wl,--no-undefined -Wl,--whole-archive log_writer_common -Wl,--no-whole-archive)
	endif()
endif()

target_link_libraries(log_writer_stdout PRIVATE Celix::log_service_api)

install_celix_bundle(log_writer_stdout EXPORT celix)
add_library(Celix::log_writer_stdout ALIAS log_writer_stdout)

