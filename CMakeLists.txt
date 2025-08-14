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

cmake_minimum_required (VERSION 3.19)
cmake_policy(SET CMP0012 NEW)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0068 NEW)

project (Celix C CXX ASM)

include(cmake/celix_project/CelixProject.cmake)
include(cmake/cmake_celix/UseCelix.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#find required packages
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Options
option(ENABLE_TESTING "Enables unit/bundle testing" FALSE)
if (ENABLE_TESTING)
    find_package(GTest QUIET)
    if (NOT GTest_FOUND)
        include(AddGTest)
    endif()
endif ()


option(ENABLE_CCACHE "Enabled building with ccache" FALSE)
if (ENABLE_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    if(CCACHE_EXECUTABLE)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
endif ()

# Set C specific flags
set(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 -fPIC ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wall -Werror -Wformat -Wno-error=deprecated-declarations ${CMAKE_C_FLAGS}")

# Set C++ specific flags
set(CMAKE_CXX_FLAGS "-fno-rtti ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -Weffc++ -Wformat -Wno-error=deprecated-declarations ${CMAKE_CXX_FLAGS}")

if (NOT DEFINED CMAKE_CXX_STANDARD)
    #Celix header only C++ supported is based on C++14. However, test code can be and is written in C++17.
    #Default using C++14 to ensure the code compiles for C++14.

    #There are 2 exceptions: The libraries Promises and PushStream are C++17 only.

    #To ensure IDE like CLion can provide optimal C++17 support, the CMAKE_CXX_STANDARD variable is only set if
    #not already defined.
    set(CMAKE_CXX_STANDARD 14)
endif ()

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_CXX_FLAGS "-Wno-missing-template-arg-list-after-template-kw ${CMAKE_CXX_FLAGS}")
endif()

if(NOT APPLE)
    set(CMAKE_C_FLAGS "-pthread ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-pthread ${CMAKE_EXE_LINKER_FLAGS}")

    #prevent the linker from optimizing out linked libraries. libraries linked against executable are maybe not used
    #by the executable, but could be used by the (dynamically) loaded libraries.
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
endif()

# Set compiler specific options
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "-Wno-nullability-completeness -Wno-expansion-to-defined ${CMAKE_C_FLAGS}")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-Wno-unused-result ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-Wno-unused-result ${CMAKE_CXX_FLAGS}")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7.0)
        set(CMAKE_C_FLAGS "-Wno-format-truncation -Wno-stringop-overflow ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-Wno-format-truncation -Wno-stringop-overflow ${CMAKE_CXX_FLAGS}")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
        set(CMAKE_C_FLAGS "-Wno-stringop-truncation ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-Wno-stringop-truncation ${CMAKE_CXX_FLAGS}")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11.0)
        set(CMAKE_C_FLAGS "-Wno-error=stringop-overread ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-Wno-error=stringop-overread ${CMAKE_CXX_FLAGS}")
    endif()
endif()

option(ENABLE_MORE_WARNINGS "whether to enable more compiler warnings." OFF)
if (ENABLE_MORE_WARNINGS)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4)
            set(CMAKE_CXX_EXTRA_FLAGS "-Wlogical-op -Wold-style-cast -Wuseless-cast -Wdouble-promotion -Wshadow -Wformat=2")
        endif()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6)
            set(CMAKE_CXX_EXTRA_FLAGS "-Wduplicated-cond -Wnull-dereference ${CMAKE_CXX_EXTRA_FLAGS}")
        endif()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7)
            set(CMAKE_CXX_EXTRA_FLAGS "-Wrestrict -Wduplicated-branches ${CMAKE_CXX_EXTRA_FLAGS}")
        endif()
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.5)
            set(CMAKE_CXX_EXTRA_FLAGS "-Wlogical-op -Wold-style-cast -Wshadow -Wformat=2 -Wnull-dereference")
        endif()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4)
            set(CMAKE_CXX_EXTRA_FLAGS "-Wold-style-cast -Wuseless-cast ${CMAKE_CXX_EXTRA_FLAGS}")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_EXTRA_FLAGS} ${CMAKE_CXX_FLAGS}")
endif()

# Set build type specific flags
# Debug
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -O0 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -O0 ${CMAKE_CXX_FLAGS}")
set(CMAKE_DEBUG_POSTFIX "d")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXECUTABLE_SUFFIX ${CMAKE_DEBUG_POSTFIX})
endif()

# Set version for the framework package/release
if (NOT DEFINED CELIX_MAJOR)
    set(CELIX_MAJOR "3")
    set(CELIX_MINOR "0")
    set(CELIX_MICRO "0")
endif ()

# Default bundle version
set(DEFAULT_VERSION 1.0.0)

if (ENABLE_TESTING)
    #Note tests are C++. Default C++14 and some C++17. For C++14 tests no additional option is needed.
    #For C++17 tests CELIX_CXX17 must be enabled.
    enable_testing()
endif()

option(CELIX_USE_ZIP_INSTEAD_OF_JAR "Default Celix cmake command will use jar to package bundle (if found). This option enforces Celix to use zip instead." OFF)

option(CELIX_CXX14 "Build C++14 libraries and bundles." ON)
option(CELIX_CXX17 "Build C++17 libraries, bundles and if testing is enabled C++17 tests" ON)
if (CELIX_CXX17 AND NOT CELIX_CXX14)
    set(CELIX_CXX14 ON)
endif ()

option(ENABLE_CMAKE_WARNING_TESTS "Enable cmake warning tests to test warning prints" OFF)
option(ENABLE_TESTING_ON_CI "Whether to enable testing on CI. This influence allowed timing errors during unit tests" OFF)
option(ENABLE_DEPRECATED_WARNINGS "Enable compiler warnings for usage of deprecated functionality" OFF)
option(ENABLE_FUZZING "Enable fuzz testing, using LibFuzzer" OFF) #Note support for LibFuzzer is built-in for Clang
option(ENABLE_BENCHMARKING "Enable benchmarking, using Google Benchmark" OFF)

if (NOT ENABLE_DEPRECATED_WARNINGS)
    set(CMAKE_C_FLAGS "-Wno-deprecated-declarations ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations ${CMAKE_CXX_FLAGS}")
endif ()

include(CMakePushCheckState)
cmake_push_check_state()
cmake_reset_check_state()
include(CheckLinkerFlag)
check_linker_flag(CXX LINKER:--wrap,celix_nonexistent_symbol LINKER_WRAP_SUPPORTED)
cmake_pop_check_state()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND LINKER_WRAP_SUPPORTED)
    set(EI_TESTS ON)
else ()
    set(EI_TESTS OFF)
endif()

# avoid unknown export "celix" error when building nothing
add_library(celix INTERFACE)
add_library(Celix::celix ALIAS celix)
install(TARGETS celix EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix)

#Add generate_export_header to cmake
include(GenerateExportHeader)

#Libraries and Launcher
add_subdirectory(libs)

#Bundles
add_subdirectory(bundles)

#Experimental Bundles/Libraries
add_subdirectory(misc/experimental)

#Example as last, because some example will check if underlining options are enabled
add_subdirectory(examples/celix-examples)

#include CMake warning tests
include(cmake/celix_project/WarningTests.cmake)

#export targets
install(EXPORT celix NAMESPACE Celix:: DESTINATION share/celix/cmake FILE Targets.cmake COMPONENT cmake)
install_celix_targets(celix NAMESPACE Celix:: DESTINATION share/celix/cmake FILE CelixTargets COMPONENT cmake)

#install celix cmake modules
option(INSTALL_FIND_MODULES "Whether to install Find modules defined by Celix" ON)
if (INSTALL_FIND_MODULES)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/ DESTINATION share/celix/cmake/Modules)
endif ()
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_celix/ DESTINATION share/celix/cmake/cmake_celix)

file(GENERATE
        OUTPUT "${PROJECT_BINARY_DIR}/celix/gen/cmake/CelixDeps.cmake"
        INPUT cmake/CelixDeps.cmake.in
        )

install(FILES
        "${PROJECT_BINARY_DIR}/celix/gen/cmake/CelixDeps.cmake"
        DESTINATION share/celix/cmake COMPONENT cmake)

#configure and install CelixConfig and CelixConfigVersion files
configure_file(cmake/CelixConfigVersion.cmake.in
        "${PROJECT_BINARY_DIR}/celix/gen/cmake/CelixConfigVersion.cmake" @ONLY)

install(FILES
        "cmake/CelixConfig.cmake"
        "${PROJECT_BINARY_DIR}/celix/gen/cmake/CelixConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Celix" COMPONENT cmake)
