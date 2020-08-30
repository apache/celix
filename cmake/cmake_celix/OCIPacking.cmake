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

##### setup bundles/container target
if (NOT TARGET celix-oci-bundles)
    add_custom_target(celix-oci-application-bundles ALL
            DEPENDS $<TARGET_PROPERTY:celix-oci-bundles,OCI_APPLICATION_BUNDLES>
    )
    set_target_properties(celix-oci-application-bundles PROPERTIES "OCI_APPLICATION_BUNDLES" "") #initial empty deps list
if (NOT TARGET celix-containers)
	add_custom_target(celix-containers ALL
		DEPENDS $<TARGET_PROPERTY:celix-containers,CONTAINER_DEPLOYMENTS>
	)
	set_target_properties(celix-containers PROPERTIES "CONTAINER_DEPLOYMENTS" "") #initial empty deps list
endif ()


    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "") #target deps for the container.
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_0" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_1" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_2" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_3" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_4" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_5" "") #bundles to deploy for the container for run level 0
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_COPY_BUNDLES" ${CONTAINER_COPY}) #copy bundles in bundle dir or link using abs paths. NOTE this cannot be changed after a add_deploy command

    #deploy specific
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_NAME" "${CONTAINER_NAME}")
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_GROUP" "${CONTAINER_GROUP}")
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_LOC" "${CONTAINER_LOC}")
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_PROPERTIES" "")
    #set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_EMBEDDED_PROPERTIES" "")
    #####


#Creates a OCI runtime bundle from a celix containers.
#TODO align with add_celix_docker, make it possible to "inherit" from add_celix_container
function (add_celix_oci_bundle_dir)
        set(OPTIONS )
        set(ONE_VAL_ARGS CELIX_CONTAINER OCI_NAME EXPORT)
        set(MULTI_VAL_ARGS )
        cmake_parse_arguments(OCI "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

        if (NOT TARGET ${OCI_CELIX_CONTAINER})
            message(FATAL_ERROR "Cannot create a OCI container from '${OCI_CELIX_CONTAINER}'; Specify a valid Celix container target with the CELIX_CONTAINER arguments")
        endif ()

        set(TARGET ${OCI_CELIX_CONTAINER}-oci-container)

        add_custom_target(${TARGET} ALL)
        get_target_property(DEPS celix-oci-application-bundles PROPERTY "OCI_APPLICATION_BUNDLES" )
        list(APPEND DEPS ${TARGET})
        set_target_properties(celix-oci-application-bundles PROPERTIES "OCI_APPLICATION_BUNDLES" "${DEPS}")


        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_0> LEVEL 0)
        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_1> LEVEL 1)
        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_2> LEVEL 2)
        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_3> LEVEL 3)
        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_4> LEVEL 4)
        celix_oci_container_add_bundles(${OCI_CELIX_CONTAINER} $<TARGET_PROPERTY:${OCI_CELIX_CONTAINER},CONTAINER_BUNDLES_LEVEL_5> LEVEL 5)
endfunction ()

#TODO combine with docker
function (celix_oci_container_bundles)
    #0 is docker TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)
    set(TARGET ${CONTAINER_TARGET}-oci-container)

    set(OPTIONS )
    set(ONE_VAL_ARGS LEVEL)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(BUNDLES "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    set(BUNDLES_LIST ${BUNDLES_UNPARSED_ARGUMENTS})

    if (NOT DEFINED BUNDLES_LEVEL)
        set(BUNDLES_LEVEL 1)
    endif ()

    get_target_property(BUNDLES ${TARGET} "BUNDLES_LEVEL_${BUNDLES_LEVEL}")
    get_target_property(BUNDLES_DIR ${TARGET} "BUNDLES_DIR")
    get_target_property(LOC ${TARGET} "LOC")
    get_target_property(DEPS ${TARGET} "DEPS")

    foreach(BUNDLE IN ITEMS ${BUNDLES_LIST})
        set(HANDLED FALSE)
        if(IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
            set(OUT "${LOC}/${BUNDLES_DIR}/${BUNDLE_FILENAME}")
            add_custom_command(OUTPUT ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE} ${OUT}
                    COMMENT "Copying (file) bundle '${BUNDLE}' to '${LOC}/${BUNDLES_DIR}'"
                    DEPENDS ${BUNDLE}
                    )
            list(APPEND BUNDLES "${BUNDLES_DIR}/${BUNDLE_FILENAME}")
            set(HANDLED TRUE)
        elseif (TARGET ${BUNDLE})
            get_target_property(TARGET_TYPE ${BUNDLE} TYPE)
            if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
                #ignore
                set(HANDLED TRUE)
            else ()
                get_target_property(IMP ${BUNDLE} BUNDLE_IMPORTED)
                if (IMP) #An imported bundle target -> handle target without DEPENDS
                    string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
                    set(OUT "${CMAKE_BINARY_DIR}/celix/gen/docker/${DOCKER_TARGET}/copy-bundle-for-target-${BUNDLE_ID}.timestamp")
                    set(DEST "${LOC}/${BUNDLES_DIR}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
                    add_custom_command(OUTPUT ${OUT}
                            COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                            COMMAND ${CMAKE_COMMAND} -E make_directory ${LOC}/${BUNDLES_DIR}
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${DEST}
                            COMMENT "Copying (imported) bundle '${BUNDLE}' to '${LOC}/${BUNDLES_DIR}'"
                            )
                    list(APPEND BUNDLES "${BUNDLES_DIR}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
                    set(HANDLED TRUE)
                endif ()
            endif ()
        endif ()

        if(NOT HANDLED) #assuming (future) bundle target
            string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
            set(OUT "${CMAKE_BINARY_DIR}/celix/gen/docker/${DOCKER_TARGET}/copy-bundle-for-target-${BUNDLE_ID}.timestamp")
            set(DEST "${LOC}/${BUNDLES_DIR}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
            add_custom_command(OUTPUT ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${LOC}/${BUNDLES_DIR}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${DEST}
                    COMMENT "Copying (target) bundle '${BUNDLE}' to '${LOC}/${BUNDLES_DIR}'"
                    DEPENDS ${BUNDLE} $<TARGET_PROPERTY:${BUNDLE},BUNDLE_CREATE_BUNDLE_TARGET>
                    )
            list(APPEND BUNDLES "${BUNDLES_DIR}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
        endif()
        list(APPEND DEPS "${OUT}")
    endforeach()

    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_${BUNDLES_LEVEL}" "${BUNDLES}")
    set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_DEPS" "${DEPS}")
endfunction ()

    #TODO install_celix_oci_containers_targets
