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

#[[
Add bundles as dependencies to a cmake target, so that the bundle zip files will be created before the cmake target.

```CMake
add_celix_bundle_dependencies(<cmake_target>
    bundles...
)
```

```CMake
add_celix_bundle_dependencies(my_exec my_bundle1 my_bundle2)
```
]]
function(add_celix_bundle_dependencies)
    list(GET ARGN 0 TARGET)
    list(REMOVE_AT ARGN 0)
    foreach(BUNDLE_TARGET IN LISTS ARGN)
        if (TARGET ${BUNDLE_TARGET})
            get_target_property(IMPORT ${BUNDLE_TARGET} BUNDLE_IMPORTED)
            get_target_property(CREATE_BUNDLE_TARGET ${BUNDLE_TARGET} BUNDLE_CREATE_BUNDLE_TARGET)
            if (IMPORT)
                #NOP, an imported bundle target -> handle target without DEPENDS
            else ()
                add_dependencies(${TARGET} ${CREATE_BUNDLE_TARGET})
            endif ()
        endif()
    endforeach()
endfunction()

function(install_celix_targets)
    install_celix_bundle_targets(${ARGN})
endfunction ()

#[[
Add a compile-definition with a set of comma seperated bundles paths to a target and also adds the bundles as
dependency to the target.

```CMake
celix_target_bundle_set_definition(<cmake_target>
        NAME <set_name>
        [<bundle1> <bundle2>..]
        )
```

Example:
```CMake
celix_target_bundle_set_definition(test_example NAME TEST_BUNDLES Celix::shell Celix::shell_tui)
```

The compile-definition will have the name `${NAME}` and will contain a `,` separated list of bundle paths.
The bundle set can be installed using the Celix framework util function `celix_framework_utils_installBundleSet` (C)
or `celix::installBundleSet` (C++).

Adding a compile-definition with a set of bundles can be useful for testing purpose.
]]
function(celix_target_bundle_set_definition)
    list(GET ARGN 0 TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS NAME)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(BUNDLE_SET "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    set(BUNDLES_LIST ${BUNDLE_SET_UNPARSED_ARGUMENTS})

    if (NOT BUNDLE_SET_NAME)
        message(FATAL_ERROR "Missing required NAME argument")
    endif ()

    set(BUNDLES "")

    foreach(BUNDLE IN LISTS BUNDLES_LIST)
        if (TARGET ${BUNDLE})
            celix_get_bundle_file(${BUNDLE} BUNDLE_FILE)
            add_celix_bundle_dependencies(${TARGET_NAME} ${BUNDLE})
        elseif (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            set(BUNDLE_FILE ${BUNDLE})
        else()
            message(FATAL_ERROR "Cannot add bundle `${BUNDLE}` to bundle set definition. Argument is not a path or cmake target")
        endif ()

        if (BUNDLES)
            set(BUNDLES "${BUNDLES},${BUNDLE_FILE}")
        else ()
            set(BUNDLES "${BUNDLE_FILE}")
        endif ()
    endforeach()

    target_compile_definitions(${TARGET_NAME} PRIVATE ${BUNDLE_SET_NAME}=\"${BUNDLES}\")
endfunction()

#[[
Configure the symbol visibility preset of the provided target to hidden.

This is done by setting the target properties C_VISIBILITY_PRESET to hidden, the CXX_VISIBILITY_PRESET to hidden and
VISIBILITY_INLINES_HIDDEN to ON.

```CMake
celix_target_hide_symbols(<cmake_target> [RELEASE] [DEBUG] [RELWITHDEBINFO] [MINSIZEREL])
```

Optional arguments are:
- RELEASE: hide symbols for the release build type
- DEBUG: hide symbols for the debug build type
- RELWITHDEBINFO: hide symbols for the relwithdebinfo build type
- MINSIZEREL: hide symbols for the minsizerel build type

If no optional arguments are provided, the symbols are hidden for all build types.

Example:
```CMake
celix_target_hide_symbols(my_bundle RELEASE MINSIZEREL)
```
]]
function(celix_target_hide_symbols)
    list(GET ARGN 0 BUNDLE_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
    cmake_parse_arguments(HIDE_SYMBOLS "${OPTIONS}" "" "" ${ARGN})

    set(BUILD_TYPE "")
    if (CMAKE_BUILD_TYPE)
        string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
    endif ()

    set(HIDE_SYMBOLS FALSE)
    if (NOT HIDE_SYMBOLS_RELEASE AND NOT HIDE_SYMBOLS_DEBUG AND NOT HIDE_SYMBOLS_RELWITHDEBINFO AND NOT HIDE_SYMBOLS_MINSIZEREL)
        set(HIDE_SYMBOLS TRUE)
    elseif (HIDE_SYMBOLS_RELEASE AND BUILD_TYPE STREQUAL "RELEASE")
        set(HIDE_SYMBOLS TRUE)
    elseif (HIDE_SYMBOLS_DEBUG AND BUILD_TYPE STREQUAL "DEBUG")
        set(HIDE_SYMBOLS TRUE)
    elseif (HIDE_SYMBOLS_RELWITHDEBINFO AND BUILD_TYPE STREQUAL "RELWITHDEBINFO")
        set(HIDE_SYMBOLS TRUE)
    elseif (HIDE_SYMBOLS_MINSIZEREL AND BUILD_TYPE STREQUAL "MINSIZEREL")
        set(HIDE_SYMBOLS TRUE)
    endif ()

    if (HIDE_SYMBOLS)
        set_target_properties(${BUNDLE_TARGET}
                PROPERTIES
                C_VISIBILITY_PRESET hidden
                CXX_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN ON)
    endif ()
endfunction()
