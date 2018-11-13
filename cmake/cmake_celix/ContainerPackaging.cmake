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
if (NOT TARGET celix-containers)
	add_custom_target(celix-containers ALL
		DEPENDS $<TARGET_PROPERTY:celix-containers,CONTAINER_DEPLOYMENTS>
	)
	set_target_properties(celix-containers PROPERTIES "CONTAINER_DEPLOYMENTS" "") #initial empty deps list

	get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
	list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/deploy")
	list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/celix")
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
endif ()
#####

function(add_deploy)
    message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
    add_celix_container(${ARGN})
endfunction()
function(add_celix_container)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY CXX USE_CONFIG)
    set(ONE_VAL_ARGS GROUP NAME LAUNCHER LAUNCHER_SRC DIR)
    set(MULTI_VAL_ARGS BUNDLES PROPERTIES EMBEDDED_PROPERTIES RUNTIME_PROPERTIES)
    cmake_parse_arguments(CONTAINER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##### Check arguments #####
    if (NOT DEFINED CONTAINER_NAME)
        set(CONTAINER_NAME "${CONTAINER_TARGET}")
    endif ()
    if (NOT DEFINED CONTAINER_DIR)
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
        set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/${SRC_FILENAME}")
        set(LAUNCHER_ORG "${CONTAINER_LAUNCHER_SRC}")
        add_custom_command(OUTPUT ${LAUNCHER_SRC}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER_ORG} ${LAUNCHER_SRC}
        )
    else () #generate custom launcher
        if (CONTAINER_CXX)
            set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.cc")
        else()
            set(LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.c")
        endif()
        set(STAGE1_LAUNCHER_SRC "${PROJECT_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.stage1.c")

        file(GENERATE
                OUTPUT "${STAGE1_LAUNCHER_SRC}"
                CONTENT "#include <celix_launcher.h>
int main(int argc, char *argv[]) {
    const char * config = \"\\
CELIX_CONTAINER_NAME=$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_NAME>\\n\\
CELIX_BUNDLES_PATH=bundles\\n\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_0>>:CELIX_AUTO_START_0=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_0>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_1>>:CELIX_AUTO_START_1=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_1>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_2>>:CELIX_AUTO_START_2=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_2>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_3>>:CELIX_AUTO_START_3=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_3>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_4>>:CELIX_AUTO_START_4=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_4>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_5>>:CELIX_AUTO_START_5=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_5>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>>:CELIX_AUTO_START_5=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>, >\\n>\\
$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_EMBEDDED_PROPERTIES>,\\n\\
>\";

    celix_properties_t *embeddedProps = celix_properties_loadFromString(config);
    return celixLauncher_launchAndWaitForShutdown(argc, argv, embeddedProps);
}
"
        )

        file(GENERATE OUTPUT "${LAUNCHER_SRC}" INPUT "${STAGE1_LAUNCHER_SRC}")
    endif ()

    if (LAUNCHER_SRC) #compilation needed
        add_executable(${CONTAINER_TARGET} ${LAUNCHER_SRC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CONTAINER_LOC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES OUTPUT_NAME ${CONTAINER_NAME})
        target_link_libraries(${CONTAINER_TARGET} PRIVATE Celix::framework)
        set(LAUNCHER "$<TARGET_FILE:${CONTAINER_TARGET}>")
    else ()
        #LAUNCHER already set
        add_custom_target(${CONTAINER_TARGET}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER} ${CONTAINER_LOC}/${CONTAINER_NAME}
        )
    endif ()

    #generate config.properties. C
    set(CONTAINER_PROPS "${CONTAINER_LOC}/config.properties")
    set(STAGE1_PROPERTIES "${PROJECT_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/container-config-stage1.properties")
    if (CONTAINER_USE_CONFIG)
        file(GENERATE
                OUTPUT "${STAGE1_PROPERTIES}"
                CONTENT "CELIX_CONTAINER_NAME=$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_NAME>
CELIX_BUNDLES_PATH=bundles
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_0>>:CELIX_AUTO_START_0=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_0>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_1>>:CELIX_AUTO_START_1=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_1>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_2>>:CELIX_AUTO_START_2=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_2>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_3>>:CELIX_AUTO_START_3=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_3>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_4>>:CELIX_AUTO_START_4=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_4>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_5>>:CELIX_AUTO_START_5=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_5>, >>
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>>:CELIX_AUTO_START_6=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>, >>
$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_RUNTIME_PROPERTIES>,
>"
        )
        file(GENERATE
                OUTPUT "${CONTAINER_PROPS}"
                INPUT "${STAGE1_PROPERTIES}"
                )
    else ()
        file(GENERATE
                OUTPUT "${STAGE1_PROPERTIES}"
                CONTENT "$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_RUNTIME_PROPERTIES>,
>
"
                )
        #Condition is there so that config.properties file will only be generated if there are runtime properties
        file(GENERATE
                OUTPUT "${CONTAINER_PROPS}"
                INPUT "${STAGE1_PROPERTIES}"
                CONDITION $<NOT:$<STREQUAL:,$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_RUNTIME_PROPERTIES>>>
                )
    endif ()

    #needed in the release.sh & run.sh files
    #Setting CELIX_LIB_DIRS, CELIX_BIN_DIR and CELIX_LAUNCHER
    if (TARGET Celix::framework)
        #Celix Main Project
        set(CELIX_LIB_DIRS "$<TARGET_FILE_DIR:Celix::framework>:$<TARGET_FILE_DIR:Celix::utils>:$<TARGET_FILE_DIR:Celix::dfi>")
        set(CELIX_BIN_DIR  "$<TARGET_FILE_DIR:Celix::launcher>")
    else ()
	message(FATAL_ERROR "No Celix::framework target is defined. Did you use find_package(Celix REQUIRED)?")
    endif()

    #generate release.sh and optional run.sh
    if(APPLE)
        set(LIB_PATH_NAME "DYLD_LIBRARY_PATH")
    else()
        set(LIB_PATH_NAME "LD_LIBRARY_PATH")
    endif()


    #add a custom target which can depend on generation expressions
    add_custom_target(${CONTAINER_TARGET}-deps
        DEPENDS
            $<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_TARGET_DEPS>
    )
    add_dependencies(${CONTAINER_TARGET} ${CONTAINER_TARGET}-deps)

    ##### Container Target Properties #####
    #internal use
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "") #target deps for the container.
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_0" "") #bundles to deploy for the container for startup level 0
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_1" "") #bundles to deploy for the container for startup level 1
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_2" "") #bundles to deploy for the container for startup level 2
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_3" "") #bundles to deploy for the container for startup level 3, the default used startup level
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_4" "") #bundles to deploy for the container for startup level 4
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_5" "") #bundles to deploy for the container for startup level 5
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_6" "") #bundles to deploy for the container for startup level 6
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_COPY_BUNDLES" ${CONTAINER_COPY}) #copy bundles in bundle dir or link using abs paths. NOTE this cannot be changed after a add_deploy command

    #deploy specific
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_NAME" "${CONTAINER_NAME}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_GROUP" "${CONTAINER_GROUP}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_LOC" "${CONTAINER_LOC}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_USE_CONFIG" "${CONTAINER_USE_CONFIG}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_RUNTIME_PROPERTIES" "")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_EMBEDDED_PROPERTIES" "")
    #####

    celix_container_bundles(${CONTAINER_TARGET} LEVEL 3 ${CONTAINER_BUNDLES})
    if (CONTAINER_USE_CONFIG)
        celix_container_runtime_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})
    else ()
        celix_container_embedded_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})
    endif ()
    celix_container_runtime_properties(${CONTAINER_TARGET} ${CONTAINER_RUNTIME_PROPERTIES})
    celix_container_embedded_properties(${CONTAINER_TARGET} ${CONTAINER_EMBEDDED_PROPERTIES})


    #ensure the container dir will be deleted during clean
    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_LOC>")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
endfunction()


#NOTE can be used for drivers/proxies/endpoints bundle dirs
function(deploy_bundles_dir)
    message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
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
        set(HANDLED FALSE)
        if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME) 
            set(OUT "${CONTAINER_LOC}/${BD_DIR_NAME}/${BUNDLE_FILENAME}")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE} ${OUT}
		        COMMENT "Copying (file) bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
            )
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
                    set(OUT "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/copy-bundle-for-target-${BUNDLE_ID}.timestamp")
                    set(DEST "${CONTAINER_LOC}/${BD_DIR_NAME}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
                    add_custom_command(OUTPUT ${OUT}
                        COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${CONTAINER_LOC}/${BD_DIR_NAME}
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${DEST}
                        COMMENT "Copying (imported) bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
                    )
                    set(HANDLED TRUE)
	         else() #Assuming a bundle target (library or add_custom_target)
			string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
			set(OUT "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/copy-bundle-for-target-${BUNDLE_ID}.timestamp")
			set(DEST "${CONTAINER_LOC}/${BD_DIR_NAME}/$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
			add_custom_command(OUTPUT ${OUT}
				COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${CONTAINER_LOC}/${BD_DIR_NAME}
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" ${DEST}
				COMMENT "Copying (target) bundle '${BUNDLE}' to '${CONTAINER_LOC}/${BD_DIR_NAME}'"
				DEPENDS ${BUNDLE} $<TARGET_PROPERTY:${BUNDLE},BUNDLE_CREATE_BUNDLE_TARGET>
				)
                    set(HANDLED TRUE)
                endif ()
            endif ()
        endif ()

        if (NOT HANDLED) #not a imported bundle target so (assuming) a future bundle target
		message(FATAL_ERROR "Cannot add bundle in container ${CONTAINER_TARGET}. Provided bundle is not a abs path to an existing file nor a cmake target (${BUNDLE}).")
        endif()
        list(APPEND DEPS "${OUT}")
    endforeach()

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "${DEPS}")
endfunction()

function(deploy_bundles)
    message(DEPRECATION "deploy_bundles is depecrated, use celix_container_bundles instead.")
    celix_container_bundles(${ARGN})
endfunction()
function(celix_container_bundles)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS LEVEL)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(BUNDLES "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    set(BUNDLES_LIST ${BUNDLES_UNPARSED_ARGUMENTS})

    if (NOT DEFINED BUNDLES_LEVEL)
        set(BUNDLES_LEVEL 1)
    endif ()

    get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES_LEVEL_${BUNDLES_LEVEL}")
    get_target_property(COPY ${CONTAINER_TARGET} "CONTAINER_COPY_BUNDLES")
    get_target_property(DEPS ${CONTAINER_TARGET} "CONTAINER_TARGET_DEPS")

    foreach(BUNDLE IN ITEMS ${BUNDLES_LIST})
        set(HANDLED FALSE)
        if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
               get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
               set(COPY_LOC "${BUNDLE_FILENAME}")
               set(ABS_LOC "${BUNDLE}")
               set(HANDLED TRUE)
           elseif (TARGET ${BUNDLE})
               get_target_property(TARGET_TYPE ${BUNDLE} TYPE)
               if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
                   #ignore. this can be used to keep targets name, but ignore it use in containers (e.g. Celix::dm_shell)
                   set(HANDLED TRUE)
               else()
                   get_target_property(IMP ${BUNDLE} BUNDLE_IMPORTED)
                   if (IMP) #An imported bundle target -> handle target without DEPENDS
                       set(COPY_LOC "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
                       set(ABS_LOC "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>")
                       set(HANDLED TRUE)
                   endif ()
               endif ()
           endif ()

           if (NOT HANDLED) #not a imported bundle target, so assuming a (future) bundle target
               set(COPY_LOC "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILENAME>")
               set(ABS_LOC "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>")

               if (NOT COPY) #in case of COPY dep will be added in celix_container_bundles_dir
                   string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars (e.g. for target like Celix::shell)
                   set(OUT "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/check-bundle-for-target-${BUNDLE_ID}.timestamp")
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

    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_${BUNDLES_LEVEL}" "${BUNDLES}")
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_TARGET_DEPS" "${DEPS}")

   if(COPY) 
       celix_container_bundles_dir(${CONTAINER_TARGET} DIR_NAME bundles BUNDLES ${BUNDLES_LIST})
   endif()
endfunction()

function(deploy_properties)
    celix_container_runtime_properties(${ARGN})
endfunction()
function(celix_container_runtime_properties)
    #0 is container TARGET
    #1..n is bundles
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    get_target_property(PROPS ${CONTAINER_TARGET} "CONTAINER_RUNTIME_PROPERTIES")

    foreach(PROP IN ITEMS ${ARGN})
        list(APPEND PROPS ${PROP})
    endforeach()

   set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_RUNTIME_PROPERTIES" "${PROPS}")
endfunction()

function(deploy_embedded_properties)
    message(DEPRECATION "deploy_embedded_properties is depecrated, use celix_container_embedded_properties instead.")
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

function(celix_container_properties)
    get_target_property(USE_CNF ${CONTAINER_TARGET} "CONTAINER_USE_CONFIG")
    if (USE_CNF)
        celix_container_runtime_properties(${ARGN})
    else ()
        celix_container_embedded_properties(${ARGN})
    endif ()
endfunction()
