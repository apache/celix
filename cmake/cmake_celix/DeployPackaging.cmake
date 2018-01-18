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
add_custom_target(celix-containers ALL
        DEPENDS $<TARGET_PROPERTY:celix-containers,CONTAINER_DEPLOYMENTS>
)
set_target_properties(celix-containers PROPERTIES "CONTAINER_DEPLOYMENTS" "") #initial empty deps list

get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/deploy")
list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/celix")
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
#####

function(add_deploy)
    #message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
    add_celix_container(${ARGN})
endfunction()
function(add_celix_container)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY CXX)
    set(ONE_VAL_ARGS GROUP NAME LAUNCHER LAUNCHER_SRC DIR)
    set(MULTI_VAL_ARGS BUNDLES PROPERTIES EMBEDDED_PROPERTIES)
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

    #add this target as depependency to the celix-containers target
    get_target_property(CONTAINERDEPS celix-containers "CONTAINER_DEPLOYMENTS")
    list(APPEND CONTAINERDEPS ${CONTAINER_TARGET})
    set_target_properties(celix-containers PROPERTIES "CONTAINER_DEPLOYMENTS" "${CONTAINERDEPS}")

    #FILE TARGETS FOR CONTAINER
    set(CONTAINER_PROPS "${CONTAINER_LOC}/config.properties")
    set(CONTAINER_ECLIPSE_LAUNCHER "${CONTAINER_LOC}/${CONTAINER_NAME}.launch")

    set(LAUNCHER_DEP )
    if (CONTAINER_LAUNCHER)
        if (IS_ABSOLUTE "${CONTAINER_LAUNCHER}")
            set(LAUNCHER "${CONTAINER_LAUNCHER}")
        else()
            #assuming target
            set(LAUNCHER "$<TARGET_FILE:${CONTAINER_LAUNCHER}>")
            set(LAUNCHER_DEP ${CONTAINER_LAUNCHER})
        endif()
    elseif (CONTAINER_LAUNCHER_SRC)
        get_filename_component(SRC_FILENAME ${CONTAINER_LAUNCHER_SRC} NAME)
        set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-${SRC_FILENAME}")
        set(LAUNCHER_ORG "${CONTAINER_LAUNCHER_SRC}")
    else () #generate custom launcher
        if (CONTAINER_CXX)
            set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-main.cc")
        else()
            set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-main.c")
        endif()

        file(GENERATE
                OUTPUT "${LAUNCHER_SRC}"
                CONTENT "#include <celix_launcher.h>
int main(int argc, char *argv[]) {
    const char * config = \"\\
$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_EMBEDDED_PROPERTIES>,\\n\\
>\";

    properties_pt packedConfig = properties_loadFromString(config);
    return celixLauncher_launchWithArgsAndProps(argc, argv, packedConfig);
}
"
        )
    endif ()

    if (LAUNCHER_SRC) #compilation needed
        add_executable(${CONTAINER_TARGET} ${LAUNCHER_SRC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CONTAINER_LOC})
        target_link_libraries(${CONTAINER_TARGET} PRIVATE Celix::framework)
        set(LAUNCHER "$<TARGET_FILE:${CONTAINER_TARGET}>")
    else ()
        #LAUNCHER already set
        add_custom_target(${CONTAINER_TARGET}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER} ${CONTAINER_LOC}/${CONTAINER_TARGET}
        )
    endif ()

    #generate config.properties
    set(STAGE1_PROPERTIES "${PROJECT_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-container-config-stage1.properties")
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
    if (TARGET framework)
        #Celix Main Project
        set(CELIX_LIB_DIRS "$<TARGET_FILE_DIR:Celix::framework>:$<TARGET_FILE_DIR:Celix::utils>:$<TARGET_FILE_DIR:Celix::dfi>")
        set(CELIX_BIN_DIR  "$<TARGET_FILE_DIR:Celix::launcher>")
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
    set(RELEASE_CONTENT "#!/bin/sh\nexport ${LIB_PATH_NAME}=${CELIX_LIB_DIRS}:\${${LIB_PATH_NAME}}\nexport PATH=${CELIX_BIN_DIR}:\${PATH}")
    file(GENERATE
        OUTPUT ${RELEASE_SH}
        CONTENT ${RELEASE_CONTENT}
    )

    set(RUN_SH ${CONTAINER_LOC}/run.sh)
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

    #add a custom target which can depend on generation expressions
    add_custom_target(${CONTAINER_TARGET}-deps
        DEPENDS
            ${RUN_SH}
            ${CONTAINER_ECLIPSE_LAUNCHER}
            ${RELEASE_SH}
            ${CONTAINER_PROPS}
            $<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_TARGET_DEPS>
    )
    add_dependencies(${CONTAINER_TARGET} ${CONTAINER_TARGET}-deps)


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
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_EMBEDDED_PROPERTIES" "")
    #####

    celix_container_bundles(${CONTAINER_TARGET} ${CONTAINER_BUNDLES})
    celix_container_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})
    celix_container_embedded_properties(${CONTAINER_TARGET} ${CONTAINER_EMBEDDED_PROPERTIES})


    #ensure the container dir will be deleted during clean
    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_LOC>")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
endfunction()


#NOTE can be used for drivers/proxies/endpoints bundle dirs
function(deploy_bundles_dir)
    #message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
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
            string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
            set(OUT "${CMAKE_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-copy-bundle-for-target-${BUNDLE_ID}.timestamp")
            set(DEST "${CONTAINER_LOC}/${BD_DIR_NAME}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CONTAINER_LOC}/${BD_DIR_NAME}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${DEST}
                COMMENT "Copying bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
                DEPENDS ${BUNDLE} $<TARGET_PROPERTY:${BUNDLE},BUNDLE_CREATE_BUNDLE_TARGET>
            )
        endif()
        list(APPEND DEPS "${OUT}")
    endforeach()

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "${DEPS}")
endfunction()

function(deploy_bundles)
    #message(DEPRECATION "deploy_bundles is depecrated, use celix_container_bundles instead.")
    celix_container_bundles(${ARGN})
endfunction()
function(celix_container_bundles)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES")
    get_target_property(COPY ${CONTAINER_TARGET} "CONTAINER_COPY_BUNDLES")
    get_target_property(DEPS ${CONTAINER_TARGET} "CONTAINER_TARGET_DEPS")

    foreach(BUNDLE IN ITEMS ${ARGN})
           if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
               get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
               set(COPY_LOC "bundles/${BUNDLE_FILENAME}")
               set(ABS_LOC "${BUNDLE}")
           else () #assume target (could be a future target -> if (TARGET ...) not possible
               set(COPY_LOC "bundles/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
               set(ABS_LOC "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>")

               if (NOT COPY) #in case of COPY dep will be added in celix_container_bundles_dir
                   string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
                   set(OUT "${CMAKE_BINARY_DIR}/celix/gen/${CONTAINER_TARGET}-check-bundle-for-target-${BUNDLE_ID}.timestamp")
                   add_custom_command(OUTPUT ${OUT}
                       COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                       DEPENDS ${BUNDLE} $<TARGET_PROPERTY:${BUNDLE},BUNDLE_CREATE_BUNDLE_TARGET>
                   )
                   list(APPEND DEPS ${OUT})
               endif ()
           endif ()
           if(COPY)
                list(APPEND BUNDLES ${COPY_LOC})
           else()
                list(APPEND BUNDLES ${ABS_LOC})
           endif()
   endforeach()

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES" "${BUNDLES}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "${DEPS}")

   if(COPY) 
       celix_container_bundles_dir(${CONTAINER_TARGET} DIR_NAME bundles BUNDLES ${ARGN})
   endif()
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

function(deploy_embedded_properties)
    #message(DEPRECATION "deploy_embedded_properties is depecrated, use celix_container_embedded_properties instead.")
    celix_container_embedded_properties(${ARGN})
endfunction()
function(celix_container_embedded_properties)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(PROPS ${CONTAINER_TARGET} "CONTAINER_EMBEDDED_PROPERTIES")

    foreach(PROP IN ITEMS ${ARGN})
        list(APPEND PROPS ${PROP})
    endforeach()

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_EMBEDDED_PROPERTIES" "${PROPS}")
endfunction()
