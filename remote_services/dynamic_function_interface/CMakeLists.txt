#
# Licensed under Apache License v2. See LICENSE for more information.
#
find_package(Jansson REQUIRED)
find_package(CppUTest REQUIRED)

#set(FFI_LIB /lib64/libffi.so) #TODO add findPackag for lib ffi
set(FFI_LIB /opt/local/lib/libffi.dylib) 
set(FFI_INCLUDE /opt/local/lib/libffi-3.2.1/include)

include_directories( 
    ${CPPUTEST_INCLUDE_DIR}
    ${JANSSON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}
    ${FFI_INCLUDE}
    memstream
)

set(MEMSTREAM_SOURCES "")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") 
	set(MEMSTREAM_SOURCES memstream/open_memstream.c memstream/fmemopen.c)
endif()

add_library(dfi
    dyn_type.c
    dyn_function.c
    json_serializer.c
    avro_descriptor_translator.c
    ${MEMSTREAM_SOURCES}
)
target_link_libraries(dfi ${FFI_LIB} ${JANSSON_LIBRARY})


#if (FRAMEWORK_TESTS)
	add_executable(dfi_tests
	    tst/dyn_type_tests.cpp
	    tst/dyn_function_tests.cpp
	    tst/dyn_closure_tests.cpp
	    tst/json_serializer_tests.cpp
	    tst/avro_descriptor_translator_tests.cpp
	    tst/run_tests.cpp
	)
	target_link_libraries(dfi_tests dfi ${FFI_LIB} ${CPPUTEST_LIBRARY} ${JANSSON_LIBRARY}) 

	add_custom_target(copy-input 
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/schemas schemas
	)
	add_dependencies(dfi_tests copy-input)

	ADD_TARGET_FOR_TEST(dfi_tests)
	SETUP_TARGET_FOR_COVERAGE(dfi_tests_cov dfi_tests ${CMAKE_BINARY_DIR}/coverage/dfi)
#endif()
