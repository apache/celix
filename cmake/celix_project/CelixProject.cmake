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

option(ENABLE_ADDRESS_SANITIZER "Enabled building with address sanitizer. Note for gcc libasan must be installed," OFF)
option(ENABLE_UNDEFINED_SANITIZER "Enabled building with undefined behavior sanitizer." OFF)
option(ENABLE_THREAD_SANITIZER "Enabled building with thread sanitizer." OFF)

# Clear "Advanced" flag for sanitizer options
mark_as_advanced(CLEAR ENABLE_ADDRESS_SANITIZER)
mark_as_advanced(CLEAR ENABLE_UNDEFINED_SANITIZER)
mark_as_advanced(CLEAR ENABLE_THREAD_SANITIZER)

if (ENABLE_ADDRESS_SANITIZER)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        set(CMAKE_C_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_C_FLAGS "-lasan -fsanitize=address -fno-omit-frame-pointer ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-lasan -fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
    endif ()

    if (ENABLE_TESTING)
        set(CMAKE_C_FLAGS "-DCPPUTEST_MEM_LEAK_DETECTION_DISABLED ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-DCPPUTEST_MEM_LEAK_DETECTION_DISABLED ${CMAKE_CXX_FLAGS}")
    endif ()
endif()

if (ENABLE_UNDEFINED_SANITIZER)
    set(CMAKE_C_FLAGS "-fsanitize=undefined ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
endif()

if (ENABLE_THREAD_SANITIZER)
    set(CMAKE_C_FLAGS "-fsanitize=thread ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=thread ${CMAKE_CXX_FLAGS}")
endif()

MACRO(celix_subproject)
    set(ARGS "${ARGN}")

    list(GET ARGS 0 OPTION_NAME)
    list(REMOVE_AT ARGS 0)

    list(GET ARGS 0 OPTION_DESCRIPTION)
    list(REMOVE_AT ARGS 0)

    list(GET ARGS 0 OPTION_DEFAULT)
    list(REMOVE_AT ARGS 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS )
    set(MULTI_VAL_ARGS DEPS)
    cmake_parse_arguments(OPTION "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGS})

    string(TOUPPER ${OPTION_NAME} UC_OPTION_NAME)
    set(NAME "BUILD_${UC_OPTION_NAME}")
    
    option(${NAME} "${OPTION_DESCRIPTION}" ${OPTION_DEFAULT})
    
    IF (${NAME})
        set(${OPTION_NAME} "ON")
        FOREACH (DEP ${OPTION_DEPS})
            string(TOUPPER ${DEP} UC_DEP)
            set(DEP_NAME "BUILD_${UC_DEP}")
            if (NOT ${DEP_NAME})
                message(FATAL_ERROR "${DEP} is required by ${OPTION_NAME}: please add -D${DEP_NAME}:BOOL=ON")
            endif ()
        ENDFOREACH (DEP)
    ENDIF (${NAME})
ENDMACRO(celix_subproject)

#[[
Custom target which list the Celix CMake targets that are still using deprecated headers.
]]
if (NOT TARGET celix-deprecated)
    add_custom_target(celix-deprecated
        COMMAND ${CMAKE_COMMAND} -E echo "Targets still using deprecated utils headers: $<JOIN:$<TARGET_PROPERTY:celix-deprecated,UTIL_TARGETS>, >"
    )
    set_target_properties(celix-deprecated PROPERTIES "UTIL_TARGETS" "")
endif ()


#[[
Add include path for the Celix utils deprecated headers to the provided target (as PRIVATE)

```CMake
celix_deprecated_utils_headers(<target_name>))
```
]]
function(celix_deprecated_utils_headers)
    list(GET ARGN 0 TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libs/utils/include_deprecated)
    set_property(TARGET celix-deprecated APPEND PROPERTY "UTIL_TARGETS" "${TARGET_NAME}")
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/ApacheRat.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CodeCoverage.cmake)
