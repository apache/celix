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

if (NOT TARGET celix-runtimes)
	add_custom_target(celix-runtimes ALL
	    DEPENDS "$<TARGET_PROPERTY:celix-runtimes,DEPS>"
	)
	set_target_properties(celix-runtimes PROPERTIES "DEPS" "")
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/runtimes")
endif ()

function(add_runtime)
    message(DEPRECATION "add_celix_runtime is deprecated, and will be removed in future Celix releases.")
    add_celix_runtime(${ARGN})
endfunction()
function(add_celix_runtime)
    message(DEPRECATION "add_celix_runtime is deprecated, and will be removed in future Celix releases.")
    list(GET ARGN 0 RUNTIME_TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS USE_TERM LOG_TO_FILES)
    set(ONE_VAL_ARGS WAIT_FOR NAME GROUP)
    set(MULTI_VAL_ARGS DEPLOYMENTS CONTAINERS COMMANDS ARGUMENTS RELEASE_FILES)
    cmake_parse_arguments(RUNTIME "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED RUNTIME_NAME)
        set(RUNTIME_NAME ${RUNTIME_TARGET_NAME})
    endif ()
    if (NOT DEFINED RUNTIME_GROUP)
        set(RUNTIME_LOC "${CMAKE_BINARY_DIR}/runtimes/${RUNTIME_NAME}")
    else ()
        set(RUNTIME_LOC "${CMAKE_BINARY_DIR}/runtimes/${RUNTIME_GROUP}/${RUNTIME_NAME}")
    endif ()
    set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RUNTIME_TARGET_NAME}-runtime-timestamp")

    set(START_SCRIPT "$<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>/start.sh")
    set(STOP_SCRIPT "$<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>/stop.sh")
    set(COMMON_SCRIPT "$<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>/common.sh")

    add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>
        COMMAND chmod +x $<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>/start.sh
        COMMAND chmod +x $<TARGET_PROPERTY:${RUNTIME_TARGET_NAME},RUNTIME_LOC>/stop.sh
        DEPENDS  ${START_SCRIPT} ${STOP_SCRIPT} ${SETUP_SCRIPT}
        WORKING_DIRECTORY "${RUNTIME_LOC}"
        COMMENT "Creating runtime ${RUNTIME_TARGET_NAME}" VERBATIM
    )
    add_custom_target(${RUNTIME_TARGET_NAME}
        DEPENDS "${TIMESTAMP_FILE}"
    )

    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_CONTAINERS" "") #containers that should be runned
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_COMMANDS" "") #command that should be executed
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_ARGUMENTS" "") #potential arguments to use for deployments
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RELEASE_FILES" "") #release files which needs to be released before starting the containers/commands
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_NAME" "${RUNTIME_NAME}") #The runtime workdir
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_GROUP" "${RUNTIME_GROUP}") #The runtime workdir
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_LOC" "${RUNTIME_LOC}") #The runtime workdir
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_USE_TERM" "${RUNTIME_USE_TERM}") #Wether or not the use terminal
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_LOG_TO_FILES" "${RUNTIME_LOG_TO_FILES}") #log to files or std out/err
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_NEXT_CONTAINER_ID" "0") #used for indexes int he bash scripts

    #wait for deployment, e.g. the one that control the lifecycle of the runtime.
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_WAIT_FOR_CONTAINER" "")

    #wait for command, e.g. the one that control the lifecycle of the runtime.
    set_target_properties(${RUNTIME_TARGET_NAME} PROPERTIES "RUNTIME_WAIT_FOR_COMMAND" "")


    #replaces @RUNTIME_TARGET_NAME@
    configure_file("${CELIX_CMAKE_DIRECTORY}/runtime_start.sh.in" "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/start.sh.in.1" @ONLY)
    configure_file("${CELIX_CMAKE_DIRECTORY}/runtime_stop.sh.in" "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/stop.sh.in.1" @ONLY)
    configure_file("${CELIX_CMAKE_DIRECTORY}/runtime_common.sh.in" "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/common.sh.in.1" @ONLY)


    file(GENERATE
            OUTPUT "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/common.sh.in.2"
            INPUT  "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/common.sh.in.1"
    )
    file(GENERATE
        OUTPUT "${START_SCRIPT}"
        INPUT "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/start.sh.in.1"
    )
    file(GENERATE
        OUTPUT "${STOP_SCRIPT}"
        INPUT "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/stop.sh.in.1"
    )


    file(GENERATE
        OUTPUT "${COMMON_SCRIPT}"
        INPUT "${CMAKE_BINARY_DIR}/celix/gen/runtimes/${RUNTIME_TARGET_NAME}/common.sh.in.2"
    )

    get_target_property(DEPS celix-runtimes "DEPS")
    list(APPEND DEPS "${RUNTIME_TARGET_NAME}")
    set_target_properties(celix-runtimes PROPERTIES "DEPS" "${DEPS}")

    celix_runtime_containers(${RUNTIME_TARGET_NAME} ${RUNTIME_DEPLOYMENTS})
    celix_runtime_containers(${RUNTIME_TARGET_NAME} ${RUNTIME_CONTAINERS})
    celix_runtime_commands(${RUNTIME_TARGET_NAME} ${RUNTIME_COMMANDS})
    celix_runtime_arguments(${RUNTIME_TARGET_NAME} ${RUNTIME_ARGUMENTS})
    celix_runtime_release_files(${RUNTIME_TARGET_NAME} ${RUNTIME_RELEASE_FILES})

    if (RUNTIME_WAIT_FOR)
        celix_runtime_container_wait_for(${RUNTIME_TARGET_NAME} ${RUNTIME_WAIT_FOR})
    endif ()

endfunction()

function(runtime_use_term)
    message(DEPRECATION "runtime_use_term is depecrated, use celix_runtime_use_term instead.")
    celix_runtime_use_term(${ARGN})
endfunction()
function(celix_runtime_use_term)
    #0 is runtime TARGET
    #1 is BOOL (use xterm)
    list(GET ARGN 0 RUNTIME_NAME)
    list(GET ARGN 1 USE_TERM)
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_USE_TERM" "${USE_TERM}")
endfunction()

function(runtime_log_to_files)
    message(DEPRECATION "runtime_log_to_files is depecrated, use celix_runtime_log_to_files instead.")
    celix_runtime_log_to_files(${ARGN})
endfunction()
function(celix_runtime_log_to_files)
    #0 is runtime TARGET
    #1 is BOOL (log to files)
    list(GET ARGN 0 RUNTIME_NAME)
    list(GET ARGN 1 LOG_TO_FILES)
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_LOG_TO_FILES" "${LOG_TO_FILES}")
endfunction()

function(runtime_deployments)
    message(DEPRECATION "runtime_deployments is depecrated, use celix_runtime_containers instead.")
    celix_runtime_containers(${ARGN})
endfunction()
function(celix_runtime_containers)
    #0 is runtime TARGET
    #1..n is deployments
    list(GET ARGN 0 RUNTIME_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(CONTAINERS ${RUNTIME_NAME} "RUNTIME_CONTAINERS")
    foreach(CONTAINER IN ITEMS ${ARGN})
        get_target_property(DEP_ID ${RUNTIME_NAME} "RUNTIME_NEXT_CONTAINER_ID")
        math(EXPR DEP_ID "${DEP_ID}+1")
        set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_DEPLOYMENT_${CONTAINER}_ID" "${DEP_ID}")
        list(APPEND CONTAINERS "CONTAINERS_NAMES[${DEP_ID}]=\"$<TARGET_PROPERTY:${CONTAINER},CONTAINER_NAME>\"")
        list(APPEND CONTAINERS "CONTAINERS_DIRS[${DEP_ID}]=\"$<TARGET_PROPERTY:${CONTAINER},CONTAINER_LOC>\"")
        list(APPEND CONTAINERS "CONTAINERS_DEBUG_OPTS[${DEP_ID}]=\"\${${CONTAINER}_DEBUG_OPTS:-}\"")
        set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_NEXT_CONTAINER_ID" "${DEP_ID}")
   endforeach()

   set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_CONTAINERS" "${CONTAINERS}")
endfunction()

function(runtime_deployment_wait_for)
    message(DEPRECATION "runtime_deployment_wait_for is depecrated, use celix_runtime_container_wait_for instead.")
    celix_runtime_container_wait_for(${ARGN})
endfunction()
function(celix_runtime_container_wait_for)
    #0 is runtime TARGET
    #1 is deployment TARGET
    list(GET ARGN 0 RUNTIME_NAME)
    list(GET ARGN 1 CONTAINER)

    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_WAIT_FOR_CONTAINER" "$<TARGET_PROPERTY:${CONTAINER},CONTAINER_LOC>")
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_WAIT_FOR_COMMAND" "")
endfunction()

function(runtime_commands)
    message(DEPRECATION "runtime_commands is depecrated, use celix_runtime_commands instead.")
    celix_runtime_commands(${ARGN})
endfunction()
function(celix_runtime_commands)
    #0 is runtime TARGET
    #1..n is commands
    list(GET ARGN 0 RUNTIME_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(COMMANDS ${RUNTIME_NAME} "RUNTIME_COMMANDS")
    foreach(CMD IN ITEMS ${ARGN})
        list(APPEND COMMANDS ${CMD})
    endforeach()
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_COMMANDS" "${COMMANDS}")
endfunction()

function(runtime_command_wait_for)
    message(DEPRECATION "runtime_command_wait_for is depecrated, use celix_runtime_command_wait_for instead.")
    celix_runtime_command_wait_for(${ARGN})
endfunction()
function(celix_runtime_command_wait_for)
    #0 is runtime TARGET
    #1 is COMMAND STR
    list(GET ARGN 0 RUNTIME_NAME)
    list(GET ARGN 1 COMMAND)

    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_WAIT_FOR_COMMAND" "${COMMAND}")
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_WAIT_FOR_CONTAINER" "")
endfunction()

function(runtime_arguments)
    message(DEPRECATION "runtime_arguments is depecrated, use celix_runtime_arguments instead.")
    celix_runtime_arguments(${ARGN})
endfunction()
function(celix_runtime_arguments)
    #0 is runtime TARGET
    #1..n is commands
    list(GET ARGN 0 RUNTIME_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(ARGUMENTS ${RUNTIME_NAME} "RUNTIME_ARGUMENTS")
    list(LENGTH ARGN ARG_LENGTH)
    if (${ARG_LENGTH} GREATER 1)
        foreach(I RANGE 1 ${ARG_LENGTH} 2)
            math(EXPR IMINUS "${I}-1")
            list(GET ARGN ${IMINUS} DEPLOY_NAME)
            list(GET ARGN ${I} DEPLOY_ARGS)
            list(APPEND ARGUMENTS "CONTAIMER_ARGUMENTS[$<TARGET_PROPERTY:${RUNTIME_NAME},RUNTIME_CONTAIMER_${DEPLOY_NAME}_ID>]=\"${DEPLOY_ARGS}\"")
        endforeach()
    endif ()
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RUNTIME_ARGUMENTS" "${ARGUMENTS}")
endfunction()

function(celix_runtime_release_files)
    #0 is runtime TARGET
    #1..n is release files
    list(GET ARGN 0 RUNTIME_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(RELEASE_FILES ${RUNTIME_NAME} "RELEASE_FILES")
    foreach(RELEASE_FILE IN ITEMS ${ARGN})
        list(APPEND RELEASE_FILES "source ${RELEASE_FILE}")
    endforeach()
    set_target_properties(${RUNTIME_NAME} PROPERTIES "RELEASE_FILES" "${RELEASE_FILES}")
endfunction()
