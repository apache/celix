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

include_directories(
	.
)

set(MEMSTREAM_SOURCES "")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") 
	set(MEMSTREAM_SOURCES memstream/open_memstream.c memstream/fmemopen.c)
endif()

add_library(dfi SHARED
    dyn_common.c
    dyn_type.c
    dyn_function.c
    dyn_interface.c
    dyn_message.c
    json_serializer.c
    json_rpc.c
    ${MEMSTREAM_SOURCES}
)
target_link_libraries(dfi ${FFI_LIBRARIES} ${JANSSON_LIBRARY})

install(TARGETS dfi DESTINATION lib)
FILE(GLOB files "*.h")
INSTALL(FILES ${files} DESTINATION include/celix/dfi)
