find_package(Jansson REQUIRED)

add_executable(struct_tests
    struct_tests.c
    dyn_type.c
    json_serializer.c
)

target_link_libraries(struct_tests ${JANSSON_LIBRARIES} /lib64/libffi.so ) 
