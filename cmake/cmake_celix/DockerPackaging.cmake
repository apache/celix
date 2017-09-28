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

### Setup Docker Option
option(ENABLE_DOCKER "Enable the add_docker function to create docker files")

if (ENABLE_DOCKER)
    ##### setup docker target
    add_custom_target(docker ALL
        DEPENDS $<TARGET_PROPERTY:docker,DOCKER_DEPS>
    )
    add_custom_target(build-docker-images)
    set(DOCKER_USE_SUDO ON CACHE BOOL "Wether the use of sudo is needed to run docker")
    set(DOCKER_CMD "docker" CACHE STRING "Docker command to use.")

    set_target_properties(docker PROPERTIES "DOCKER_DEPS" "") #initial empty deps list

    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/docker")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
    #####


    ##### Docker Dependencies Library
    # This (dummy) dependencies library is used to link against required libraries for the docker images.
    # The create fs script will ensure that the minimal required directory / files is created and this
    # library will ensure that stdc++, m, jansson and libffi and ld-linux libaries are installed
    #
    # NOTE that this target can be used to extend the libraries added to the docker image
    file(GENERATE
            OUTPUT "${CMAKE_BINARY_DIR}/.celix_docker_depslib/docker_dummy.cc"
            CONTENT "//intentionally emtpy source file")
    add_library(celix_docker_depslib SHARED
            ${CMAKE_BINARY_DIR}/.celix_docker_depslib/docker_dummy.cc
    )
    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/.celix_docker_depslib")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
    set_target_properties(celix_docker_depslib PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.celix_docker_depslib")
    find_package(FFI REQUIRED)
    find_package(Jansson REQUIRED)
    target_link_libraries(celix_docker_depslib ${JANSSON_LIBRARIES} ${FFI_LIBRARIES})
    target_link_libraries(celix_docker_depslib m)

    #TODO, check if libnss_dns and _files are really not needed... seems so
    #target_link_libraries(celix_docker_depslib /lib64/libnss_dns.so.2 /lib64/libnss_files.so.2)
endif ()

function(add_docker)
    list(GET ARGN 0 DOCKER_TARGET)
    list(REMOVE_AT ARGN 0)

    if (NOT ENABLE_DOCKER)
        message(WARNING "Docker not enable, skipping target '${DOCKER_TARGET}'")
        return()
    endif()

    set(OPTIONS )
    set(ONE_VAL_ARGS GROUP NAME FROM BUNDLES_DIR WORKDIR IMAGE_NAME ENTRYPOINT DEPSLIB)
    set(MULTI_VAL_ARGS BUNDLES PROPERTIES INSTRUCTIONS)
    cmake_parse_arguments(DOCKER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    #set defaults
    if (NOT DOCKER_FROM)
        set(DOCKER_FROM "scratch")
        set(DOCKER_CREATE_FS true)
    else()
        set(DOCKER_CREATE_FS false)
    endif()
    if (NOT DOCKER_NAME)
        set(DOCKER_NAME "${DOCKER_TARGET}")
    endif()
    if (NOT DOCKER_IMAGE_NAME)
        set(DOCKER_IMAGE_NAME "${DOCKER_NAME}")
    endif()
    if (NOT DOCKER_WORKDIR)
        set(DOCKER_WORKDIR "/root")
    endif()
    if (NOT DOCKER_ENTRYPOINT)
        set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"/bin/celix\"]")
    else ()
        set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"${DOCKER_ENTRYPOINT}\"]")
    endif ()
    if (NOT DOCKER_GROUP)
        set(DOCKER_LOC "${CMAKE_BINARY_DIR}/docker/${DOCKER_NAME}")
    else()
        set(DOCKER_LOC "${CMAKE_BINARY_DIR}/docker/${DOCKER_GROUP}/${DOCKER_NAME}")
    endif()
    if (NOT DOCKER_BUNDLES_DIR)
        set(DOCKER_BUNDLES_DIR "/bundles")
    endif()
    if (NOT DOCKER_DEPSLIB)
        set(DOCKER_DEPSLIB "celix_docker_depslib")
    endif ()

    #ensure the docker dir will be deleted during clean
    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION>")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")

    ###### Setup docker custom target timestamp
    set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-timestamp")

    add_custom_target(${DOCKER_TARGET}
        DEPENDS ${TIMESTAMP_FILE}
    )

    #Setting CELIX_LIB_DIRS, CELIX_BIN_DIR and CELIX_LAUNCHER 
    if (EXISTS ${CELIX_FRAMEWORK_LIBRARY}) 
        #CELIX_FRAMEWORK_LIBRARY set by FindCelix.cmake -> Celix Based Project
        #CELIX_LAUNCHER is set by FindCelix.cmake
    else()
        set(CELIX_LAUNCHER "$<TARGET_FILE:celix>")
    endif()

    #setup dependencies based on timestamp
    if (DOCKER_CREATE_FS)
        add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION>
            COMMAND cd $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION> && /bin/bash ${CELIX_CMAKE_DIRECTORY}/cmake_celix/create_target_filesystem.sh -e ${CELIX_LAUNCHER} -l $<TARGET_FILE:${DOCKER_DEPSLIB}>
            DEPENDS  "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_DEPS>" ${DOCKERFILE} ${DOCKER_DEPSLIB}
            WORKING_DIRECTORY "${DOCKER_LOCATION}"
            COMMENT "Creating docker dir for ${DOCKER_TARGET}" VERBATIM
        )
    else ()
        add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION>
            DEPENDS  "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_DEPS>" ${DOCKERFILE}
            WORKING_DIRECTORY "${DOCKER_LOCATION}"
            COMMENT "Creating docker dir for ${DOCKER_TARGET}" VERBATIM
        )
    endif ()

    ##### Deploy Target Properties for Dockerfile #####
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_LOCATION" "${DOCKER_LOC}")
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_FROM" "${DOCKER_FROM}") #name of docker base, default celix-base
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_IMAGE_NAME" "${DOCKER_IMAGE_NAME}") #name of docker images, default deploy target name
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_DIR" "${DOCKER_BUNDLES_DIR}") #bundles directory in docker image
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES" "") #bundles
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_WORKDIR" "${DOCKER_WORKDIR}") #workdir in docker image, should also contain the config.properties
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_ENTRYPOINT" "${DOCKER_ENTRYPOINT}")
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_CREATE_FS" "${DOCKER_CREATE_FS}") #wether to create a fs with the minimal needed libraries / etc files
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_INSTRUCTIONS" "") #list of additional instructions
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_PROPERTIES" "")
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_DEPS" "")

    set(DOCKERFILE "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION>/Dockerfile")

    file(GENERATE
            OUTPUT "${DOCKERFILE}"
            CONTENT "# Dockerfile for celix based image
FROM $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_FROM>
ENV IMAGE_NAME $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_IMAGE_NAME>
ADD . /
WORKDIR $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_WORKDIR>
$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_ENTRYPOINT>
$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_INSTRUCTIONS>,
>
")

    #generate config.properties
    set(DOCKER_PROPS "${DOCKER_LOC}/${DOCKER_WORKDIR}/config.properties")
    set(STAGE1_PROPERTIES "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-config-stage1.properties")
    file(GENERATE
            OUTPUT "${STAGE1_PROPERTIES}"
            CONTENT "cosgi.auto.start.1=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES>, >
$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_PROPERTIES>,
>
"
    )
    file(GENERATE
            OUTPUT "${DOCKER_PROPS}"
            INPUT "${STAGE1_PROPERTIES}"
    )


    if (DOCKER_BUNDLES)
        docker_bundles(${DOCKER_TARGET} ${DOCKER_BUNDLES})
    endif()
    if (DOCKER_PROPERTIES)
        docker_properties(${DOCKER_TARGET} ${DOCKER_PROPERTIES})
    endif ()
    if (DOCKER_INSTRUCTIONS)
        docker_instructions(${DOCKER_TARGET} ${DOCKER_INSTRUCTIONS})
    endif ()

    get_target_property(DEPS docker "DOCKER_DEPS")
    list(APPEND DEPS ${DOCKER_TARGET})
    set_target_properties(docker PROPERTIES "DOCKER_DEPS" "${DEPS}")

    #Note assuming sudo is needed for the docker command
    set(SUDO_CMD "")
    if (DOCKER_USE_SUDO)
        set(SUDO_CMD "sudo")
    endif ()
    add_custom_target(build-${DOCKER_TARGET}-docker-image
	    COMMAND cd $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOCATION> && ${SUDO_CMD} ${DOCKER_CMD} build -t "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_IMAGE_NAME>" .
        DEPENDS ${DOCKERFILE} ${DOCKER_TARGET}
        COMMENT "Creating docker image for target '${DOCKER_TARGET}'" VERBATIM
    )
    add_dependencies(build-docker-images build-${DOCKER_TARGET}-docker-image)

endfunction()

function(docker_bundles)
    if (NOT ENABLE_DOCKER)
        return()
    endif()

    #0 is docker TARGET
    #1..n is bundles
    list(GET ARGN 0 DOCKER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(BUNDLES ${DOCKER_TARGET} "DOCKER_BUNDLES")
    get_target_property(BUNDLES_DIR ${DOCKER_TARGET} "DOCKER_BUNDLES_DIR")
    get_target_property(LOC ${DOCKER_TARGET} "DOCKER_LOCATION")
    get_target_property(DEPS ${DOCKER_TARGET} "DOCKER_DEPS")

    foreach(BUNDLE IN ITEMS ${ARGN})
        if(IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
            list(APPEND BUNDLES "${BUNDLES_DIR}/${BUNDLE_FILENAME}")
            set(OUT "${LOC}/${BUNDLES_DIR}/${BUNDLE_FILENAME}")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE} ${OUT}
                COMMENT "Copying bundle '${BUNDLE}' to '${OUT}'"
            )
        else() #assuming target
            list(APPEND BUNDLES "${BUNDLES_DIR}/${BUNDLE}.zip")
            set(OUT ${LOC}/${BUNDLES_DIR}/${BUNDLE}.zip)
            add_custom_command(OUTPUT ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" "${OUT}"
                    COMMENT "Copying bundle '${BUNDLE}' to '${OUT}'"
            )
            add_dependencies(${DOCKER_TARGET} ${BUNDLE}_bundle) #ensure the the deploy depends on the _bundle target, custom_command depends on add_library
        endif()
        list(APPEND DEPS "${OUT}")
    endforeach()

    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES" "${BUNDLES}")
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_DEPS" "${DEPS}")
endfunction()

function(docker_properties)
    if (NOT ENABLE_DOCKER)
        return()
    endif()

    #0 is docker TARGET
    #1..n is properties
    list(GET ARGN 0 DOCKER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(PROPS ${DOCKER_TARGET} "DOCKER_PROPERTIES")

    foreach(PROP IN ITEMS ${ARGN})
        list(APPEND PROPS ${PROP})
    endforeach()

    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_PROPERTIES" "${PROPS}")
endfunction()

function(docker_instructions)
    if (NOT ENABLE_DOCKER)
        return()
    endif()

    #0 is docker TARGET
    #1..n is instructions
    list(GET ARGN 0 DOCKER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(INSTRUCTIONS ${DOCKER_TARGET} "DOCKER_INSTRUCTIONS")

    foreach(INSTR IN ITEMS ${ARGN})
        list(APPEND INSTRUCTIONS ${INSTR})
    endforeach()

    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_INSTRUCTIONS" "${INSTRUCTIONS}")
endfunction()
