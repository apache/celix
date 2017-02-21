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

add_custom_target(dockerfiles)
add_custom_target(dockerimages)


function(add_docker_for_all_deployments)
    #NOTE that this function must be called after all CMake add_deploy commands are executed.
    #NOTE that this is only done for deployments with the COPY instruction

    get_target_property(DEPLOYMENTS deploy "DEPLOY_DEPLOYMENTS")
    foreach(DEPLOY_TARGET ${DEPLOYMENTS})
        get_target_property(COPY ${DEPLOY_TARGET} DEPLOY_COPY_BUNDLES)
        if (COPY)
            deploy_docker(${DEPLOY_TARGET})
        endif()
    endforeach()
endfunction()

function(deploy_docker)
    list(GET ARGN 0 DEPLOY_TARGET)
    list(REMOVE_AT ARGN 0)

    deploy_docker_check(${DEPLOY_TARGET})

    set(OPTIONS )
    set(ONE_VAL_ARGS FROM NAME BUNDLES_DIR WORK_DIR)
    set(MULTI_VAL_ARGS INSTRUCTIONS)
    cmake_parse_arguments(DOCKER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    #set defaults
    if (NOT DOCKER_FROM)
        set(DOCKER_FROM "celix-base")
    endif()
    if (NOT DOCKER_NAME)
        set(DOCKER_NAME "${DEPLOY_TARGET}")
    endif()
    if (NOT DOCKER_BUNDLES_DIR)
        set(DOCKER_BUNDLES_DIR "/var/celix/workdir/bundles/")
    endif()
    if (NOT DOCKER_WORK_DIR)
        set(DOCKER_WORK_DIR "/var/celix/workdir/")
    endif()

    ##### Deploy Target Properties for Dockerfile #####
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_DOCKERFILE_FROM" "${DOCKER_FROM}") #name of docker base, default celix-base
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_DOCKERFILE_IMAGE_NAME" "${DOCKER_NAME}") #name of docker images, default deploy target name
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_DOCKERFILE_BUNDLES_DIR" "${DOCKER_BUNDLES_DIR}") #bundles directory in docker image
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_DOCKERFILE_WORK_DIR" "${DOCKER_WORK_DIR}") #workdir in docker image, should also contain the config.properties
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_DOCKERFILE_INSTRUCTIONS" "") #list of additional instructions

    set(DOCKERFILE "$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_LOCATION>/Dockerfile")

    file(GENERATE
            OUTPUT "${DOCKERFILE}"
            CONTENT "# Dockerfile for inaetics/celix-node-agent-service
FROM $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_FROM>

#set standalone env
ENV IMAGE_NAME $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_IMAGE_NAME>

#add bundles, properties and set workdir
COPY bundles/*.zip $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_BUNDLES_DIR>
COPY config.properties $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_WORK_DIR>
WORKDIR $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_WORK_DIR>
$<JOIN:$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_INSTRUCTIONS>,
>
")

    get_target_property(DEPS dockerfiles "DOCKER_FILES_DEPS")
    list(APPEND DEPS "${DOCKERFILE}")
    set_target_properties(dockerfiles PROPERTIES "DOCKER_FILES_DEPS" "${DEPS}")

    add_custom_target(${DEPLOY_TARGET}-dockerimage
        COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_LOCATION> docker build -t "$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_DOCKERFILE_IMAGE_NAME>" .
        DEPENDS ${DOCKERFILE} ${DEPLOY_TARGET}
        COMMENT "Creating docker image for deployment '${DEPLOY_TARGET}'" VERBATIM
    )
    add_dependencies(dockerimages ${DEPLOY_TARGET}-dockerimage)

    add_custom_target(${DEPLOY_TARGET}-dockerfile
            DEPENDS ${DOCKERFILE} ${DEPLOY_TARGET}
    )
    add_dependencies(dockerfiles ${DEPLOY_TARGET}-dockerfile)
endfunction()

function(deploy_docker_check)
    get_target_property(COPY ${DEPLOY_TARGET} DEPLOY_COPY_BUNDLES)
    if (NOT COPY)
        message(WARNING "
            Cannot create a valid DockerFile for deploy target '${DEPLOY_TARGET}', because the bundles are not copied in the deploy dir.
            This is needed because the source docker copy commands must be inside the docker build directory.
            Add the COPY to the add_deploy command for '${DEPLOY_TARGET}'
        ")
    endif()
endfunction()
