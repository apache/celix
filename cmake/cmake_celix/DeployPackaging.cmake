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
add_custom_target(containers ALL
        DEPENDS $<TARGET_PROPERTY:containers,CONTAINER_DEPLOYMENTS>
)
set_target_properties(containers PROPERTIES "CONTAINER_DEPLOYMENTS" "") #initial empty deps list

get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/deploy")
list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/celix")
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
#####

function(add_deploy)
    add_celix_container(${ARGN})
endfunction()

function(add_celix_container)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY CXX)
    set(ONE_VAL_ARGS GROUP NAME LAUNCHER LAUNCHER_SRC DIR)
    set(MULTI_VAL_ARGS BUNDLES PROPERTIES)
    cmake_parse_arguments(CONTAINER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##### Check arguments #####
    if (NOT CONTAINER_NAME)
        set(CONTAINER_NAME "${CONTAINER_TARGET}")
    endif ()
    if (NOT CONTAINER_DIR)
        set(CONTAINER_DIR "${CMAKE_BINARY_DIR}/deploy")
    endif ()
    ######

    ##### Setting defaults #####
    if (CONTAINER_GROUP)
        set(CONTAINER_LOC "${CONTAINER_DIR}/${CONTAINER_GROUP}/${CONTAINER_NAME}")
        set(CONTAINER_PRINT_NAME "${CONTAINER_GROUP}/${CONTAINER_NAME}")
    else ()
        set(CONTAINER_LOC "${CONTAINER_DIR}/${CONTAINER_NAME}")
        set(CONTAINER_PRINT_NAME "${CONTAINER_NAME}")
    endif ()
    ######

    get_target_property(CONTAINERDEPS containers "CONTAINER_DEPLOYMENTS")
    list(APPEND CONTAINERDEPS ${CONTAINER_TARGET})
    set_target_properties(containers PROPERTIES "CONTAINER_DEPLOYMENTS" "${CONTAINERDEPS}")

    #FILE TARGETS FOR CONTAINER
    set(CONTAINER_PROPS "${CONTAINER_LOC}/config.properties")
    set(CONTAINER_ECLIPSE_LAUNCHER "${CONTAINER_LOC}/${CONTAINER_NAME}.launch")

    if (CONTAINER_LAUNCHER_SRC)
        get_filename_component(SRC_FILENAME ${LAUNCHER_SRC} NAME)
        set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-${SRC_FILENAME}")
        set(LAUNCHER_ORG "${CONTAINER_LAUNCHER_SRC}")
    elseif (CONTAINER_CXX)
	    set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-main.cc")
        set(LAUNCHER_ORG "${CELIX_CMAKE_DIRECTORY}/cmake_celix/main.c.in")
    else()
	    set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-main.c")
        set(LAUNCHER_ORG "${CELIX_CMAKE_DIRECTORY}/cmake_celix/main.c.in")
    endif()

    if (CONTAINER_LAUNCHER)
        if (IS_ABSOLUTE "${CONTAINER_LAUNCHER}")
            set(LAUNCHER "${CONTAINER_LAUNCHER}")
        else()
            #assuming target
            set(LAUNCHER "$<TARGET_FILE:${CONTAINER_LAUNCHER}>")
        endif()
        add_custom_target(${CONTAINER_TARGET}
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${LAUNCHER} ${CONTAINER_LOC}/${CONTAINER_TARGET}
                DEPENDS  "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_TARGET_DEPS>"
                COMMENT "Deploying ${CONTAINER_PRINT_NAME} Celix container" VERBATIM
        )
    else ()
        add_custom_command(OUTPUT ${LAUNCHER_SRC}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/celix/gen
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER_ORG} ${LAUNCHER_SRC}
                DEPENDS  "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_TARGET_DEPS>"
                COMMENT "Deploying ${CONTAINER_PRINT_NAME} Celix container" VERBATIM
        )

        include_directories(${CELIX_INCLUDE_DIRS})
        add_executable(${CONTAINER_TARGET} ${LAUNCHER_SRC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CONTAINER_LOC})
        target_link_libraries(${CONTAINER_TARGET} PRIVATE ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY})
        set(LAUNCHER "$<TARGET_FILE:${CONTAINER_TARGET}>")
    endif ()

    #generate config.properties
    set(STAGE1_PROPERTIES "${CMAKE_CURRENT_BINARY_DIR}/${CONTAINER_TARGET}-container-config-stage1.properties")
    file(GENERATE 
        OUTPUT "${STAGE1_PROPERTIES}"
        CONTENT "cosgi.auto.start.1=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES>, >
$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_PROPERTIES>,
>
"
    )
    file(GENERATE
        OUTPUT "${CONTAINER_PROPS}"
        INPUT "${STAGE1_PROPERTIES}"
    )

    #needed in the release.sh & run.sh files
    #Setting CELIX_LIB_DIRS, CELIX_BIN_DIR and CELIX_LAUNCHER
    if (TARGET celix_framework)
        #Celix Main Project
        set(CELIX_LIB_DIRS "$<TARGET_FILE_DIR:celix_framework>:$<TARGET_FILE_DIR:celix_utils>:$<TARGET_FILE_DIR:celix_dfi>")
        set(CELIX_BIN_DIR  "$<TARGET_FILE_DIR:celix>")
    else ()
        #CELIX_FRAMEWORK_LIBRARY and CELIX_LAUNCHER set by FindCelix.cmake -> Celix Based Project
        get_filename_component(CELIX_LIB_DIR ${CELIX_FRAMEWORK_LIBRARY} DIRECTORY) #Note assuming all celix libs are in the same dir
        set(CELIX_LIB_DIRS "${CELIX_LIB_DIR}")
        get_filename_component(CELIX_BIN_DIR ${CELIX_LAUNCHER} DIRECTORY)
    endif()


    #generate release.sh and optional run.sh
    if(APPLE)
        set(LIB_PATH_NAME "DYLD_LIBRARY_PATH")
    else()
        set(LIB_PATH_NAME "LD_LIBRARY_PATH")
    endif()
    set(RELEASE_SH ${CONTAINER_LOC}/release.sh)
    set(RUN_SH ${CONTAINER_LOC}/run.sh)
    set(RELEASE_CONTENT "#!/bin/sh\nexport ${LIB_PATH_NAME}=${CELIX_LIB_DIRS}:\${${LIB_PATH_NAME}}\nexport PATH=${CELIX_BIN_DIR}:\${PATH}")
    file(GENERATE
        OUTPUT ${RELEASE_SH}
        CONTENT ${RELEASE_CONTENT}
    )
    set(RUN_CONTENT "${RELEASE_CONTENT}\n${LAUNCHER} \$@\n")
    file(GENERATE
        OUTPUT ${RUN_SH}
        CONTENT ${RUN_CONTENT}
    )

    #generate eclipse project launch file
    set(PROGRAM_NAME "${LAUNCHER}")
    set(PROJECT_ATTR "${CMAKE_PROJECT_NAME}-build")
    set(WORKING_DIRECTORY ${CONTAINER_LOC})
    include("${CELIX_CMAKE_DIRECTORY}/cmake_celix/RunConfig.in.cmake") #set VAR RUN_CONFIG_IN
    file(GENERATE
        OUTPUT "${CONTAINER_ECLIPSE_LAUNCHER}"
        CONTENT "${RUN_CONFIG_IN}"
    )

    ##### Container Target Properties #####
    #internal use
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "") #target deps for the container.
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES" "") #bundles to deploy fro the container.
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_COPY_BUNDLES" ${CONTAINER_COPY}) #copy bundles in bundle dir or link using abs paths. NOTE this cannot be changed after a add_deploy command

    #deploy specific
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_NAME" "${CONTAINER_NAME}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_GROUP" "${CONTAINER_GROUP}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_LOC" "${CONTAINER_LOC}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_PROPERTIES" "")
    #####

    celix_container_bundles(${CONTAINER_TARGET} ${CONTAINER_BUNDLES})
    celix_container_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})


    #ensure the container dir will be deleted during clean
    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_LOC>")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
endfunction()


#NOTE can be used for drivers/proxies/endpoints bundle dirs

function(deploy_bundles_dir)
    celix_container_bundles_dir(${ARGN})
endfunction()
function(celix_container_bundles_dir)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS)
    set(ONE_VAL_ARGS DIR_NAME)
    set(MULTI_VAL_ARGS BUNDLES)
    cmake_parse_arguments(BD "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    if(NOT BD_DIR_NAME)
        message(FATAL_ERROR "Missing mandatory DIR_NAME argument")
    endif()

    get_target_property(CONTAINER_LOC ${CONTAINER_TARGET} "CONTAINER_LOC")
    get_target_property(DEPS ${CONTAINER_TARGET} "CONTAINER_TARGET_DEPS")

    foreach(BUNDLE IN ITEMS ${BD_BUNDLES})
        if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME) 
            set(OUT "${CONTAINER_LOC}/${BD_DIR_NAME}/${BUNDLE_FILENAME}")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE} ${OUT}
		COMMENT "Copying bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
                DEPENDS ${BUNDLE}
            )
        else()
            set(OUT "${CONTAINER_LOC}/${BD_DIR_NAME}/${BUNDLE}.zip")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${OUT}
                COMMENT "Copying bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
                DEPENDS ${BUNDLE}
            )
            add_dependencies(${CONTAINER_TARGET} ${BUNDLE}_bundle) #ensure the the deploy depends on the _bundle target, custom_command depends on add_library

        endif()
        list(APPEND DEPS "${OUT}")

    endforeach()

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "${DEPS}")
endfunction()

function(deploy_bundles)
    celix_container_bundles(${ARGN})
endfunction()
function(celix_container_bundles)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES")
    get_target_property(COPY ${CONTAINER_TARGET} "CONTAINER_COPY_BUNDLES")

    foreach(BUNDLE IN ITEMS ${ARGN})
           if(COPY)
                if(IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
                    get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME) 
                    list(APPEND BUNDLES "bundles/${BUNDLE_FILENAME}")
                else() #assuming target
                    list(APPEND BUNDLES "bundles/${BUNDLE}.zip")
                endif()
           else()
                if(IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
                    list(APPEND BUNDLES ${BUNDLE})
                else() #assuming target
                    list(APPEND BUNDLES "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>")
                endif()
           endif()
   endforeach()

   if(COPY) 
       celix_container_bundles_dir(${CONTAINER_TARGET} DIR_NAME bundles BUNDLES ${ARGN})
   endif()

   set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES" "${BUNDLES}")
endfunction()

function(deploy_properties)
    celix_container_properties(${ARGN})
endfunction()
function(celix_container_properties)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(PROPS ${CONTAINER_TARGET} "CONTAINER_PROPERTIES")

    foreach(PROP IN ITEMS ${ARGN})
        list(APPEND PROPS ${PROP})
    endforeach()

   set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_PROPERTIES" "${PROPS}")
endfunction()
