project(Dummy C)

cmake_minimum_required(VERSION 3.3)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

add_executable(dummy
    main.c
)
target_link_libraries(dummy ${JANSSON_LIBRARIES} gcc_s)

install(TARGETS dummy DESTINATION bin)
