include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("/opt/local/include")
link_directories("/opt/local/lib")

add_executable(hash_map_test hash_map_test.c)
target_link_libraries(hash_map_test framework cunit ncurses)

add_executable(hash_map_test_hash hash_map_test_hash.c)
target_link_libraries(hash_map_test_hash framework cunit ncurses)

add_executable(array_list_test array_list_test.c)
target_link_libraries(array_list_test framework cunit ncurses)

enable_testing()
#add_test (tester ${EXECUTABLE_OUTPUT_PATH}/tester)
add_test (hash_map_test ${EXECUTABLE_OUTPUT_PATH}/hash_map_test)
add_test (hash_map_test_hash ${EXECUTABLE_OUTPUT_PATH}/hash_map_test_hash)
add_test (array_list_test ${EXECUTABLE_OUTPUT_PATH}/array_list_test)