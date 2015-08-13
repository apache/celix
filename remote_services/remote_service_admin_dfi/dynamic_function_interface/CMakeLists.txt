#
# Licensed under Apache License v2. See LICENSE for more information.
#
include_directories(
	.
)

set(MEMSTREAM_SOURCES "")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") 
	set(MEMSTREAM_SOURCES memstream/open_memstream.c memstream/fmemopen.c)
endif()

add_library(dfi STATIC
    dyn_common.c
    dyn_type.c
    dyn_function.c
    dyn_interface.c
    json_serializer.c
    ${MEMSTREAM_SOURCES}
)
target_link_libraries(dfi ${FFI_LIBRARIES} ${JANSSON_LIBRARY})