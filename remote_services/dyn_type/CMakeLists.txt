find_package(Jansson REQUIRED)

add_executable(struct_tests
    struct_tests.c
    dyn_type.c
    json_serializer.c
)

add_executable(func_tests
    func_tests.c
    dyn_type.c
    dyn_function.c
)

add_executable(closure_tests
    closure_tests.c
    dyn_type.c
    dyn_function.c
)

target_link_libraries(struct_tests ${JANSSON_LIBRARIES} /lib64/libffi.so ) 
target_link_libraries(func_tests ${JANSSON_LIBRARIES} /lib64/libffi.so ) 
target_link_libraries(closure_tests ${JANSSON_LIBRARIES} /lib64/libffi.so ) 
