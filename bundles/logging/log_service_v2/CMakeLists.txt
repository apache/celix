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

add_celix_bundle(log_service
	SYMBOLIC_NAME "apache_celix_log_service"
	NAME "Apache Celix Log Service"
	FILENAME celix_log_service
	GROUP "Celix/Logging"
	VERSION "1.1.0"
	SOURCES
		#v2 api
		src/log.c
		src/log_entry.c
		src/log_factory.c
		src/log_service_impl.c
		src/log_service_activator.c
		src/log_reader_service_impl.c
)
target_include_directories(log_service PRIVATE src)
target_link_libraries(log_service PRIVATE Celix::log_service_api)
install_celix_bundle(log_service EXPORT celix COMPONENT logging)

add_celix_bundle(log_writer_stdout
	SYMBOLIC_NAME "apache_celix_log_writer_stdout"
	NAME "Apache Celix Log Writer Stdout"
	FILENAME celix_log_writer_stdout
	GROUP "Celix/Logging"
	VERSION "1.1.0"
	SOURCES
		#v2 api
		src/deprecated_log_writer_stdout_activator.c
)
install_celix_bundle(log_writer_stdout EXPORT celix COMPONENT logging)

#Setup target aliases to match external usage
add_library(Celix::log_service ALIAS log_service)
add_library(Celix::log_writer_stdout ALIAS log_writer_stdout)
