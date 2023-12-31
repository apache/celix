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


set(CELIX_DEFAULT_CONTAINER_CXX_OPTION ON CACHE BOOL "Whether the default container options is CXX. If OFF this will be C")

##### setup bundles/container target
if (NOT TARGET celix-containers)
	add_custom_target(celix-containers ALL
		DEPENDS $<TARGET_PROPERTY:celix-containers,CONTAINER_DEPLOYMENTS>
	)
	set_target_properties(celix-containers PROPERTIES "CONTAINER_DEPLOYMENTS" "") #initial empty deps list

	get_directory_property(CLEANFILES ADDITIONAL_CLEAN_FILES)
	list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/deploy")
	list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/celix")
	set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${CLEANFILES}")
endif ()
#####

function(add_deploy)
    message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
    add_celix_container(${ARGN})
endfunction()

#[[
Add a Celix container, consisting out of a selection of bundles and a Celix launcher.
Celix containers can be used to start a Celix framework together with a selection of bundles.

A Celix container will be build in `<cmake_build_dir>/deploy[/<group_name>]/<celix_container_name>`.
Use the `<celix_container_name>` executable to run the containers.

There are three variants of 'add_celix_container':
- If no launcher is specified a custom Celix launcher will be generated. This launcher also contains the configured properties.
- If a LAUNCHER_SRC is provided a Celix launcher will be build using the provided sources. Additional sources can be added with the
  CMake 'target_sources' command.
- If a LAUNCHER (absolute path to a executable of CMake `add_executable` target) is provided that will be used as Celix launcher.

Creating a Celix containers using 'add_celix_container' will lead to a CMake executable target (unless a LAUNCHER is used).
These targets can be used to run/debug Celix containers from a IDE (if the IDE supports CMake).

Optional Arguments:
- COPY: With this option the bundles used in the container will be copied in and configured for a bundles directory
  next to the container executable. Only one of the COPY or NO_COPY options can be provided.
  Default is COPY.
- NO_COPY: With this option the bundles used in the container will be configured using absolute paths to the bundles
  zip files. Only one of the COPY or NO_COPY options can be provided.
  Default is COPY.
- CXX: With this option the generated Celix launcher (if used) will be a C++ source.
  This ensures that the Celix launcher is linked against stdlibc++. Only one of the C or CXX options can be provided.
  Default is CXX
- C: With this option the generated Celix launcher (if used) will be a C source. Only one of the C or CXX options can
  be provided.
  Default is CXX
- USE_CONFIG: With this option the config properties are generated in a 'config.properties' instead of embedded in
  the Celix launcher.
- GROUP: If configured the build location will be prefixed the GROUP. Default is empty.
- NAME: The name of the executable. Default is <celix_container_name>. Only useful for generated/LAUNCHER_SRC
  Celix launchers.
- DIR: The base build directory of the Celix container. Default is `<cmake_build_dir>/deploy`.
- BUNDLES: A list of bundles for the Celix container to install and start.
  These bundle will be configured for run level 3. See 'celix_container_bundles' for more info.
- INSTALL_BUNDLES: A list of bundles for the Celix container to install (but not start).
- PROPERTIES: A list of configuration properties, these can be used to configure the Celix framework and/or bundles.
  Normally this will be EMBEDDED_PROPERTIES, but if the USE_CONFIG option is used this will be RUNTIME_PROPERTIES.
  See the framework library or bundles documentation about the available configuration options.
- EMBEDDED_PROPERTIES: A list of configuration properties which will be used in the generated Celix launcher.
- RUNTIME_PROPERTIES: A list of configuration properties which will be used in the generated config.properties file.

```CMake
add_celix_container(<celix_container_name>
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER launcher
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

```CMake
add_celix_container(<celix_container_name>
    LAUNCHER_SRC launcher_src
    [COPY]
    [NO_COPY]
    [CXX]
    [C]
    [USE_CONFIG]
    [GROUP group_name]
    [NAME celix_container_name]
    [DIR dir]
    [BUNDLES <bundle1> <bundle2> ...]
    [INSTALL_BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [EMBEDDED_PROPERTIES "prop1=val1" "prop2=val2" ...]
    [RUNTIME_PROPERTIES "prop1=val1" "prop2=val2" ...]
)
```

Examples:
```CMake
#Creates a Celix container in ${CMAKE_BINARY_DIR}/deploy/simple_container which starts 3 bundles located at
#${CMAKE_BINARY_DIR}/deploy/simple_container/bundles.
add_celix_container(simple_container
    BUNDLES
        Celix::shell
        Celix::shell_tui
        Celix::log_admin
    PROPERTIES
        CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=debug
)
```
]]
function(add_celix_container)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY NO_COPY C CXX FAT USE_CONFIG)
    set(ONE_VAL_ARGS GROUP NAME LAUNCHER LAUNCHER_SRC DIR)
    set(MULTI_VAL_ARGS BUNDLES INSTALL_BUNDLES PROPERTIES EMBEDDED_PROPERTIES RUNTIME_PROPERTIES)
    cmake_parse_arguments(CONTAINER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##### Check arguments #####
    if (NOT DEFINED CONTAINER_NAME)
        set(CONTAINER_NAME "${CONTAINER_TARGET}")
    endif ()
    if (NOT DEFINED CONTAINER_DIR)
        set(CONTAINER_DIR "${CMAKE_BINARY_DIR}/deploy")
    endif ()
    if (CONTAINER_COPY AND CONTAINER_NO_COPY)
        message(FATAL_ERROR "Only COPY of NO_COPY can be configured for a Celix container, not both")
    endif ()
    if (CONTAINER_C AND CONTAINER_CXX)
        message(FATAL_ERROR "Only CXX of C can be configured for a Celix container. Not both")
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
        set(LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/${SRC_FILENAME}")
        set(LAUNCHER_ORG "${CONTAINER_LAUNCHER_SRC}")
        file(GENERATE OUTPUT "${LAUNCHER_SRC}" INPUT "${LAUNCHER_ORG}")
    else () #generate custom launcher
        if (CONTAINER_C)
            set(LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.c")
        elseif (CONTAINER_CXX)
            set(LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.cc")
        else()
            if (CELIX_DEFAULT_CONTAINER_CXX_OPTION)
                set(LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.cc")
            else ()
                set(LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.c")
            endif ()
        endif()
        set(STAGE1_LAUNCHER_SRC "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/main.stage1.c")

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
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>>:CELIX_AUTO_START_6=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_LEVEL_6>, >\\n>\\
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_INSTALL>>:CELIX_AUTO_INSTALL=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_INSTALL>, >\\n>\\
$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_EMBEDDED_PROPERTIES>,\\n\\
>\";

    celix_properties_t *embeddedProps = celix_properties_loadFromString(config);
    return celixLauncher_launchAndWaitForShutdown(argc, argv, embeddedProps);
}
"
        )

        #Rerun generate to do a second parsing of generator expressions
        file(GENERATE OUTPUT "${LAUNCHER_SRC}" INPUT "${STAGE1_LAUNCHER_SRC}")
    endif ()

    if (LAUNCHER_SRC) #compilation needed
        add_executable(${CONTAINER_TARGET} ${LAUNCHER_SRC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CONTAINER_LOC})
        set_target_properties(${CONTAINER_TARGET} PROPERTIES OUTPUT_NAME ${CONTAINER_NAME})
        target_link_libraries(${CONTAINER_TARGET} PRIVATE Celix::framework)
        if(NOT APPLE)
            #Add --no-as-needed options, to ensure that libraries are always linked.
            #This is needed because most libraries are not used by the executable, but by the bundle libraries instead.
            set_target_properties(${CONTAINER_TARGET} PROPERTIES LINK_FLAGS -Wl,--no-as-needed)
        endif()
        set(LAUNCHER "$<TARGET_FILE:${CONTAINER_TARGET}>")
    else ()
        #LAUNCHER already set
        add_custom_target(${CONTAINER_TARGET}
		    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER} ${CONTAINER_LOC}/${CONTAINER_NAME}
        )
    endif ()

    #generate config.properties. C
    set(CONTAINER_PROPS "${CONTAINER_LOC}/config.properties")
    set(STAGE1_PROPERTIES "${CMAKE_BINARY_DIR}/celix/gen/containers/${CONTAINER_TARGET}/container-config-stage1.properties")
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
$<$<BOOL:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_INSTALL>>:CELIX_AUTO_INSTALL=$<JOIN:$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_BUNDLES_INSTALL>, >>
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
    #Setting CELIX_LIB_DIRS
    if (TARGET Celix::framework)
        #Celix Main Project
        set(CELIX_LIB_DIRS "$<TARGET_FILE_DIR:Celix::framework>:$<TARGET_FILE_DIR:Celix::utils>:$<TARGET_FILE_DIR:Celix::dfi>")
    else ()
        message(FATAL_ERROR "No Celix::framework target is defined. Did you use find_package(Celix REQUIRED)?")
    endif()

    #generate release.sh and optional run.sh
    if(APPLE)
        set(LIB_PATH_NAME "DYLD_LIBRARY_PATH")
    else()
        set(LIB_PATH_NAME "LD_LIBRARY_PATH")
    endif()

    if (CONTAINER_NO_COPY)
        set(CONTAINER_COPY FALSE)
    elseif (CONTAINER_COPY)
        set(CONTAINER_COPY TRUE)
    else ()
        set(CONTAINER_COPY TRUE)
    endif ()

    ##### Container Target Properties #####
    #internal use
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_0" "") #bundles to deploy for the container for startup level 0
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_1" "") #bundles to deploy for the container for startup level 1
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_2" "") #bundles to deploy for the container for startup level 2
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_3" "") #bundles to deploy for the container for startup level 3, the default used startup level
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_4" "") #bundles to deploy for the container for startup level 4
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_5" "") #bundles to deploy for the container for startup level 5
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_6" "") #bundles to deploy for the container for startup level 6
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_INSTALL" "") #bundles to install for the container
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
    celix_container_bundles(${CONTAINER_TARGET} INSTALL ${CONTAINER_INSTALL_BUNDLES})
    if (CONTAINER_USE_CONFIG)
        celix_container_runtime_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})
    else ()
        celix_container_embedded_properties(${CONTAINER_TARGET} ${CONTAINER_PROPERTIES})
    endif ()
    celix_container_runtime_properties(${CONTAINER_TARGET} ${CONTAINER_RUNTIME_PROPERTIES})
    celix_container_embedded_properties(${CONTAINER_TARGET} ${CONTAINER_EMBEDDED_PROPERTIES})

    if (LAUNCHER_DEP)
        add_dependencies(${CONTAINER_TARGET} ${LAUNCHER_DEP})
    endif ()

    #ensure the container dir will be deleted during clean
    get_directory_property(CLEANFILES ADDITIONAL_CLEAN_FILES)
    list(APPEND CLEANFILES "$<TARGET_PROPERTY:${CONTAINER_TARGET},CONTAINER_LOC>")
    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${CLEANFILES}")
endfunction()


#NOTE can be used for drivers/proxies/endpoints bundle dirs
function(deploy_bundles_dir)
    message(DEPRECATION "deploy_bundles_dir is depecrated, use celix_container_bundles_dir instead.")
    celix_container_bundles_dir(${ARGN})
endfunction()

#[[
Copy the specified bundles to a specified dir in the container build dir.

```CMake
celix_container_bundles_dir(<celix_container_target_name>
    DIR_NAME dir_name
    BUNDLES
        bundle1
        bundle2
        ...
)
```
]]
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

    set(DEST_DIR "${CONTAINER_LOC}/${BD_DIR_NAME}")
    get_target_property(CLEAN_FILES ${CONTAINER_TARGET} "ADDITIONAL_CLEAN_FILES")
    list(APPEND CLEAN_FILES ${DEST_DIR})
    list(REMOVE_DUPLICATES CLEAN_FILES)
    set_target_properties(${CONTAINER_TARGET} PROPERTIES "ADDITIONAL_CLEAN_FILES" "${CLEAN_FILES}")

    foreach(BUNDLE IN ITEMS ${BD_BUNDLES})
        if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
            string(MAKE_C_IDENTIFIER ${BUNDLE_FILENAME} BUNDLE_ID) #Create id with no special chars
            set(DEST "${DEST_DIR}/${BUNDLE_FILENAME}")
        elseif (TARGET ${BUNDLE})
            get_target_property(TARGET_TYPE ${BUNDLE} TYPE)
            if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
                #ignore
            else ()
                get_target_property(IMPORTED_TARGET ${BUNDLE} BUNDLE_IMPORTED)
                get_target_property(ALIASED_TARGET ${BUNDLE} ALIASED_TARGET)
                if (IMPORTED_TARGET) #An imported bundle target -> handle target without DEPENDS
                    _celix_extract_imported_bundle_info(${BUNDLE}) #extracts BUNDLE_FILE and BUNDLE_FILENAME
                    string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars
                    set(DEST "${DEST_DIR}/${BUNDLE_FILENAME}")
                elseif (ALIASED_TARGET) #An alias target use the aliased target
                    string(MAKE_C_IDENTIFIER ${ALIASED_TARGET} BUNDLE_ID) #Create id with no special chars
                    set(BUNDLE_FILE "$<TARGET_PROPERTY:${ALIASED_TARGET},BUNDLE_FILE>")
                    celix_get_bundle_filename(${ALIASED_TARGET} BUNDLE_FILENAME)
                    set(DEST "${DEST_DIR}/${BUNDLE_FILENAME}")
                    set(DEPS ${ALIASED_TARGET}_bundle) #Note white-box knowledge of celix_bundle custom target
                else() #Assuming a bundle target (library or add_custom_target)
                    string(MAKE_C_IDENTIFIER ${BUNDLE} BUNDLE_ID) #Create id with no special chars
                    set(BUNDLE_FILE "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>")
                    celix_get_bundle_filename(${BUNDLE} BUNDLE_FILENAME)
                    set(DEST "${DEST_DIR}/${BUNDLE_FILENAME}")
                    set(DEPS ${BUNDLE}_bundle) #Note white-box knowledge of celix_bundle custom target
                endif ()
            endif ()
        else ()
            message(FATAL_ERROR "Cannot add bundle in container ${CONTAINER_TARGET}. Provided bundle is not a abs path to an existing file nor a cmake target (${BUNDLE}).")
        endif ()

        if (BUNDLE_ID AND DEST AND BUNDLE_FILE AND NOT TARGET ${CONTAINER_TARGET}_copy_bundle_${BUNDLE_ID})
            add_custom_target(${CONTAINER_TARGET}_copy_bundle_${BUNDLE_ID}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_FILE} ${DEST}
                BYPRODUCTS ${DEST}
                DEPENDS ${DEPS}
            )
            add_dependencies(${CONTAINER_TARGET} ${CONTAINER_TARGET}_copy_bundle_${BUNDLE_ID})
            get_target_property(CLEAN_FILES ${CONTAINER_TARGET} "ADDITIONAL_CLEAN_FILES")
            list(APPEND CLEAN_FILES ${DEST})
            set_target_properties(${CONTAINER_TARGET} PROPERTIES "ADDITIONAL_CLEAN_FILES" "${CLEAN_FILES}")
        endif ()
    endforeach()
endfunction()

function(deploy_bundles)
    message(DEPRECATION "deploy_bundles is depecrated, use celix_container_bundles instead.")
    celix_container_bundles(${ARGN})
endfunction()

#[[
Add a selection of bundles to the Celix container.

```CMake
celix_container_bundles(<celix_container_target_name>
    [COPY]
    [NO_COPY]
    [LEVEL (0..6)]
    [INSTALL]
    bundle1
    bundle2
    ...
)
```

Example:
```CMake
celix_container_bundles(my_container Celix::shell Celix::shell_tui)
```

The selection of  bundles are (if configured) copied to the container build dir and
are added to the configuration properties so that they are installed and started when the Celix container is executed.

The Celix framework supports 7 (0 - 6) run levels. Run levels can be used to control the start and stop order of bundles.
Bundles in run level 0 are started first and bundles in run level 6 are started last.
When stopping bundles in run level 6 are stopped first and bundles in run level 0 are stopped last.
Within a run level the order of configured decides the start order; bundles added earlier are started first.

Optional Arguments:
- LEVEL: The run level for the added bundles. Default is 3.
- INSTALL: If this option is present, the bundles will only be installed instead of the default install and start.
           The bundles will be installed after all bundle in LEVEL 0..6 are installed and started.
- COPY: If this option is present, the bundles will be copied to the container build dir. This option overrides the
        NO_COPY option used in the add_celix_container call.
- NO_COPY: If this option is present, the install/start bundles will be configured using a absolute path to the
           bundle. This option overrides optional NO_COPY option used in the add_celix_container call.
]]
function(celix_container_bundles)
    #0 is container TARGET
    list(GET ARGN 0 CONTAINER_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS INSTALL COPY NO_COPY)
    set(ONE_VAL_ARGS LEVEL)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(BUNDLES "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    set(BUNDLES_LIST ${BUNDLES_UNPARSED_ARGUMENTS})

    #Check arguments
    if (BUNDLES_COPY AND BUNDLES_NO_COPY)
        message(FATAL_ERROR "Only COPY of NO_COPY can be configured for celix_container_bundles, not both.")
    endif ()

    if (NOT DEFINED BUNDLES_LEVEL)
        set(BUNDLES_LEVEL 3)
    endif ()

    if (BUNDLES_INSTALL)
        get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES_INSTALL")
    else () #bundle level 0,1,2,3,4,5 or 6
        get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES_LEVEL_${BUNDLES_LEVEL}")
    endif ()
    get_target_property(COPY ${CONTAINER_TARGET} "CONTAINER_COPY_BUNDLES")

    if (BUNDLES_COPY)
        set(COPY TRUE)
    elseif (BUNDLES_NO_COPY)
        set(COPY FALSE)
    endif ()

    foreach(BUNDLE IN ITEMS ${BUNDLES_LIST})
        if (TARGET ${BUNDLE})
           get_target_property(TARGET_TYPE ${BUNDLE} TYPE)
           if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
               #ignore. this can be used to keep targets name, but ignore it use in containers (e.g. Celix::dm_shell)
               message(DEBUG "Ignoring bundle target ${BUNDLE} add to container ${CONTAINER_TARGET}. Target type is not SHARED_LIBRARY")
           elseif (TARGET_TYPE STREQUAL "SHARED_LIBRARY" OR TARGET_TYPE STREQUAL "UTILITY")
               celix_get_bundle_file(${BUNDLE} ABS_LOC)
               celix_get_bundle_filename(${BUNDLE} COPY_LOC)
               add_celix_bundle_dependencies(${CONTAINER_TARGET} ${BUNDLE})
           else ()
               message(FATAL_ERROR "Cannot add bundle target ${BUNDLE} with target type ${TARGET_TYPE} to container ${CONTAINER_TARGET}. Only INTERFACE_LIBRARY, SHARED_LIBRARY or UTILITY is supported.")
           endif ()
        elseif (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
            get_filename_component(BUNDLE_FILENAME ${BUNDLE} NAME)
            set(COPY_LOC "${BUNDLE_FILENAME}")
            set(ABS_LOC "${BUNDLE}")
        else ()
            message(FATAL_ERROR "Cannot add bundle `${BUNDLE}` to container target ${CONTAINER_TARGET}. Argument is not a path or cmake target")
        endif ()

       if (COPY)
           set(BUNDLE_TO_ADD ${COPY_LOC})
       else ()
           set(BUNDLE_TO_ADD ${ABS_LOC})
       endif ()

       list(FIND BUNDLES ${BUNDLE_TO_ADD} INDEX)
       if (INDEX EQUAL -1) #Note this ignores the same bundle for the same level
           _celix_container_check_duplicate_bundles(${CONTAINER_TARGET} ${BUNDLE_TO_ADD} ${BUNDLES_LEVEL})
           list(APPEND BUNDLES ${BUNDLE_TO_ADD})
       endif ()
    endforeach()

    if (BUNDLES_INSTALL)
        set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_INSTALL" "${BUNDLES}")
    else () #bundle level 0,1,2,3,4,5 or 6
        set_target_properties(${CONTAINER_TARGET} PROPERTIES "CONTAINER_BUNDLES_LEVEL_${BUNDLES_LEVEL}" "${BUNDLES}")
    endif ()

   if (COPY)
       celix_container_bundles_dir(${CONTAINER_TARGET} DIR_NAME bundles BUNDLES ${BUNDLES_LIST})
   endif()
endfunction()

#[[
Private function to check if there are duplicate bundles in the container and print a warning if so.
Arg CONTAINER_TARGET ADDED_BUNDLES_LIST TARGET_LEVEL
]]
function(_celix_container_check_duplicate_bundles)
    list(GET ARGN 0 CONTAINER_TARGET)
    list(GET ARGN 1 TO_ADD_BUNDLE)
    list(GET ARGN 2 TARGET_LEVEL)

    if (NOT TARGET_LEVEL) #install
        return() #Bundles can be installed and added to a level
    endif()

    set(PARTIAL_MSG "Bundle `${TO_ADD_BUNDLE}` is added to the container multiple times. This can lead to errors \
        during bundle installation. Bundle `${TO_ADD_BUNDLE}` for level ${TARGET_LEVEL} is already added to the \
        container '${CONTAINER_TARGET}` at level ")

    foreach(BUNDLE_LEVEL RANGE 0 6)
        get_target_property(BUNDLES ${CONTAINER_TARGET} "CONTAINER_BUNDLES_LEVEL_${BUNDLE_LEVEL}")
        list(FIND BUNDLES ${TO_ADD_BUNDLE} INDEX)
        if (INDEX GREATER -1)
            message(WARNING "${PARTIAL_MSG} ${BUNDLE_LEVEL}.")
        endif()
    endforeach()
endfunction()

function(deploy_properties)
    celix_container_runtime_properties(${ARGN})
endfunction()

#[[
Add the provided properties to the target Celix container config properties.
These properties will be added to the config.properties in the container build dir.

```CMake
celix_container_runtime_properties(<celix_container_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```
]]
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

#[[
Add the provided properties to the target Celix container config properties.
These properties will be embedded into the generated Celix launcher.

```CMake
celix_container_embedded_properties(<celix_container_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```
]]
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

#[[
Add the provided properties to the target Celix container config properties.
If the USE_CONFIG option is used these configuration properties will be added to the 'config.properties' file else
these will be embedded into the generated Celix launcher.

```CMake
celix_container_properties(<celix_container_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```
]]
function(celix_container_properties)
    get_target_property(USE_CNF ${CONTAINER_TARGET} "CONTAINER_USE_CONFIG")
    if (USE_CNF)
        celix_container_runtime_properties(${ARGN})
    else ()
        celix_container_embedded_properties(${ARGN})
    endif ()
endfunction()

