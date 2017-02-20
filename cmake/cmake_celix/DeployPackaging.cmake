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

##### setup bundles/deploy target
add_custom_target(deploy ALL
        DEPENDS $<TARGET_PROPERTY:deploy,DEPLOY_DEPLOYMENTS>
)
set_target_properties(deploy PROPERTIES "DEPLOY_DEPLOYMENTS" "") #initial empty deps list
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/deploy")
#####

function(add_deploy)
    list(GET ARGN 0 DEPLOY_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY)
    set(ONE_VAL_ARGS GROUP NAME LAUNCHER)
    set(MULTI_VAL_ARGS BUNDLES PROPERTIES)
    cmake_parse_arguments(DEPLOY "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##### Check arguments #####
    if(NOT DEPLOY_NAME) 
        set(DEPLOY_NAME "${DEPLOY_TARGET}")
    endif()
    ######

    ##### Setting defaults #####
    if(DEPLOY_GROUP) 
        set(DEPLOY_LOCATION "${CMAKE_BINARY_DIR}/deploy/${DEPLOY_GROUP}/${DEPLOY_NAME}")
        set(DEPLOY_PRINT_NAME "${DEPLOY_GROUP}/${DEPLOY_NAME}")
    else()
        set(DEPLOY_LOCATION "${CMAKE_BINARY_DIR}/deploy/${DEPLOY_NAME}")
        set(DEPLOY_PRINT_NAME "${DEPLOY_NAME}")
    endif()
    ######


    ###### Setup deploy custom target and config.properties file
    set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEPLOY_TARGET}-deploy-timestamp")

    add_custom_target(${DEPLOY_TARGET}
        DEPENDS ${TIMESTAMP_FILE}
    )
    get_target_property(DEPLOYDEPS deploy "DEPLOY_DEPLOYMENTS")
    list(APPEND DEPLOYDEPS ${DEPLOY_TARGET})
    set_target_properties(deploy PROPERTIES "DEPLOY_DEPLOYMENTS" "${DEPLOYDEPS}")

    #FILE TARGETS FOR DEPLOY
    set(DEPLOY_EXE "${DEPLOY_LOCATION}/${DEPLOY_NAME}")
    set(DEPLOY_RUN_SH "${DEPLOY_LOCATION}/run.sh")
    set(DEPLOY_PROPS "${DEPLOY_LOCATION}/config.properties")
    set(DEPLOY_ECLIPSE_LAUNCHER "${DEPLOY_LOCATION}/${DEPLOY_NAME}.launch")
    set(DEPLOY_RELEASE_SH "${DEPLOY_LOCATION}/release.sh")

    find_program(LINK_CMD ln)
    if (LINK_CMD) 
        #if ln is available use a softlink to celix exe instead of a run.sh
        list(APPEND DEPLOY_FILE_TARGETS ${DEPLOY_PROPS} ${DEPLOY_ECLIPSE_LAUNCHER} ${DEPLOY_RELEASE_SH} ${DEPLOY_RUN_SH} ${DEPLOY_EXE})
    else()
        list(APPEND DEPLOY_FILE_TARGETS ${DEPLOY_PROPS} ${DEPLOY_ECLIPSE_LAUNCHER} ${DEPLOY_RELEASE_SH} ${DEPLOY_RUN_SH})
    endif()

    #setup dependencies based on timestamp
    add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
        DEPENDS  "$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_TARGET_DEPS>" ${DEPLOY_FILE_TARGETS} 
        WORKING_DIRECTORY "${DEPLOY_LOCATION}"
        COMMENT "Deploying ${DEPLOY_PRINT_NAME}" VERBATIM
    )

    #Setting CELIX_LIB_DIRS, CELIX_BIN_DIR and CELIX_LAUNCHER 
    if (EXISTS ${CELIX_FRAMEWORK_LIBRARY}) 
        #CELIX_FRAMEWORK_LIBRARY set by FindCelix.cmake -> Celix Based Project
        get_filename_component(CELIX_LIB_DIR ${CELIX_FRAMEWORK_LIBRARY} DIRECTORY) #Note assuming all celix libs are in the same dir
        set(CELIX_LIB_DIRS "${CELIX_LIB_DIR}")
        #CELIX_LAUNCHER is set by FindCelix.cmake
        get_filename_component(CELIX_BIN_DIR ${CELIX_LAUNCHER} DIRECTORY)
    else()
        #Celix Main Project
        set(CELIX_LIB_DIRS "$<TARGET_FILE_DIR:celix_framework>:$<TARGET_FILE_DIR:celix_utils>:$<TARGET_FILE_DIR:celix_dfi>")
        set(CELIX_LAUNCHER "$<TARGET_FILE:celix>")
        set(CELIX_BIN_DIR  "$<TARGET_FILE_DIR:celix>")
    endif()

    #generate config.properties
    set(STAGE1_PROPERTIES "${CMAKE_CURRENT_BINARY_DIR}/${DEPLOY_TARGET}-deploy-config-stage1.properties")
    file(GENERATE 
        OUTPUT "${STAGE1_PROPERTIES}"
        CONTENT "cosgi.auto.start.1=$<JOIN:$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_BUNDLES>, >
$<JOIN:$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_PROPERTIES>,
>
"
    )
    file(GENERATE
        OUTPUT "${DEPLOY_PROPS}"
        INPUT "${STAGE1_PROPERTIES}"
    )


    #Setup launcher using celix target, celix binary or custom launcher
    if (DEPLOY_LAUNCHER)
        if (IS_ABSOLUTE "${DEPLOY_LAUNCHER}")
            #assuming target
            set(LAUNCHER "${DEPLOY_LAUNCHER}")
        else()
            set(LAUNCHER "$<TARGET_FILE:${DEPLOY_LAUNCHER}>")
        endif()
    else()
        #Use CELIX_LAUNCHER
        set(LAUNCHER "${CELIX_LAUNCHER}")
    endif()

    #softlink celix exe file
    add_custom_command(OUTPUT "${DEPLOY_EXE}"
        COMMAND ${LINK_CMD} -s "${LAUNCHER}" "${DEPLOY_EXE}"
        WORKING_DIRECTORY ${DEPLOY_LOCATION}
        DEPENDS "${LAUNCHER}" 
        COMMENT "Symbolic link launcher to ${DEPLOY_EXE}" VERBATIM
    ) 


    #generate release.sh and optional run.sh
    if(APPLE)
        set(LIB_PATH_NAME "DYLD_LIBRARY_PATH")
    else()
        set(LIB_PATH_NAME "LD_LIBRARY_PATH")
    endif()
    set(RELEASE_CONTENT "#!/bin/sh\nexport ${LIB_PATH_NAME}=${CELIX_LIB_DIRS}:\${${LIB_PATH_NAME}}\nexport PATH=${CELIX_BIN_DIR}:\${PATH}")
    file(GENERATE
        OUTPUT ${DEPLOY_RELEASE_SH}
        CONTENT ${RELEASE_CONTENT}
    )
    set(RUN_CONTENT "${RELEASE_CONTENT}\n${LAUNCHER} \$@\n")
    file(GENERATE
        OUTPUT ${DEPLOY_RUN_SH}
        CONTENT ${RUN_CONTENT}
    )

    #generate eclipse project launch file
    set(PROGRAM_NAME "${LAUNCHER}")
    set(CONTAINER_NAME ${DEPLOY_NAME})
    set(PROJECT_ATTR "${CMAKE_PROJECT_NAME}-build")
    set(WORKING_DIRECTORY ${DEPLOY_LOCATION})
    include("${CELIX_CMAKE_DIRECTORY}/cmake_celix/RunConfig.in.cmake") #set VAR RUN_CONFIG_IN
    file(GENERATE
        OUTPUT "${DEPLOY_ECLIPSE_LAUNCHER}"
        CONTENT "${RUN_CONFIG_IN}"    
    )

    ##### Deploy Target Properties #####
    #internal use
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_TARGET_DEPS" "") #bundles to deploy.
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_BUNDLES" "") #bundles to deploy.
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_COPY_BUNDLES" ${DEPLOY_COPY}) #copy bundles in bundle dir or link using abs paths. NOTE this cannot be changed after a add_deploy command

    #deploy specific
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_LOCATION" ${DEPLOY_LOCATION})
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_PROPERTIES" "")
    #####

    deploy_bundles(${DEPLOY_TARGET} ${DEPLOY_BUNDLES})
    deploy_properties(${DEPLOY_TARGET} ${DEPLOY_PROPERTIES})
endfunction()


#NOTE can be used for drivers/proxies/endpoints bundle dirs
function(deploy_bundles_dir)
    list(GET ARGN 0 DEPLOY_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS)
    set(ONE_VAL_ARGS DIR_NAME)
    set(MULTI_VAL_ARGS BUNDLES)
    cmake_parse_arguments(BD "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    if(NOT BD_DIR_NAME)
        message(FATAL_ERROR "Missing mandatory DIR_NAME argument")
    endif()

    get_target_property(DEPLOY_LOC ${DEPLOY_NAME} "DEPLOY_LOCATION")
    get_target_property(DEPS ${DEPLOY_NAME} "DEPLOY_TARGET_DEPS")

    foreach(BUNDLE IN ITEMS ${BD_BUNDLES})
        if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME) 
            set(OUT "${DEPLOY_LOC}/${BD_DIR_NAME}/${BUNDLE_FILENAME}")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE} ${OUT}
                COMMENT "Copying bundle '${BUNDLE}' to '${DEPLOY_LOC}/${BD_DIR_NAME}'"
                DEPENDS ${BUNDLE}
            )
        else()
            set(OUT "${DEPLOY_LOC}/${BD_DIR_NAME}/${BUNDLE}.zip")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${OUT}
                COMMENT "Copying bundle '${BUNDLE}' to '${DEPLOY_LOC}/${BD_DIR_NAME}'"
                DEPENDS ${BUNDLE} #Note cannot directly depends on ${BUNDLE}_bundle, depending in ${BUNDLE} triggering build instead. 
            )
            add_dependencies(${DEPLOY_NAME} ${BUNDLE}_bundle) #ensure the the deploy depends on the _bundle target, custom_command depends on add_library
        endif()
        list(APPEND DEPS "${OUT}")

    endforeach()

    set_target_properties(${DEPLOY_NAME} PROPERTIES "DEPLOY_TARGET_DEPS" "${DEPS}")
endfunction()

function(deploy_bundles)
    #0 is deploy TARGET
    #1..n is bundles
    list(GET ARGN 0 DEPLOY_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(BUNDLES ${DEPLOY_NAME} "DEPLOY_BUNDLES")
    get_target_property(COPY ${DEPLOY_NAME} "DEPLOY_COPY_BUNDLES")

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
       deploy_bundles_dir(${DEPLOY_NAME} DIR_NAME bundles BUNDLES ${ARGN})
   endif()

   set_target_properties(${DEPLOY_NAME} PROPERTIES "DEPLOY_BUNDLES" "${BUNDLES}")
endfunction()

function(deploy_properties)
    #0 is deploy TARGET
    #1..n is bundles
    list(GET ARGN 0 DEPLOY_NAME)
    list(REMOVE_AT ARGN 0)

    get_target_property(PROPS ${DEPLOY_NAME} "DEPLOY_PROPERTIES")

    foreach(PROP IN ITEMS ${ARGN})
        list(APPEND PROPS ${PROP})
    endforeach()

   set_target_properties(${DEPLOY_NAME} PROPERTIES "DEPLOY_PROPERTIES" "${PROPS}")
endfunction()
