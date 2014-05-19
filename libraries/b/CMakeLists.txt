cmake_minimum_required(VERSION 2.6)

add_library(b SHARED b.c)

target_link_libraries(b a)
include_directories("${PROJECT_SOURCE_DIR}/libraries/a")