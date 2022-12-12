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
Embeds a Celix bundle into a CMake target.

```CMake
celix_target_embedded_bundle(<cmake_target>
    BUNDLE <bundle>
    [NAME <name>]
)
```

Example:
```CMake
celix_target_embedded_bundle(my_executable
    BUNDLE Celix::shell
    NAME celix_shell
)
# result in the symbols:
# - celix_embedded_bundle_celix_shell_start
# - celix_embedded_bundle_celix_shell_end
# - celix_embedded_bundles = "embedded://celix_shell"
# to be added to `my_executable`
```

The Celix bundle will be embedded into the CMake target between the symbols: `celix_embedded_bundle_${NAME}_start` and
`celix_embedded_bundle_${NAME}_end`.

Also a `const char * const` symbol with the name `celix_embedded_bundles` will be added or updated containing a `,`
seperated list of embedded Celix bundle urls. The url will be: `embedded://${NAME}`.

For Linux the linking flag `--export-dynamic` is added to ensure that the previous mentioned symbols can be retrieved
using `dlsym`.

Mandatory Arguments:
- BUNDLE: The bundle target or bundle file (absolute path) to embed in the CMake target.

Optional Arguments:
- NAME: The name to use when embedding the Celix bundle. This name is used in the _start and _end symbol, but also
  for the embedded bundle url.
  For a bundle CMake target the default is the bundle symbolic name and for a bundle file the default is the
  bundle filename without extension. The NAME must be a valid C identifier.

Bundles embedded in an executable can be installed/started using the bundle url: "embedded://${NAME}" in
combination with `celix_bundleContext_installBundle` (C) or `celix::BundleContext::installBundle` (C++).
All embedded bundle can be installed using the framework utils function
`celix_framework_utils_installEmbeddedBundles` (C) or `celix::installEmbeddedBundles` (C++).
]]
function(celix_target_embedded_bundle)
    get_property(LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    if (NOT "ASM" IN_LIST LANGUAGES)
        message(FATAL_ERROR "celix_target_embedded_bundle is only supported it the language ASM is enabled."
         " Please add ASM as cmake project language.")
    endif ()

    list(GET ARGN 0 TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS)
    set(ONE_VAL_ARGS BUNDLE NAME)
    set(MULTI_VAL_ARGS)
    cmake_parse_arguments(EMBED_BUNDLE "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    if (NOT EMBED_BUNDLE_BUNDLE)
        message(FATAL_ERROR "Missing required BUNDLE argument")
    endif ()

    if (TARGET ${EMBED_BUNDLE_BUNDLE})
        celix_get_bundle_symbolic_name(${EMBED_BUNDLE_BUNDLE} DEFAULT_NAME)
        celix_get_bundle_file(${EMBED_BUNDLE_BUNDLE} EMBED_BUNDLE_FILE)
        add_celix_bundle_dependencies(${TARGET_NAME} ${EMBED_BUNDLE_BUNDLE})
    elseif (IS_ABSOLUTE ${EMBED_BUNDLE_BUNDLE} AND EXISTS ${EMBED_BUNDLE_BUNDLE})
        get_filename_component(RAW_NAME ${EMBED_BUNDLE_BUNDLE} NAME_WE)
        string(MAKE_C_IDENTIFIER ${RAW_NAME} DEFAULT_NAME)
        set(EMBED_BUNDLE_FILE ${EMBED_BUNDLE_BUNDLE})
    else ()
        message(FATAL_ERROR "Cannot embed bundle `${EMBED_BUNDLE_BUNDLE}` to target ${TARGET_NAME}. Argument is not a path or cmake target")
    endif ()

    if (NOT EMBED_BUNDLE_NAME)
        set(EMBED_BUNDLE_NAME ${DEFAULT_NAME})
    endif()

    string(MAKE_C_IDENTIFIER ${EMBED_BUNDLE_NAME} EMBED_BUNDLE_NAME_CHECK)
    if (NOT EMBED_BUNDLE_NAME STREQUAL EMBED_BUNDLE_NAME_CHECK)
        message(FATAL_ERROR "Cannot embed bundle ${EMBED_BUNDLE_BUNDLE}, because the bundle symbolic name (${EMBED_BUNDLE_NAME}) is not a valid c identifier. Please specify an valid c identifier as embedded bundle name instead.")
    endif ()

    if (APPLE)
        set(ASSEMBLY_FILE_IN ${CELIX_CMAKE_DIRECTORY}/templates/embed_bundle_apple.s)
    else ()
        set(ASSEMBLY_FILE_IN ${CELIX_CMAKE_DIRECTORY}/templates/embed_bundle_linux.s)
    endif ()
    set(ASSEMBLY_FILE "${CMAKE_BINARY_DIR}/celix/gen/target/${TARGET_NAME}/embed_bundle_${EMBED_BUNDLE_NAME}.s")
    configure_file(${ASSEMBLY_FILE_IN} ${ASSEMBLY_FILE} @ONLY)
    target_sources(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})

    get_target_property(CELIX_EMBEDDED_BUNDLES ${TARGET_NAME} "CELIX_EMBEDDED_BUNDLES")
    if (NOT CELIX_EMBEDDED_BUNDLES)
        set(CELIX_EMBEDDED_BUNDLES "embedded://${EMBED_BUNDLE_NAME}")

        #If executable also add symbol with a ; seperated list of embedded bundles urls
        get_target_property(TYPE ${TARGET_NAME} TYPE)
        if (TYPE STREQUAL "EXECUTABLE")
            set(C_FILE "${CMAKE_BINARY_DIR}/celix/gen/target/${TARGET_NAME}/celix_embedded_bundles.c")
            file(GENERATE OUTPUT ${C_FILE}.stage1 CONTENT "const char * const celix_embedded_bundles = \"$<JOIN:$<TARGET_PROPERTY:${TARGET_NAME},CELIX_EMBEDDED_BUNDLES>,$<COMMA>>\";")
            file(GENERATE OUTPUT ${C_FILE} INPUT ${C_FILE}.stage1)
            target_sources(${TARGET_NAME} PRIVATE ${C_FILE})
        endif ()

        if (NOT APPLE)
            #For linux ensure the --export-dynamic linking flag is added to that symbols in the main program can be found with dlsym.
            target_link_libraries(${TARGET_NAME} PRIVATE -Wl,--export-dynamic)
        endif ()
    else()
        list(APPEND CELIX_EMBEDDED_BUNDLES "embedded://${EMBED_BUNDLE_NAME}")
    endif()
    list(REMOVE_DUPLICATES CELIX_EMBEDDED_BUNDLES)
    set_target_properties(${TARGET_NAME} PROPERTIES "CELIX_EMBEDDED_BUNDLES" "${CELIX_EMBEDDED_BUNDLES}")
endfunction()

#[[
Embed multiple Celix bundles into a CMake target.

```CMake
celix_target_embedded_bundles(<cmake_target> [<bundle1> <bundle2> ...])
```

Example:
```CMake
celix_target_embedded_bundles(my_executable Celix::shell Celix::shell_tui)
```

The bundles will be embedded using their symbolic name if the bundle is a CMake target or their filename (without
extension) if the bundle is a file (absolute path).
]]
function(celix_target_embedded_bundles)
    list(GET ARGN 0 TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    foreach (BUNDLE IN LISTS ARGN)
        celix_target_embedded_bundle(${TARGET_NAME} BUNDLE ${BUNDLE})
    endforeach ()
endfunction()

#[[
Add a compile definition with a set of bundles to a target.

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

The compile definition will have the name `${NAME}` and will contain a `,` separated list of bundle paths.
The bundle can be installed using the Celix framework util function `celix_framework_utils_installBundleSet` (C)
or `celix::installBundlesSet` (C++).

Adding a compile definition with a set of bundles can be useful for testing purpose.
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
        add_celix_bundle_dependencies(${TARGET_NAME} ${BUNDLE_TARGET_NAME})
    endforeach()

    target_compile_definitions(${TARGET_NAME} PRIVATE ${BUNDLE_SET_NAME}=\"${BUNDLES}\")
endfunction()
