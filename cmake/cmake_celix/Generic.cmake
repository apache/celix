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
${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE_TARGET_NAME}O
#0 target
#1 bundle target
#3 optional embed id
]]
#TODO update to use command (BUNDLE_TARGET EMBED_NAME)
function(celix_target_embed_bundle)
    #0 is the target name
    list(GET ARGN 0 TARGET_NAME)
    list(GET ARGN 1 BUNDLE_TARGET_NAME)
    if (ARGC GREATER_EQUAL 3)
        list(GET ARGN 2 EMBED_BUNDLE_NAME)
    endif ()

    if (NOT EMBED_BUNDLE_NAME)
        celix_get_bundle_symbolic_name(${BUNDLE_TARGET_NAME} EMBED_BUNDLE_NAME)
        if (NOT EMBED_BUNDLE_NAME)
            message(FATAL_ERROR "Cannot find bundle symbolic name from bundle target ${BUNDLE_TARGET_NAME}.")
        endif ()
    endif()

    string(MAKE_C_IDENTIFIER ${EMBED_BUNDLE_NAME} EMBED_BUNDLE_NAME_CHECK)
    if (NOT EMBED_BUNDLE_NAME STREQUAL EMBED_BUNDLE_NAME_CHECK)
        message(FATAL_ERROR "Embedding bundle name ${BUNDLE_TARGET_NAME}, but `${EMBED_BUNDLE_NAME}` is not a valid c identifier. Please specify an valid c identifier as embedded bundle name")
    endif ()

    if (APPLE)
        set(ASSEMBLY_FILE_IN ${CELIX_CMAKE_DIRECTORY}/templates/embed_bundle_apple.s)
    else ()
        set(ASSEMBLY_FILE_IN ${CELIX_CMAKE_DIRECTORY}/templates/embed_bundle_linux.s)
    endif ()
    celix_get_bundle_file(${BUNDLE_TARGET_NAME} EMBED_BUNDLE_FILE)
    set(ASSEMBLY_FILE "${CMAKE_BINARY_DIR}/celix/gen/target/${TARGET_NAME}/embed_bundle_${EMBED_BUNDLE_NAME}.s")
    configure_file(${ASSEMBLY_FILE_IN} ${ASSEMBLY_FILE} @ONLY)

    #NOTE assuming use of PRIVATE/PUBLIC, etc -> TODO can this be checked?
    if (APPLE)
        target_sources(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})
    else()
        target_sources(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})
        target_link_libraries(${TARGET_NAME} PRIVATE ${ASSEMBLY_FILE})
    endif()

    get_target_property(CELIX_EMBEDDED_BUNDLES ${TARGET_NAME} "CELIX_EMBEDDED_BUNDLES")
    if (NOT CELIX_EMBEDDED_BUNDLES)
        set(CELIX_EMBEDDED_BUNDLES "embedded://${EMBED_BUNDLE_NAME}")
        set(C_FILE_IN "${CELIX_CMAKE_DIRECTORY}/templates/celix_embedded_bundles.c.in")
        set(C_FILE "${CMAKE_BINARY_DIR}/celix/gen/target/${TARGET_NAME}/celix_embedded_bundles.c")
        file(GENERATE OUTPUT ${C_FILE}.stage1 CONTENT "const char * const CELIX_EMBEDDED_BUNDLES = \"$<JOIN:$<TARGET_PROPERTY:${TARGET_NAME},CELIX_EMBEDDED_BUNDLES>,;>\";")
        file(GENERATE OUTPUT ${C_FILE} INPUT ${C_FILE}.stage1)
        target_sources(${TARGET_NAME} PRIVATE ${C_FILE})
    else ()
        list(APPEND CELIX_EMBEDDED_BUNDLES "${EMBED_BUNDLE_NAME}")
    endif()
    set_target_properties(${TARGET_NAME} PROPERTIES "CELIX_EMBEDDED_BUNDLES" "${CELIX_EMBEDDED_BUNDLES}")

endfunction()

function(celix_target_embed_bundles)
    list(GET ARGN 0 TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    foreach (BUNDLE_TARGET_NAME IN LISTS ARGN)
        celix_target_embed_bundle(${TARGET_NAME} ${BUNDLE_TARGET_NAME})
    endforeach ()
endfunction()