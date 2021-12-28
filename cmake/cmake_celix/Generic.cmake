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

add_celix_bundle_dependencies(<cmake_target>
    bundles...
)
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
#TODO doc
celix_target_embed_bundle(<cmake_target>
    BUNDLE <bundle_target>
    [NAME <name>]
]]
function(celix_target_embed_bundle)
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

    if (APPLE)
        target_sources(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})
    else()
        target_sources(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})
    endif()

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
            #For linux ensure the -rdynamic linking flag is added to that symbols in the main program can be found with dlsym.
            target_link_libraries(${TARGET_NAME} PRIVATE -Wl,-rdynamic)
        endif ()
    else()
        list(APPEND CELIX_EMBEDDED_BUNDLES "embedded://${EMBED_BUNDLE_NAME}")
    endif()
    list(REMOVE_DUPLICATES CELIX_EMBEDDED_BUNDLES)
    set_target_properties(${TARGET_NAME} PROPERTIES "CELIX_EMBEDDED_BUNDLES" "${CELIX_EMBEDDED_BUNDLES}")
endfunction()

#[[
#TODO doc
]]
function(celix_target_embed_bundles)
    list(GET ARGN 0 TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    foreach (BUNDLE_TARGET_NAME IN LISTS ARGN)
        celix_target_embed_bundle(${TARGET_NAME} BUNDLE ${BUNDLE_TARGET_NAME})
    endforeach ()
endfunction()

#[[
TODO doc
Note only supports cmake bundle targets, not cmake bundle files.
celix_target_bundle_set_definition(<cmake_target>
    NAME <set_name>
    [<bundle_target1> <bundle_target2>..]
)
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

    foreach(BUNDLE_TARGET_NAME IN LISTS BUNDLES_LIST)
        celix_get_bundle_file(${BUNDLE_TARGET_NAME} BUNDLE_FILE)
        if (BUNDLES)
            set(BUNDLES "${BUNDLES},${BUNDLE_FILE}")
        else ()
            set(BUNDLES "${BUNDLE_FILE}")
        endif ()
        add_celix_bundle_dependencies(${TARGET_NAME} ${BUNDLE_TARGET_NAME})
    endforeach()

    target_compile_definitions(${TARGET_NAME} PRIVATE ${BUNDLE_SET_NAME}=\"${BUNDLES}\")
endfunction()