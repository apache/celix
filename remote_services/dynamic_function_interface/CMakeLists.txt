#
# Licensed under Apache License v2. See LICENSE for more information.
#
find_package(Jansson REQUIRED)
find_package(CppUTest REQUIRED)
set(FFI_LIB /lib64/libffi.so) #TODO add findPackag for lib ffi

include_directories( 
    ${CPPUTEST_INCLUDE_DIR}
    ${JANSSON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(dfi
    dyn_type.c
    dyn_function.c
    json_serializer.c
    avro_descriptor_translator.c
)
target_link_libraries(dfi ${FFI_LIB} ${JANSSON_LIBRARY})


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

