cmake_minimum_required (VERSION 2.6)
project (Celix)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS "-std=c99")
SET(CMAKE_C_FLAGS "-D_GNU_SOURCE")

ADD_CUSTOM_TARGET(install-fw
  ${CMAKE_COMMAND}
  -D "CMAKE_INSTALL_COMPONENT=framework"
  -D "CMAKE_INSTALL_PREFIX=framework"
  -P "celix/cmake_install.cmake"
  -P "launcher/cmake_install.cmake"
  )


#include(CPackConfig.in)
include(Packaging.cmake)
add_subdirectory(celix)
add_subdirectory(dependency_manager)
add_subdirectory(launcher)
add_subdirectory(test)

add_subdirectory(shell)
add_subdirectory(shell_tui)

add_subdirectory(receiver)
add_subdirectory(receiver-2.0)
add_subdirectory(sender)

add_subdirectory(hello_world)
add_subdirectory(echo_service/client)
add_subdirectory(echo_service/server)
add_subdirectory(whiteboard)
#add_subdirectory(echo_service)

add_subdirectory(celix_test)

include(target.cmake)
enable_testing()