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

set(CELIX_DEFAULT_CONTAINER_CXX_OPTION ON CACHE BOOL "Whether the default docker options is CXX. If OFF this will be C")


##### setup docker target
option(CELIX_BUILD_DOCKER_USE_DOCKER "Use docker to build docker images" ON)
option(CELIX_BUILD_DOCKER_USE_DOCKER_DIR_TAR "Use docker directory tar packages packing" OFF)
if (NOT TARGET celix-build-docker-dirs)
  if (APPLE) #create filesystem script is not working on mac os, exclude target from ALL
    add_custom_target(celix-build-docker-dirs
      DEPENDS $<TARGET_PROPERTY:celix-build-docker-dirs,DOCKER_DEPS>
      )
  else ()
    add_custom_target(celix-build-docker-dirs ALL
      DEPENDS $<TARGET_PROPERTY:celix-build-docker-dirs,DOCKER_DEPS>
      )
  endif ()
  set_target_properties(celix-build-docker-dirs PROPERTIES "DOCKER_DEPS" "") #initial empty deps list
endif ()

if (CELIX_BUILD_DOCKER_USE_DOCKER)
  if (NOT TARGET celix-build-docker-images)
    add_custom_target(celix-build-docker-images)
    set(DOCKER_USE_SUDO ON CACHE BOOL "Wether the use of sudo is needed to run docker")
    set(DOCKER_CMD "docker" CACHE STRING "Docker command to use.")

    get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
    list(APPEND CLEANFILES "${CMAKE_BINARY_DIR}/docker")
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")
  endif ()
endif()

#####

#[[
Adds a docker target dir, containing a all the required executables,
libraries, filesystem files and selected bundles needed to run a Apache Celix framework in a docker container.

The 'add_celix_docker' target is a executable target and can be used to link libraries which are needed in the docker image.

The docker dir can be found in `<cmake_build_dir>/docker[/<group_name>]/<docker_name>`.

The docker directories are build with the target `celix-build-docker-dirs`, this does not create the
docker images and can also be executed on systems without docker. The `celix-build-docker-dirs` is trigger
with `make all`.

The docker images are build with the target `celix-build-docker-images`. For this to work docker needs te installed
and the user executing the `celix-build-docker-images` should have right to create docker images.
The `celix-build-docker-images` is not triggered with `make all`

There are three variants of 'add_celix_docker':
- If no launcher is specified a custom Celix launcher will be generated. This launcher also contains the configured properties.
- If a LAUNCHER_SRC is provided a Celix launcher will be build using the provided sources. Additional sources can be added with the
  CMake 'target_sources' command.
- If a LAUNCHER (absolute path to a executable of CMake `add_executable` target) is provided that will be used as Celix launcher.

Optional arguments:
- CXX: With this option the generated Celix launcher (if used) will be a C++ source.
  This ensures that the Celix launcher is linked against stdlibc++.
- C: With this option the generated Celix launcher (if used) will be a C source.
- GROUP: If configured the build location will be prefixed the GROUP. Default is empty.
- NAME: The name of the executable. Default is <docker_target_name>. Only useful for generated/LAUNCHER_SRC Celix launchers.
- FROM: Configured the docker image base. Default is scratch.
  If configured a minimal filesystem will not be created!
- BUNDLES_DIR: Configures the directory where are all the bundles are copied. Default is /bundles.
- WORKDIR: Configures the workdir of the docker image. Default is /root.
- IMAGE_NAME: Configure the image name. Default is NAME.
- BUNDLES: Configures the used bundles. These bundles are configured for run level 3. see 'celix_docker_bundles' for more info.
- PROPERTIES: Configure configuration properties.
- INSTRUCTIONS: Additional dockker instruction to add the the generated Dockerfile.

```CMake
add_celix_docker(<docker_target_name>
    [CXX]
    [C]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```

```CMake
add_celix_docker(<docker_target_name>
    LAUNCHER_SRC launcher_src
    [CXX]
    [C]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```

```CMake
add_celix_docker(<docker_target_name>
    LAUNCHER launcher
    [CXX]
    [C]
    [GROUP group_name]
    [NAME deploy_name]
    [FROM docker_from_image]
    [BUNDLES_DIR bundle_dir_in_docker_image]
    [WORKDIR workdir_in_docker_image]
    [IMAGE_NAME docker_image_name]
    [BUNDLES <bundle1> <bundle2> ...]
    [PROPERTIES "prop1=val1" "prop2=val2" ...]
    [INSTRUCTIONS "instr1" "instr2" ...]
)
```
]]
function(add_celix_docker)
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  set(OPTIONS C CXX)
  set(ONE_VAL_ARGS GROUP NAME FROM BUNDLES_DIR FILES_DIR WORKDIR IMAGE_NAME LAUNCHER LAUNCHER_SRC TAR_LOCATION)
  set(MULTI_VAL_ARGS BUNDLES PROPERTIES INSTRUCTIONS FILES)
  cmake_parse_arguments(DOCKER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

  #set defaults
  if (NOT DEFINED DOCKER_FROM)
    set(DOCKER_FROM "scratch")
    set(DOCKER_CREATE_FS true)
  else ()
    set(DOCKER_CREATE_FS false)
  endif ()
  if (NOT DEFINED DOCKER_NAME)
    set(DOCKER_NAME "${DOCKER_TARGET}")
  endif ()
  if (NOT DEFINED DOCKER_IMAGE_NAME)
    set(DOCKER_IMAGE_NAME "${DOCKER_NAME}")
  endif ()
  if (NOT DEFINED DOCKER_WORKDIR)
    set(DOCKER_WORKDIR "/root")
  endif ()
  if (NOT DEFINED DOCKER_GROUP)
    set(DOCKER_LOC "${CMAKE_BINARY_DIR}/docker/${DOCKER_NAME}")
  else ()
    set(DOCKER_LOC "${CMAKE_BINARY_DIR}/docker/${DOCKER_GROUP}/${DOCKER_NAME}")
  endif ()
  if (NOT DEFINED DOCKER_BUNDLES_DIR)
    set(DOCKER_BUNDLES_DIR "/bundles")
  endif ()

  #ensure the docker dir will be deleted during clean
  get_directory_property(CLEANFILES ADDITIONAL_MAKE_CLEAN_FILES)
  list(APPEND CLEANFILES "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC>")
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEANFILES}")

  if (DOCKER_LAUNCHER)
    add_custom_target(${DOCKER_TARGET})
    if (IS_ABSOLUTE "${DOCKER_LAUNCHER}")
      set(LAUNCHER "${DOCKER_LAUNCHER}")
      get_filename_component(EXE_FILENAME ${DOCKER_LAUNCHER} NAME)
      set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"/bin/${EXE_FILENAME}\"]")
    else ()
      #assuming target
      set(LAUNCHER "$<TARGET_FILE:${DOCKER_LAUNCHER}>")
      set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"/bin/$<TARGET_FILE_NAME:${DOCKER_TARGET}>\"]")
    endif ()
  elseif (DOCKER_LAUNCHER_SRC)
    add_executable(${DOCKER_TARGET} EXCLUDE_FROM_ALL ${DOCKER_LAUNCHER_SRC})
    target_link_libraries(${DOCKER_TARGET} PRIVATE Celix::framework)
    set(LAUNCHER "$<TARGET_FILE:${DOCKER_TARGET}>")
    set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"/bin/$<TARGET_FILE_NAME:${DOCKER_TARGET}>\"]")
  else ()
    if (DOCKER_CXX)
      set(LAUNCHER_SRC "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-main.cc")
    elseif (DOCKER_C)
      set(LAUNCHER_SRC "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-main.c")
    else()
      if (CELIX_DEFAULT_CONTAINER_CXX_OPTION)
        set(LAUNCHER_SRC "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-main.cc")
      else()
        set(LAUNCHER_SRC "${CMAKE_CURRENT_BINARY_DIR}/${DOCKER_TARGET}-docker-main.c")
      endif()
    endif ()

    file(GENERATE
      OUTPUT ${LAUNCHER_SRC}
      CONTENT "#include <celix_launcher.h>

int main(int argc, char *argv[]) {
    const char * config = \"\\
$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_EMBEDDED_PROPERTIES>,\\n\\
>\";

    celix_properties_t *packedConfig = celix_properties_loadFromString(config);
    return celixLauncher_launchAndWaitForShutdown(argc, argv, packedConfig);
}
"
      )

    add_executable(${DOCKER_TARGET} EXCLUDE_FROM_ALL ${LAUNCHER_SRC})
    target_link_libraries(${DOCKER_TARGET} PRIVATE Celix::framework)
    if(NOT APPLE)
      #Add --no-as-needed options, to ensure that libraries are always linked.
      #This is needed because most libraries are not used by the executable, but by the bundle libraries instead.
      set_target_properties(${DOCKER_TARGET} PROPERTIES LINK_FLAGS -Wl,--no-as-needed)
    endif()
    set(LAUNCHER "$<TARGET_FILE:${DOCKER_TARGET}>")
    set(DOCKER_ENTRYPOINT "ENTRYPOINT [\"/bin/$<TARGET_FILE_NAME:${DOCKER_TARGET}>\"]")
  endif ()


  ###### Setup docker custom target timestamp
  add_custom_target(${DOCKER_TARGET}-deps
    DEPENDS ${FS_TIMESTAMP_FILE} $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_DEPS>
    )
  add_dependencies(${DOCKER_TARGET} ${DOCKER_TARGET}-deps)

  #setup dependencies based on timestamp
  if (DOCKER_CREATE_FS)
    add_custom_command(TARGET ${DOCKER_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC>
      COMMAND cd $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC> && /bin/bash ${CELIX_CMAKE_DIRECTORY}/create_target_filesystem.sh -e ${LAUNCHER} > /dev/null
      WORKING_DIRECTORY "${DOCKER_LOC}"
      COMMENT "Creating docker dir for ${DOCKER_TARGET}" VERBATIM
      )
  else ()
    add_custom_command(TARGET ${DOCKER_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC>/bin
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAUNCHER} $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC>/bin/
      WORKING_DIRECTORY "${DOCKER_LOC}"
      COMMENT "Creating docker dir for ${DOCKER_TARGET}" VERBATIM
      )
  endif ()

  ##### Deploy Target Properties for Dockerfile #####
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_LOC" "${DOCKER_LOC}")
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_FROM" "${DOCKER_FROM}") #name of docker base, default celix-base
  set_target_properties(${DOCKER_TARGET} PROPERTIES "CONTAINER_NAME" "${DOCKER_NAME}") #name of docker celix container
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_IMAGE_NAME" "${DOCKER_IMAGE_NAME}") #name of docker images, default deploy target name
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_DIR" "${DOCKER_BUNDLES_DIR}") #bundles directory in docker image
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_0" "") #bundles for runtime level 0
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_1" "") #bundles for runtime level 1
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_2" "") #bundles for runtime level 2
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_3" "") #bundles for runtime level 3
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_4" "") #bundles for runtime level 4
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_5" "") #bundles for runtime level 5
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_WORKDIR" "${DOCKER_WORKDIR}") #workdir in docker image, should also contain the config.properties
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_ENTRYPOINT" "${DOCKER_ENTRYPOINT}")
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_CREATE_FS" "${DOCKER_CREATE_FS}") #wether to create a fs with the minimal needed libraries / etc files
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_INSTRUCTIONS" "") #list of additional instructions
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_PROPERTIES" "")
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_EMBEDDED_PROPERTIES" "")
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_DEPS" "")

  set(DOCKERFILE_STAGE1 ${CMAKE_BINARY_DIR}/celix/gen/docker/${DOCKER_TARGET}/Dockerfile.in)
  set(DOCKERFILE "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC>/Dockerfile")

  file(GENERATE
    OUTPUT "${DOCKERFILE_STAGE1}"
    CONTENT "# Dockerfile for celix based image
FROM $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_FROM>
ENV IMAGE_NAME $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_IMAGE_NAME>
COPY . /
WORKDIR $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_WORKDIR>
$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_ENTRYPOINT>
$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_INSTRUCTIONS>,
>
")
  file(GENERATE
    OUTPUT ${DOCKERFILE}
    INPUT ${DOCKERFILE_STAGE1}
    )

  #generate config.properties
  set(DOCKER_PROPERTIES_FILE "${DOCKER_LOC}/${DOCKER_WORKDIR}/config.properties")
  set(STAGE1_PROPERTIES_FILE "${CMAKE_BINARY_DIR}/celix/gen/docker/${DOCKER_TARGET}/docker-config-stage1.properties")
  file(GENERATE
    OUTPUT "${STAGE1_PROPERTIES_FILE}"
    CONTENT "CELIX_AUTO_START_0=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_0>, >
CELIX_AUTO_START_1=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_1>, >
CELIX_AUTO_START_2=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_2>, >
CELIX_AUTO_START_3=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_3>, >
CELIX_AUTO_START_4=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_4>, >
CELIX_AUTO_START_5=$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_BUNDLES_LEVEL_5>, >
CELIX_CONTAINER_NAME=$<TARGET_PROPERTY:${DOCKER_TARGET},CONTAINER_NAME>
org.osgi.framework.storage=.cache
$<JOIN:$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_PROPERTIES>,
>
"
    )
  file(GENERATE
    OUTPUT "${DOCKER_PROPERTIES_FILE}"
    INPUT "${STAGE1_PROPERTIES_FILE}"
    )

  if (DOCKER_BUNDLES)
    celix_docker_bundles(${DOCKER_TARGET} LEVEL 3 ${DOCKER_BUNDLES})
  endif ()
  if (DOCKER_PROPERTIES)
    celix_docker_properties(${DOCKER_TARGET} ${DOCKER_PROPERTIES})
  endif ()
  if (DOCKER_INSTRUCTIONS)
    celix_docker_instructions(${DOCKER_TARGET} ${DOCKER_INSTRUCTIONS})
  endif ()
  if (DOCKER_FILES)
    cmake_docker_add_files(${DOCKER_TARGET} FILES ${DOCKER_FILES} DESTINATION ${DOCKER_FILES_DIR})
  endif ()

  get_target_property(DEPS celix-build-docker-dirs "DOCKER_DEPS")
  list(APPEND DEPS ${DOCKER_TARGET})
  set_target_properties(celix-build-docker-dirs PROPERTIES "DOCKER_DEPS" "${DEPS}")

  if (CELIX_BUILD_DOCKER_USE_DOCKER)
    add_custom_target(celix-build-${DOCKER_TARGET}-docker-image
      COMMAND cd $<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC> && ${DOCKER_CMD} build -t "$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_IMAGE_NAME>" .
      DEPENDS ${DOCKERFILE} ${DOCKER_TARGET}
      COMMENT "Creating docker image for target '${DOCKER_TARGET}'" VERBATIM
      )
    add_dependencies(celix-build-docker-images celix-build-${DOCKER_TARGET}-docker-image)
  endif()
endfunction()

#[[
Add the selected bundles to the Celix docker image. These bundles are copied to the docker build dir and
are added to the configuration properties so that they are installed and started when the Celix docker container is created and started.

The Celix framework support 7 (0 - 6) run levels. Run levels can be used to control the start and stop order of bundles.
Bundles in run level 0 are started first and bundles in run level 6 are started last.
When stopping bundles in run level 6 are stopped first and bundles in run level 0 are stopped last.
Within a run level the order of configured decides the start order; bundles added earlier are started first.


Optional Arguments:
- LEVEL: The run level for the added bundles. Default is 3.

```CMake
celix_docker_bundles(<celix_container_target_name>
    [LEVEL (0..5)]
    bundle1
    bundle2
    ...
)
```
]]
function(celix_docker_bundles)
  #0 is docker TARGET
  #1..n is bundles
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  set(OPTIONS)
  set(ONE_VAL_ARGS LEVEL)
  set(MULTI_VAL_ARGS)
  cmake_parse_arguments(BUNDLES "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
  set(BUNDLES_LIST ${BUNDLES_UNPARSED_ARGUMENTS})

  if (NOT DEFINED BUNDLES_LEVEL)
    set(BUNDLES_LEVEL 3)
  endif ()

  get_target_property(BUNDLES ${DOCKER_TARGET} "DOCKER_BUNDLES_LEVEL_${BUNDLES_LEVEL}")
  get_target_property(BUNDLES_DIR ${DOCKER_TARGET} "DOCKER_BUNDLES_DIR")
  get_target_property(LOC ${DOCKER_TARGET} "DOCKER_LOC")
  get_target_property(DEPS ${DOCKER_TARGET} "DOCKER_DEPS")

  foreach (BUNDLE IN ITEMS ${BUNDLES_LIST})
    set(HANDLED FALSE)
    if (IS_ABSOLUTE ${BUNDLE} AND EXISTS ${BUNDLE})
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
          _celix_extract_imported_bundle_info(${BUNDLE}) #extracts BUNDLE_FILE and BUNDLE_FILENAME
          set(OUT "${CMAKE_BINARY_DIR}/celix/gen/docker/${DOCKER_TARGET}/copy-bundle-for-target-${BUNDLE_ID}.timestamp")
          set(DEST "${LOC}/${BUNDLES_DIR}/${BUNDLE_FILENAME}")
          add_custom_command(OUTPUT ${OUT}
            COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${LOC}/${BUNDLES_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BUNDLE_FILE}" ${DEST}
            COMMENT "Copying (imported) bundle '${BUNDLE}' to '${LOC}/${BUNDLES_DIR}'"
            )
          list(APPEND BUNDLES "${BUNDLES_DIR}/${BUNDLE_FILENAME}")
          set(HANDLED TRUE)
        endif ()
      endif ()
    endif ()

    if (NOT HANDLED) #assuming (future) bundle target)
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
    endif ()
    list(APPEND DEPS "${OUT}")
  endforeach ()

  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_BUNDLES_LEVEL_${BUNDLES_LEVEL}" "${BUNDLES}")
  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_DEPS" "${DEPS}")
endfunction()

#[[
Same as `celix_container_properties`, but then for the celix container
in the docker image.

```CMake
celix_docker_properties(<docker_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```
]]
function(celix_docker_properties)
  #0 is docker TARGET
  #1..n is properties
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  get_target_property(PROPS ${DOCKER_TARGET} "DOCKER_PROPERTIES")

  foreach (PROP IN ITEMS ${ARGN})
    list(APPEND PROPS ${PROP})
  endforeach ()

  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_PROPERTIES" "${PROPS}")
endfunction()

#[[
Same as `celix_container_embedded_properties`, but then for the celix container
in the docker image.

```CMake
celix_docker_embedded_properties(<docker_target_name>
    "prop1=val1"
    "prop2=val2"
    ...
)
```
]]
function(celix_docker_embedded_properties)
  #0 is docker TARGET
  #1..n is properties
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  get_target_property(PROPS ${DOCKER_TARGET} "DOCKER_EMBEDDED_PROPERTIES")

  foreach (PROP IN ITEMS ${ARGN})
    list(APPEND PROPS ${PROP})
  endforeach ()

  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_EMBEDDED_PROPERTIES" "${PROPS}")
endfunction()

#[[
Add the provided docker instruction to the end of the generated
Dockerfile.

```CMake
celix_docker_instructions(<docker_target_name>
    "instruction1"
    "instruction2"
    ...
)
```
]]
function(celix_docker_instructions)
  #0 is docker TARGET
  #1..n is instructions
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  get_target_property(INSTRUCTIONS ${DOCKER_TARGET} "DOCKER_INSTRUCTIONS")

  foreach (INSTR IN ITEMS ${ARGN})
    list(APPEND INSTRUCTIONS ${INSTR})
  endforeach ()

  set_target_properties(${DOCKER_TARGET} PROPERTIES "DOCKER_INSTRUCTIONS" "${INSTRUCTIONS}")
endfunction()

function(cmake_docker_add_files)
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)
  set(OPTIONS)
  set(ONE_VAL_ARGS DESTINATION)
  set(MULTI_VAL_ARGS FILES)
  cmake_parse_arguments(DOCKER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
  if (NOT DEFINED DOCKER_DESTINATION)
    set(DOCKER_DESTINATION)
  endif ()
  get_target_property(DOCKER_DIR_LOCATION ${DOCKER_TARGET} "DOCKER_LOC")
  foreach(DOCKER_FILE IN ITEMS ${DOCKER_FILES})
    add_custom_command(TARGET ${DOCKER_TARGET}
      POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCKER_DIR_LOCATION}/${DOCKER_DESTINATION}
      POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DOCKER_FILE}" ${DOCKER_DIR_LOCATION}/${DOCKER_DESTINATION}
      COMMENT "Copying file '${DOCKER_FILE}' to '${DOCKER_DIR_LOCATION}/${DOCKER_DESTINATION}'"
      )
  endforeach()
endfunction()


#[[
Install docker container when 'make install' is executed and CELIX_BUILD_DOCKER_USE_DOCKER_DIR_TAR is defined.
Docker containers are installed at `<install-prefix>/shared/<project_name>/containers`.

CELIX_BUILD_DOCKER_TAR_LOCATION can be defined to override the default install-prefix.

Optional arguments:
- PROJECT_NAME: The project name for installing. Default is the cmake project name.
- IMAGE_NAME: Configure the image name. Default is NAME.

install_celix_docker(<docker_target_name>
    [PROJECT_NAME] project_name
    [IMAGE_NAME docker_image_name]
)
]]
function(install_celix_docker)
  list(GET ARGN 0 DOCKER_TARGET)
  list(REMOVE_AT ARGN 0)

  set(OPTIONS )
  set(ONE_VAL_ARGS PROJECT_NAME)
  set(MULTI_VAL_ARGS )
  cmake_parse_arguments(DOCKER "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

  if (NOT DEFINED DOCKER_PROJECT_NAME)
    string(TOLOWER ${PROJECT_NAME} DOCKER_PROJECT_NAME)
  endif()
  if (NOT DEFINED DOCKER_NAME)
    set(DOCKER_NAME "${DOCKER_TARGET}")
  endif ()
  if (NOT DEFINED DOCKER_IMAGE_NAME)
    set(DOCKER_IMAGE_NAME "${DOCKER_NAME}")
  endif ()

  if(CELIX_BUILD_DOCKER_USE_DOCKER_DIR_TAR)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${DOCKER_PROJECT_NAME}/${DOCKER_IMAGE_NAME}.tar.gz
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${DOCKER_PROJECT_NAME}
            COMMAND tar czf ${CMAKE_BINARY_DIR}/${DOCKER_PROJECT_NAME}/${DOCKER_IMAGE_NAME}.tar.gz --directory=$<TARGET_PROPERTY:${DOCKER_TARGET},DOCKER_LOC> .
            DEPENDS ${DOCKERFILE} ${DOCKER_TARGET}
            COMMENT "Generating '${DOCKER_IMAGE_NAME}.tar.gz'" VERBATIM
            )
    add_custom_target(${DOCKER_IMAGE_NAME}-dirs-tars ALL DEPENDS ${CMAKE_BINARY_DIR}/${DOCKER_PROJECT_NAME}/${DOCKER_IMAGE_NAME}.tar.gz)

    if (DEFINED CELIX_BUILD_DOCKER_TAR_LOCATION)
      set(DOCKER_TAR_LOCATION ${CELIX_BUILD_DOCKER_TAR_LOCATION})
    endif()
    if (NOT DEFINED DOCKER_TAR_LOCATION)
      set(DOCKER_TAR_LOCATION shared)
    endif()
    install(FILES ${CMAKE_BINARY_DIR}/${DOCKER_PROJECT_NAME}/${DOCKER_IMAGE_NAME}.tar.gz DESTINATION ${DOCKER_TAR_LOCATION}/${DOCKER_PROJECT_NAME}/containers)
  endif()
endfunction()