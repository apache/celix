
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


set(CELIX_NO_POSTFIX_BUILD_TYPES RelWithDebInfo Release CACHE STRING "The build type used for creating bundle without a build type postfix.")

find_program(JAR_COMMAND jar NO_CMAKE_FIND_ROOT_PATH)

if(JAR_COMMAND AND NOT CELIX_USE_ZIP_INSTEAD_OF_JAR)
    message(STATUS "Using jar to create bundles")
else()
    find_program(ZIP_COMMAND zip NO_CMAKE_FIND_ROOT_PATH)
    if(ZIP_COMMAND)
        message(STATUS "Using zip to create bundles")
    else()
        message(FATAL_ERROR "A jar or zip command is needed to jar/zip bundles")
    endif()
endif()


##### setup bundles/deploy target
if (NOT TARGET celix-bundles)
    add_custom_target(celix-bundles ALL)
endif ()
#####

macro(extract_version_parts VERSION MAJOR MINOR PATCH)
    set(MAJOR "0")
    set(MINOR "0")
    set(PATCH "0")

    string(REGEX MATCH "^([0-9]+).*" TMP "${VERSION}")
    if (CMAKE_MATCH_1)
        set(MAJOR ${CMAKE_MATCH_1})
    endif()
    string(REGEX MATCH "^([0-9]+)\\.([0-9])+.*" TMP "${VERSION}")
    if (CMAKE_MATCH_2)
        set(MINOR ${CMAKE_MATCH_2})
    endif()
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*" TMP "${VERSION}")
    if (CMAKE_MATCH_3)
        set(PATCH ${CMAKE_MATCH_3})
    endif()
endmacro()

function(set_library_version TARGET VERSION) 
    if (VERSION AND TARGET)
        extract_version_parts("${VERSION}" MAJOR MINOR PATCH)

        #NOTE setting aligning ABI version with major part of the interface version.
        #This is simpeler than using the <current>:<revision>:<age> approach of libtool
        set_property(TARGET ${TARGET} PROPERTY VERSION "${VERSION}")
        set_property(TARGET ${TARGET} PROPERTY SOVERSION ${MAJOR})
    else ()
        message(WARNING "set_library_version: Cannot set version info TARGET and/or VERSION not provided")
    endif ()
endfunction()

function(check_lib LIB)
   if(TARGET ${LIB})
        #ok
    elseif(IS_ABSOLUTE ${LIB} AND EXISTS ${LIB})
        #ok
    else() 
        message(FATAL_ERROR "Provided library (${LIB}) is not a target nor a absolute path to an existing library")
    endif()
endfunction()

function(check_bundle BUNDLE)
    if(TARGET ${BUNDLE})
        get_target_property(BUNDLE_FILE ${BUNDLE} "BUNDLE_FILE")
        if(NOT BUNDLE_FILE)
            message(FATAL_ERROR "Provided target must be a bundle target")
        endif()
    else()
        message(FATAL_ERROR "first argument must be a target")
    endif()
endfunction()


function(add_bundle)
    message(DEPRECATION "add_bundle is deprecated, use add_celix_bundle instead.")
    add_celix_bundle(${ARGN})
endfunction()

#[[
Add a Celix bundle to the project.  There are three variants:
- With SOURCES the bundle will be created using a list of sources files as input for the bundle activator library.
- With ACTIVATOR the bundle will be created using the library target or absolute path to existing library as activator library.
- With the NO_ACTIVATOR option will create a bundle without a activator (i.e. a pure resource bundle).

Optional arguments are:
- NAME: The (human readable) name of the bundle. This will be used as Bundle-Name manifest entry. Default is the  <bundle_target_name>.
- SYMBOLIC_NAME: The symbolic name of the bundle. This will be used as Bundle-SymbolicName manifest entry. Default is the <bundle_target_name>.
- DESCRIPTION: The description of the bundle. This will be used as Bundle-Description manifest entry. Default this is empty.
- GROUP: The group the bundle is part of. This will be used as Bundle-Group manifest entry. Default this is empty (no group).
- VERSION: The bundle version. This will be used for the Bundle-Version manifest entry. In combination with SOURCES the version will also be used to set the activator library target property VERSION and SOVERSION.
  For SOVERSION only the major part is used. Expected scheme is "<major>.<minor>.<path>". Default version is "0.0.0"
- FILENAME: The filename of the bundle file, without extension. Default is <bundle_target_name>. Together with the BUILD_TYPE, this will result in a filename like "bundle_target_name_Debug.zip
- PRIVATE_LIBRARIES: private libraries to be included in the bundle. Specified libraries are added to the "Private-Library" manifest statement and added in the root of the bundle. libraries can be cmake library targets or absolute paths to existing libraries.
- HEADERS: Additional headers values that are appended to the bundle manifest.

```CMake
add_celix_bundle(<bundle_target_name>
        SOURCES source1 source2 ...
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_celix_bundle(<bundle_target_name>
        ACTIVATOR <activator_lib>
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
```

```CMake
add_celix_bundle(<bundle_target_name>
        NO_ACTIVATOR
        [NAME bundle_name]
        [SYMBOLIC_NAME bundle_symbolic_name]
        [DESCRIPTION bundle_description]
        [GROUP bundle_group]
        [VERSION bundle_version]
        [FILENAME bundle_filename]
        [PRIVATE_LIBRARIES private_lib1 private_lib2 ...]
        [HEADERS "header1: header1_value" "header2: header2_value" ...]
)
]]
function(add_celix_bundle)
    list(GET ARGN 0 BUNDLE_TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS NO_ACTIVATOR)
    set(ONE_VAL_ARGS VERSION ACTIVATOR SYMBOLIC_NAME NAME DESCRIPTION FILENAME GROUP)
    set(MULTI_VAL_ARGS SOURCES PRIVATE_LIBRARIES EXPORT_LIBRARIES IMPORT_LIBRARIES HEADERS)
    cmake_parse_arguments(BUNDLE "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##check arguments
    if (NOT DEFINED BUNDLE_TARGET_NAME)
        message(FATAL_ERROR "add_bunde function requires first target name argument")
    endif ()
    if ((NOT (BUNDLE_SOURCES OR BUNDLE_ACTIVATOR)) AND (NOT BUNDLE_NO_ACTIVATOR))
        message(FATAL_ERROR "Bundle contain no SOURCES or ACTIVATOR target and the option NO_ACTIVATOR is not set")
    endif ()
    if (BUNDLE_SOURCES AND BUNDLE_ACTIVATOR)
        message(FATAL_ERROR "add_bundle function requires a value for SOURCES or ACTIVATOR not both")
    endif ()
    if (BUNDLE_ACTIVATOR)
        check_lib(${BUNDLE_ACTIVATOR})
    endif ()
    if (NOT DEFINED BUNDLE_GROUP)
        set(BUNDLE_GROUP "")
    endif ()

    #setting defaults
    if(NOT BUNDLE_VERSION) 
        set(BUNDLE_VERSION "0.0.0")
        message(WARNING "Bundle version for ${BUNDLE_NAME} not provided. Using 0.0.0")
    endif ()
    if (NOT DEFINED BUNDLE_NAME)
        set(BUNDLE_NAME ${BUNDLE_TARGET_NAME})
    endif ()
    if (NOT DEFINED BUNDLE_SYMBOLIC_NAME)
        set(BUNDLE_SYMBOLIC_NAME ${BUNDLE_TARGET_NAME})
    endif ()

    if (NOT DEFINED BUNDLE_FILENAME)
        set(BASE_BUNDLE_FILENAME ${BUNDLE_TARGET_NAME})
    else ()
        set(BASE_BUNDLE_FILENAME ${BUNDLE_FILENAME})
    endif ()


    if (CMAKE_BUILD_TYPE)
        set(BUNDLE_FILENAME ${BASE_BUNDLE_FILENAME}-${CMAKE_BUILD_TYPE}.zip)
        foreach (NO_POSTFIX_BT IN LISTS CELIX_NO_POSTFIX_BUILD_TYPES)
            if (CMAKE_BUILD_TYPE STREQUAL NO_POSTFIX_BT)
                #setting bundle file name without postfix
                set(BUNDLE_FILENAME ${BASE_BUNDLE_FILENAME}.zip)
            endif ()
        endforeach ()
    else ()
        #note if no CMAKE_BUILD_TYPE is set, do not use a postfix
        set(BUNDLE_FILENAME ${BASE_BUNDLE_FILENAME}.zip)
    endif ()


    set(BUNDLE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_FILENAME}")
    #set(BUNDLE_CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_TARGET_NAME}_content")
    #set(BUNDLE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_TARGET_NAME}_gen")
    set(BUNDLE_CONTENT_DIR "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE_TARGET_NAME}/content")
    set(BUNDLE_GEN_DIR "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE_TARGET_NAME}")


    ###### Setting up dependency for bundles target
    get_target_property(DEPS celix-bundles "BUNDLES_DEPS")
    list(APPEND DEPS "${BUNDLE_FILE}")
    set_target_properties(celix-bundles PROPERTIES "BUNDLES_DEPS" "${DEPS}")
    #####

    ####### Setting target for activator lib if neccesary ####################
    if(BUNDLE_SOURCES)
        #create lib from sources
        add_library(${BUNDLE_TARGET_NAME} SHARED ${BUNDLE_SOURCES})
        set_library_version(${BUNDLE_TARGET_NAME} ${BUNDLE_VERSION})
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES
                "BUNDLE_TARGET_IS_LIB" TRUE
                "BUNDLE_TARGET" "${BUNDLE_TARGET_NAME}_bundle"
        )
        target_link_libraries(${BUNDLE_TARGET_NAME} PRIVATE Celix::framework)
    else()
        add_custom_target(${BUNDLE_TARGET_NAME})
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES
                "BUNDLE_TARGET_IS_LIB" FALSE
                "BUNDLE_TARGET" "${BUNDLE_TARGET_NAME}_bundle"
        )
    endif()
    add_custom_target(${BUNDLE_TARGET_NAME}_bundle
        DEPENDS ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_FILE>"
    )
    add_dependencies(celix-bundles ${BUNDLE_TARGET_NAME}_bundle)
    #######################################################################
   

    ##### MANIFEST configuration and generation ##################
    #Step1 configure the file so that the target name is present in in the template
    configure_file(${CELIX_CMAKE_DIRECTORY}/Manifest.template.in ${BUNDLE_GEN_DIR}/MANIFEST.step1)

    #Step2 replace headers with target property values. Note this is done build time
    file(GENERATE 
        OUTPUT "${BUNDLE_GEN_DIR}/MANIFEST.step2"
        INPUT "${BUNDLE_GEN_DIR}/MANIFEST.step1"
    )

    #Step3 The replaced values in step 2 can contain generator expresssion, generated again to resolve those. Note this is done build time
    file(GENERATE 
        OUTPUT "${BUNDLE_GEN_DIR}/MANIFEST.MF"
        INPUT "${BUNDLE_GEN_DIR}/MANIFEST.step2"
    )   
    #########################################################

    ###### Packaging the bundle using using jar or zip and a content dir. Configuring dependencies ######
    if(JAR_COMMAND)
        add_custom_command(OUTPUT ${BUNDLE_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_CONTENT_DIR}
            COMMAND ${JAR_COMMAND} -cfm ${BUNDLE_FILE} ${BUNDLE_GEN_DIR}/MANIFEST.MF -C ${BUNDLE_CONTENT_DIR} .
            COMMENT "Packaging ${BUNDLE_TARGET_NAME}"
            DEPENDS  ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_DEPEND_TARGETS>" ${BUNDLE_GEN_DIR}/MANIFEST.MF
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    elseif(ZIP_COMMAND)
        file(MAKE_DIRECTORY ${BUNDLE_CONTENT_DIR})

        add_custom_command(OUTPUT ${BUNDLE_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_GEN_DIR}/MANIFEST.MF META-INF/MANIFEST.MF
            COMMAND ${ZIP_COMMAND} -rq ${BUNDLE_FILE} *
            COMMENT "Packaging ${BUNDLE_TARGET_NAME}"
            DEPENDS ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_DEPEND_TARGETS>" ${BUNDLE_GEN_DIR}/MANIFEST.MF
            WORKING_DIRECTORY ${BUNDLE_CONTENT_DIR}
        )
    else()
        message(FATAL_ERROR "A jar or zip command is needed to jar/zip bundles")
    endif()
    ###################################################################################


    ###################################
    ##### Additional Cleanup info #####
    ###################################
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_GEN_DIR>;$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_CONTENT_DIR>")

    #############################
    ### BUNDLE TARGET PROPERTIES
    #############################
    #already set
    #   BUNDLE_TARGET_IS_LIB -> true (can be use to test if target is bundle target
    #   BUNDLE_TARGET -> refers to the _bundle target which is responsible for building the zip file
    #internal use
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_IS_BUNDLE_TARGET" TRUE) #indicate that this is a bundle target
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_DEPEND_TARGETS" "") #bundle target dependencies. Note can be extended after the add_bundle call
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_GEN_DIR" ${BUNDLE_GEN_DIR}) #location for generated output.
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_CREATE_BUNDLE_TARGET" ${BUNDLE_TARGET_NAME}_bundle) #target which creat the bundle zip
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_IMPORTED" FALSE) #whether target is a imported (bundle) target

    #bundle specific
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_CONTENT_DIR" ${BUNDLE_CONTENT_DIR}) #location where the content to be jar/zipped.
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_FILENAME" ${BUNDLE_FILENAME}) #target bundle filename (.zip)
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_FILE" ${BUNDLE_FILE}) #target bundle abs file path (.zip)

    #name and version
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_NAME" ${BUNDLE_NAME}) #The bundle name default target name
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_SYMBOLIC_NAME" ${BUNDLE_SYMBOLIC_NAME}) #The bundle symbolic name. Default target name
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_GROUP" "${BUNDLE_GROUP}") #The bundle group, default ""
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_VERSION" ${BUNDLE_VERSION}) #The bundle version. Default 0.0.0
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_DESCRIPTION" "${BUNDLE_DESCRIPTION}") #The bundle description.

    #headers
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" 1) #Library containing the activator (if any)
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_PRIVATE_LIBS" "") #List of private libs. 
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_IMPORT_LIBS" "") #List of libs to import
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_EXPORT_LIBS" "") #list of libs to export
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_HEADERS" "") #Additional headers will be added (new line seperated) to the manifest
    ################################
    ################################

    if(BUNDLE_SOURCES) 
        celix_bundle_libs(${BUNDLE_TARGET_NAME} "PRIVATE" TRUE ${BUNDLE_TARGET_NAME})
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" "$<TARGET_SONAME_FILE_NAME:${BUNDLE_TARGET_NAME}>")
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUILD_WITH_INSTALL_RPATH" true)

        if(APPLE)
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES INSTALL_RPATH "@loader_path")
        else()
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        endif()
    elseif(BUNDLE_NO_ACTIVATOR)
        #do nothing
    else() #ACTIVATOR 
        celix_bundle_libs(${BUNDLE_TARGET_NAME} "PRIVATE" TRUE ${BUNDLE_ACTIVATOR})
        
        if(TARGET ${BUNDLE_ACTIVATOR})
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" "$<TARGET_SONAME_FILE_NAME:${BUNDLE_ACTIVATOR}>")
        elseif(IS_ABSOLUTE ${BUNDLE_ACTIVATOR} AND EXISTS${BUNDLE_ACTIVATOR})
            get_filename_component(ACT_NAME ${BUNDLE_ACTIVATOR} NAME)
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" "${ACT_NAME}>")
        else()
            message(FATAL_ERROR "Provided library (${BUNDLE_ACTIVATOR}) is not a target nor a absolute path to an existing library")
        endif()

    endif()


    celix_bundle_private_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_PRIVATE_LIBRARIES})
    celix_bundle_export_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_EXPORT_LIBRARIES})
    celix_bundle_import_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_IMPORT_LIBRARIES})
    celix_bundle_headers(${BUNDLE_TARGET_NAME} ${BUNDLE_HEADERS})
endfunction()

function(bundle_export_libs)
    message(DEPRECATION "bundle_export_libs is deprecated, use celix_bundle_export_libs instead.")
    celix_bundle_export_libs(${ARGN})
endfunction()
function(celix_bundle_export_libs)
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)
    celix_bundle_libs(${BUNDLE} "EXPORT" TRUE ${ARGN})
endfunction()

function(bundle_private_libs)
    message(DEPRECATION "bundle_private_libs is deprecated, use celix_bundle_private_libs instead.")
    celix_bundle_private_libs(${ARGN})
endfunction()

#[[
Add libraries to a bundle. The libraries should be cmake library targets or an absolute path to an existing library.

The libraries will be copied into the bundle zip and activator library will be linked (PRIVATE) against them.

celix_bundle_private_libs(<bundle_target>
    lib1 lib2 ...
)
]]
function(celix_bundle_private_libs)
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)
    celix_bundle_libs(${BUNDLE} "PRIVATE" FALSE ${ARGN})
endfunction()

function(bundle_libs)
    message(DEPRECATION "bundle_libs is deprecated, use celix_bundle_libs instead.")
    celix_bundle_libs(${ARGN})
endfunction()
function(celix_bundle_libs)
    #0 is bundle TARGET
    #1 is TYPE, e.g PRIVATE,EXPORT or IMPORT
    #2 is ADD_TO_MANIFEST 
    #2..n is libs
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    list(GET ARGN 0 TYPE)
    list(REMOVE_AT ARGN 0)

    list(GET ARGN 0 ADD_TO_MANIFEST)
    list(REMOVE_AT ARGN 0)

    #check if arg 0 is corrent
    check_bundle(${BUNDLE})
    get_target_property(BUNDLE_DIR ${BUNDLE} "BUNDLE_CONTENT_DIR")
    get_target_property(BUNDLE_GEN_DIR ${BUNDLE} "BUNDLE_GEN_DIR")


    get_target_property(LIBS ${BUNDLE} "BUNDLE_${TYPE}_LIBS")
    get_target_property(DEPS ${BUNDLE} "BUNDLE_DEPEND_TARGETS")

    foreach(LIB IN ITEMS ${ARGN})
        string(MAKE_C_IDENTIFIER ${LIB} LIBID)
        if(IS_ABSOLUTE ${LIB} AND EXISTS ${LIB})
            get_filename_component(LIB_NAME ${LIB} NAME) 
            set(OUT "${BUNDLE_DIR}/${LIB_NAME}") 
            add_custom_command(OUTPUT ${OUT} 
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB} ${OUT} 
            )
            if (ADD_TO_MANIFEST)
                list(APPEND LIBS ${LIB_NAME})
            endif()
            list(APPEND DEPS ${OUT}) 
        elseif (TARGET ${LIB})
            get_target_property(TARGET_TYPE ${LIB} TYPE)
            #NOTE add_custom_command does not support generator expression in OUTPUT value (e.g. $<TARGET_FILE:${LIB}>)
            #Using a two step approach to be able to use add_custom_command instead of add_custom_target
            set(OUT "${BUNDLE_GEN_DIR}/lib-${LIBID}-copy-timestamp")
            if ("${TARGET_TYPE}" STREQUAL "STATIC_LIBRARY")
                add_custom_command(OUTPUT ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${LIB}>" "${BUNDLE_DIR}/$<TARGET_FILE_NAME:${LIB}>"
                    DEPENDS ${LIB}
                )
            elseif ("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
                add_custom_command(OUTPUT ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${LIB}>" "${BUNDLE_DIR}/$<TARGET_SONAME_FILE_NAME:${LIB}>"
                    DEPENDS ${LIB}
                    )
            else()
                message(FATAL_ERROR "Unexptected target type (${TARGET_TYPE}) for target ${LIB}. Not a library")
            endif()
            if (ADD_TO_MANIFEST)
                list(APPEND LIBS "$<TARGET_SONAME_FILE_NAME:${LIB}>")
            endif()
            list(APPEND DEPS "${OUT}") #NOTE depending on ${OUT} not on $<TARGET_FILE:${LIB}>.
        else()
            message(FATAL_ERROR "Unexpected library argument '${LIB}'. Expected a absolute path to a library or a cmake library target")
        endif()

        get_target_property(IS_LIB ${BUNDLE} "BUNDLE_TARGET_IS_LIB")
        if ("${LIB}" STREQUAL "${BUNDLE}")
            #ignore. Do not have to link agaist own lib
        elseif(IS_LIB)
            target_link_libraries(${BUNDLE} PRIVATE ${LIB})
        endif()
    endforeach()


    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_${TYPE}_LIBS" "${LIBS}")
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DEPEND_TARGETS" "${DEPS}")
endfunction()

function(bundle_import_libs)
    message(DEPRECATION "bundle_import_libs is deprecated, use celix_bundle_import_libs instead.")
    celix_bundle_import_libs(${ARGN})
endfunction()

function(celix_bundle_import_libs)
    #0 is bundle TARGET
    #2..n is import libs
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    #check if arg 0 is corrent
    check_bundle(${BUNDLE})

    get_target_property(LIBS ${BUNDLE} "BUNDLE_IMPORT_LIBS")
    set(LIBS )

    foreach(LIB IN ITEMS ${ARGN})
        if(IS_ABSOLUTE ${LIB} AND EXISTS ${LIB})
            list(APPEND LIBS ${LIB_NAME})
        else()
            list(APPEND LIBS "$<TARGET_SONAME_FILE_NAME:${LIB}>")
        endif()

        target_link_libraries(${BUNDLE} PRIVATE ${LIB})
    endforeach()


    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_IMPORT_LIBS" "${LIBS}")
endfunction()

function(bundle_files)
    message(DEPRECATION "bundle_files is deprecated, use celix_bundle_files instead.")
    celix_bundle_files(${ARGN})
endfunction()

#[[
Add files to the target bundle. DESTINATION is relative to the bundle archive root.
The rest of the command is conform file(COPY ...) cmake command.
See cmake file(COPY ...) command for more info.

Note with celix_bundle_files, files are copied cmake generation time. Updates are not copied !!

celix_bundle_files(<bundle_target>
    files... DESTINATION <dir>
    [FILE_PERMISSIONS permissions...]
    [DIRECTORY_PERMISSIONS permissions...]
    [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
    [FILES_MATCHING]
    [[PATTERN <pattern> | REGEX <regex>]
    [EXCLUDE] [PERMISSIONS permissions...] [...]
)
]]
function(celix_bundle_files)
    #0 is bundle TARGET
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS DESTINATION)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(FILES "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    get_target_property(BUNDLE_DIR ${BUNDLE} "BUNDLE_CONTENT_DIR") 

    if (FILES_DESTINATION)
        set(DESTINATION "${BUNDLE_DIR}/${FILES_DESTINATION}")
    else()
        set(DESTINATION "${BUNDLE_DIR}")
    endif()

    #message("call: files(COPY ${FILES_UNPARSED_ARGUMENTS} DESTINATION \"${DESTINATION}\"")
    file(COPY ${FILES_UNPARSED_ARGUMENTS} DESTINATION ${DESTINATION})
endfunction()

#Note celix_bundle_add_dir copies the dir and can track changes.
function(celix_bundle_add_dir)
    #0 is bundle TARGET
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    #1 is the input dir
    list(GET ARGN 0 INPUT_DIR)
    list(REMOVE_AT ARGN 0)

    if (NOT DEFINED BUNDLE OR NOT DEFINED INPUT_DIR)
        message(FATAL_ERROR "celix_bundle_dir must have atleast two arguments: BUNDLE_TARGET and INPUT_DIR!")
    endif()

    set(OPTIONS )
    set(ONE_VAL_ARGS DESTINATION)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(COPY "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    get_target_property(BUNDLE_DIR ${BUNDLE} "BUNDLE_CONTENT_DIR")
    if (NOT DEFINED COPY_DESTINATION)
        set(DESTINATION "${BUNDLE_DIR}")
    else()
	set(DESTINATION "${BUNDLE_DIR}/${COPY_DESTINATION}")
    endif()

    string(UUID COPY_ID NAMESPACE "661ee07c-842d-11e8-adfc-80fa5b02e11b" NAME "${INPUT_DIR}" TYPE MD5)

    set(COPY_CMAKE_SCRIPT "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE}/copy-dir-${COPY_ID}.cmake")
    if (IS_ABSOLUTE ${INPUT_DIR})
	    file(WRITE ${COPY_CMAKE_SCRIPT}
		    "file(COPY ${INPUT_DIR} DESTINATION ${DESTINATION})")
    else()
	    file(WRITE ${COPY_CMAKE_SCRIPT}
		    "file(COPY ${CMAKE_CURRENT_LIST_DIR}/${INPUT_DIR} DESTINATION ${DESTINATION})")
    endif()

    set(TIMESTAMP "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE}/copy-dir-${COPY_ID}.timestamp")
    file(GLOB DIR_FILES ${INPUT_DIR})
    add_custom_command(OUTPUT ${TIMESTAMP}
            COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP}
            COMMAND ${CMAKE_COMMAND} -P ${COPY_CMAKE_SCRIPT}
            DEPENDS ${DIR_FILES}
            COMMENT "Copying dir ${INPUT_DIR} to ${DESTINATION}"
    )

    get_target_property(DEPS ${BUNDLE} "BUNDLE_DEPEND_TARGETS")
    list(APPEND DEPS "${TIMESTAMP}")
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DEPEND_TARGETS" "${DEPS}")
endfunction()

function(celix_bundle_add_files)
    #0 is bundle TARGET
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS DESTINATION)
    set(MULTI_VAL_ARGS FILES)
    cmake_parse_arguments(COPY "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    get_target_property(BUNDLE_DIR ${BUNDLE} "BUNDLE_CONTENT_DIR")
    if (NOT DEFINED COPY_DESTINATION)
        set(DESTINATION "${BUNDLE_DIR}")
    else()
	set(DESTINATION "${BUNDLE_DIR}/${COPY_DESTINATION}")
    endif()

    string(UUID COPY_ID NAMESPACE "661ee07c-842d-11e8-adfc-80fa5b02e11b" NAME "${COPY_FILES}" TYPE MD5)

    set(TIMESTAMP "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE}/copy-files-${COPY_ID}.timestamp")
    set(COPY_CMAKE_SCRIPT "${CMAKE_BINARY_DIR}/celix/gen/bundles/${BUNDLE}/copy-files-${COPY_ID}.cmake")
    file(WRITE ${COPY_CMAKE_SCRIPT}
	    "#Copy script, copies the file on a file per file base\n")
    foreach(FILE IN ITEMS ${COPY_FILES})
	    if (IS_ABSOLUTE ${FILE})
	    	file(APPEND ${COPY_CMAKE_SCRIPT}
			"file(COPY ${FILE} DESTINATION ${DESTINATION})\n")
	    else()
	    	file(APPEND ${COPY_CMAKE_SCRIPT}
			"file(COPY ${CMAKE_CURRENT_LIST_DIR}/${FILE} DESTINATION ${DESTINATION})\n")
	    endif()
    endforeach()
    add_custom_command(OUTPUT ${TIMESTAMP}
            COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP}
            COMMAND ${CMAKE_COMMAND} -P ${COPY_CMAKE_SCRIPT}
	        DEPENDS ${COPY_FILES}
	        COMMENT "Copying files to ${DESTINATION}"
    )

    get_target_property(DEPS ${BUNDLE} "BUNDLE_DEPEND_TARGETS")
    list(APPEND DEPS "${TIMESTAMP}")
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DEPEND_TARGETS" "${DEPS}")
endfunction()

function(bundle_headers)
    message(DEPRECATION "bundle_headers is deprecated, use celix_bundle_headers instead.")
    celix_bundle_headers(${ARGN})
endfunction()

#[[
Append the provided headers to the target bundle manifest.

celix_bundle_headers(<bundle_target>
    "header1: header1_value"
    "header2: header2_value"
    ...
)
]]
function(celix_bundle_headers)
    #0 is bundle TARGET
    #1..n is header name / header value
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    get_target_property(HEADERS ${BUNDLE} "BUNDLE_HEADERS")

    foreach(HEADER IN ITEMS ${ARGN})
        list(APPEND HEADERS "${HEADER}")
    endforeach()

    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_HEADERS" "${HEADERS}")
endfunction()

function(bundle_symbolic_name)
    message(DEPRECATION "bundle_symbolic_name is deprecated, use celix_bundle_symbolic_name instead.")
    celix_bundle_symbolic_name(${ARGN})
endfunction()

#[[
Set bundle symbolic name
celix_bundle_symbolic_name(<bundle_target> symbolic_name)
]]
function(celix_bundle_symbolic_name BUNDLE SYMBOLIC_NAME)
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_SYMBOLIC_NAME" ${SYMBOLIC_NAME})
endfunction()

#[[
Set bundle group.
celix_bundle_group(<bundle_target> bundle group)
]]
function(celix_bundle_group BUNDLE GROUP)
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_GROUP" ${GROUP})
endfunction()

function(bundle_name)
    message(DEPRECATION "bundle_name is deprecated, use celix_bundle_name instead.")
    celix_bundle_symbolic_name(${ARGN})
endfunction()

#[[
Set bundle name
celix_bundle_name(<bundle_target> name)
]]
function(celix_bundle_name BUNDLE NAME)
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_NAME" ${NAME})
endfunction()

function(bundle_version)
    message(DEPRECATION "bundle_version is deprecated, use celix_bundle_version instead.")
    celix_bundle_symbolic_name(${ARGN})
endfunction()

#[[
Set bundle version
celix_bundle_version(<bundle_target> version)
]]
function(celix_bundle_version BUNDLE VERSION)
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_VERSION" ${VERSION})
endfunction()

function(bundle_description)
    message(DEPRECATION "bundle_description is deprecated, use celix_bundle_description instead.")
    celix_bundle_symbolic_name(${ARGN})
endfunction()

#[[
Set bundle description
celix_bundle_description(<bundle_target> description)
]]
function(celix_bundle_description BUNDLE DESC)
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DESCRIPTION" ${DESC})
endfunction()

function(install_bundle)
    message(DEPRECATION "install_bundle is deprecated, use install_celix_bundle instead.")
    install_celix_bundle(${ARGN})
endfunction()

#[[
Install bundle when 'make install' is executed.
Bundles are installed at `<install-prefix>/share/<project_name>/bundles`.
Headers are installed at `<install-prefix>/include/<project_name>/<bundle_name>`
Resources are installed at `<install-prefix>/shared/<project_name>/<bundle_name>`

Optional arguments:
- EXPORT: Associates the installed bundle with a export_name.
  The export name can be used to generate a CelixTargets.cmake file (see install_celix_bundle_targets)
- PROJECT_NAME: The project name for installing. Default is the cmake project name.
- BUNDLE_NAME: The bundle name used when installing headers/resources. Default is the bundle target name.
- HEADERS: A list of headers to install for the bundle.
- RESOURCES: A list of resources to install for the bundle.

install_celix_bundle(<bundle_target>
    [EXPORT] export_name
    [PROJECT_NAME] project_name
    [BUNDLE_NAME] bundle_name
    [HEADERS header_file1 header_file2 ...]
    [RESOURCES resource1 resource2 ...]
)
]]
function(install_celix_bundle)
    #0 is bundle TARGET
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS PROJECT_NAME BUNDLE_NAME EXPORT)
    set(MULTI_VAL_ARGS HEADERS RESOURCES)
    cmake_parse_arguments(INSTALL "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    
    if (NOT DEFINED INSTALL_PROJECT_NAME)
        string(TOLOWER ${PROJECT_NAME} INSTALL_PROJECT_NAME)
    endif()
    if (NOT DEFINED INSTALL_BUNDLE_NAME)
        set(INSTALL_BUNDLE_NAME ${BUNDLE})
    endif()

    install(FILES "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" DESTINATION share/${INSTALL_PROJECT_NAME}/bundles COMPONENT ${BUNDLE})

    if (INSTALL_EXPORT)
        get_target_property(CURRENT_EXPORT_BUNDLES celix-bundles EXPORT_${INSTALL_EXPORT}_BUNDLES)

    	if (NOT CURRENT_EXPORT_BUNDLES)
            set(CURRENT_EXPORT_BUNDLES ${BUNDLE})
        else ()
            list(APPEND CURRENT_EXPORT_BUNDLES ${BUNDLE})
        endif ()

        list(REMOVE_DUPLICATES CURRENT_EXPORT_BUNDLES)

        set_target_properties(celix-bundles PROPERTIES
                EXPORT_${INSTALL_EXPORT}_BUNDLES "${CURRENT_EXPORT_BUNDLES}"
        )
    endif ()

    if(INSTALL_HEADERS)
        install (FILES ${INSTALL_HEADERS} DESTINATION include/${INSTALL_PROJECT_NAME}/${INSTALL_BUNDLE_NAME} COMPONENT ${BUNDLE})
    endif()
    if (INSTALL_RESOURCES)
        install (FILES ${INSTALL_RESOURCES} DESTINATION share/${INSTALL_PROJECT_NAME}/${INSTALL_BUNDLE_NAME} COMPONENT ${BUNDLE})
    endif()

endfunction()


#[[
Generate and install a Celix Targets cmake file which contains CMake commands to create imported targets for the bundles
install using the provided <export_name>. These imported CMake targets can be used in in CMake project using the installed
bundles.

Optional Arguments:
- FILE: The Celix Targets cmake filename to used, without the cmake extension. Default is <export_name>BundleTargets
- PROJECT_NAME: The project name to used for the share location. Default is the cmake project name.
- DESTINATION: The (relative) location to install the Celix Targets cmake file to. Default is share/<PROJECT_NAME>/cmake.

install_celix_targets(<export_name>
    NAMESPACE <namespace>
    [FILE <celix_target_filename>]
    [PROJECT_NAME <project_name>]
    [DESTINATION <celix_targets_destination>]
)

Example:
install_celix_targets(celix NAMESPACE Celix:: DESTINATION share/celix/cmake FILE CelixTargets)
]]

function(install_celix_bundle_targets)
    #0 is the export name
    list(GET ARGN 0 EXPORT_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS NAMESPACE DESTINATION FILE COMPONENT PROJECT_NAME)
    set(MULTI_VAL_ARGS )
    cmake_parse_arguments(EXPORT "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    get_target_property(EXPORT_BUNDLES celix-bundles EXPORT_${EXPORT_NAME}_BUNDLES)

    if (NOT DEFINED EXPORT_BUNDLES)
        message(FATAL_ERROR "Export ${EXPORT_NAME} not defined. Did you forgot to use a install_celix_bundle with the 'EXPORT ${EXPORT_NAME}' option?")
    endif ()
    if (NOT DEFINED EXPORT_NAMESPACE)
        message(FATAL_ERROR "Please provide a namespace used for the generated cmake targets.")
    endif ()

    if (CMAKE_BUILD_TYPE)
        string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
    else ()
        set(BUILD_TYPE "NOCONFIG")
    endif ()

    if (DEFINED EXPORT_FILE)
        #if present replace the .cmake from the export file
        string(REGEX REPLACE ".cmake" "" EXPORT_FILE "${EXPORT_FILE}")

        set(BASE_EXPORT_FILE ${EXPORT_FILE})
    else ()
        set(BASE_EXPORT_FILE ${EXPORT_NAME}BundleTargets)
    endif ()
    set(EXPORT_FILE ${BASE_EXPORT_FILE}-${BUILD_TYPE})

    if (NOT DEFINED EXPORT_PROJECT_NAME)
        string(TOLOWER ${PROJECT_NAME} EXPORT_PROJECT_NAME)
    endif()
    if (NOT DEFINED EXPORT_DESTINATION)
        set(EXPORT_DESTINATION share/${EXPORT_PROJECT_NAME}/cmake)
    endif ()
    if (EXPORT_COMPONENT)
        set(CMP_OPT "COMPONENT ${EXPORT_COMPONENT}")
    endif ()

    #extract number of .. needed ot reach install prefix (e.g. howto calculte _IMPORT_PREFIX
    file(TO_CMAKE_PATH ${EXPORT_DESTINATION} DEST_PATH)
    string(REGEX MATCHALL "/" SLASH_MATCHES ${DEST_PATH})
    list(LENGTH SLASH_MATCHES NR_OF_SUB_DIRS)

    set(CONF_IN_FILE "${CMAKE_BINARY_DIR}/celix/gen/cmake/${EXPORT_NAME}-ImportedBundleTargets.cmake.in")
    set(CONF_FILE "${CMAKE_BINARY_DIR}/celix/gen/cmake/${EXPORT_NAME}-ImportedBundleTargets.cmake")
    file(REMOVE "${CONF_IN_FILE}")


    file(APPEND "${CONF_IN_FILE}" "# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)
")
    foreach(_VAR RANGE ${NR_OF_SUB_DIRS})
        file(APPEND "${CONF_IN_FILE}" "get_filename_component(_IMPORT_PREFIX \"\${_IMPORT_PREFIX}\" PATH)
")
    endforeach()
        file(APPEND "${CONF_IN_FILE}" "
")

    foreach(BUNDLE_TARGET IN LISTS EXPORT_BUNDLES)
        set(TN "${EXPORT_NAMESPACE}${BUNDLE_TARGET}")
        file(APPEND "${CONF_IN_FILE}" "
if (NOT TARGET ${TN}) 
    add_library(${TN} SHARED IMPORTED)
endif ()
set_property(TARGET ${TN} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${BUILD_TYPE})
set_target_properties(${TN} PROPERTIES
    BUNDLE_IMPORTED TRUE
    BUNDLE_FILE_${BUILD_TYPE} \"\${_IMPORT_PREFIX}/share/${EXPORT_PROJECT_NAME}/bundles/$<TARGET_PROPERTY:${BUNDLE_TARGET},BUNDLE_FILENAME>\"
    BUNDLE_FILENAME_${BUILD_TYPE} \"$<TARGET_PROPERTY:${BUNDLE_TARGET},BUNDLE_FILENAME>\"
)
")
    endforeach()

    file(GENERATE OUTPUT "${CONF_FILE}" INPUT "${CONF_IN_FILE}")


    #Generate not build type specific targets file
    set(GENERIC_CONF_FILE "${CMAKE_BINARY_DIR}/celix/gen/cmake/${EXPORT_NAME}-BundleTargets.cmake")
    file(GENERATE OUTPUT ${GENERIC_CONF_FILE} CONTENT "
# Load bundle information for each installed configuration.
get_filename_component(_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)
file(GLOB CONFIG_FILES \"\${_DIR}/${BASE_EXPORT_FILE}-*.cmake\")
foreach(f \${CONFIG_FILES})
  include(\${f})
endforeach()
")

    if (EXPORT_COMPONENT)
        install(FILES "${CONF_FILE}" DESTINATION ${EXPORT_DESTINATION} RENAME ${EXPORT_FILE}.cmake COMPONENT ${EXPORT_COMPONENT})
        install(FILES "${GENERIC_CONF_FILE}" DESTINATION ${EXPORT_DESTINATION} RENAME ${BASE_EXPORT_FILE}.cmake COMPONENT ${EXPORT_COMPONENT})
    else ()
        install(FILES "${CONF_FILE}" DESTINATION ${EXPORT_DESTINATION} RENAME ${EXPORT_FILE}.cmake)
        install(FILES "${GENERIC_CONF_FILE}" DESTINATION ${EXPORT_DESTINATION} RENAME ${BASE_EXPORT_FILE}.cmake)
    endif ()
endfunction()

#[[
Get bundle file (absolute path to bundle) from an (imported) bundle
target taking into account the used CMAKE_BUILD_TYPE and available
bundle configurations.

celix_get_bundle_file(<bundle_target> VARIABLE_NAME)

Example: celix_get_bundle_file(Celix::shell SHELL_BUNDLE_FILE)

]]
function(celix_get_bundle_file)

if (TARGET ${ARGV0})
    get_target_property(_IMP ${ARGV0} BUNDLE_IMPORTED)
    if (_IMP)
        _celix_extract_imported_bundle_info(${ARGV0})
        set(${ARGV1} ${BUNDLE_FILE} PARENT_SCOPE)
        unset(BUNDLE_FILE)
        unset(BUNDLE_FILENAME)
    else ()
        get_target_property(BF ${ARGV0} BUNDLE_FILE)
        set(${ARGV1} ${BF} PARENT_SCOPE)
    endif ()
else ()
    message(FATAL_ERROR "Provided argument is not a CMake target: ${ARGV0}")
endif ()

endfunction ()

#[[
Get bundle filename from an (imported) bundle target taking into account the
used CMAKE_BUILD_TYPE and available bundle configurations.

celix_get_bundle_filename(<bundle_target> VARIABLE_NAME)

Example: celix_get_bundle_filename(Celix::shell SHELL_BUNDLE_FILENAME)

]]
function(celix_get_bundle_filename)

if (TARGET ${ARGV0})
    get_target_property(_IMP ${ARGV0} BUNDLE_IMPORTED)
    if (_IMP)
        _celix_extract_imported_bundle_info(${ARGV0})
        set(${ARGV1} ${BUNDLE_FILENAME} PARENT_SCOPE)
    else ()
        get_target_property(BF ${ARGV0} BUNDLE_FILENAME)
        set(${ARGV1} ${BF} PARENT_SCOPE)
    endif ()
else ()
    message(FATAL_ERROR "Provided argument is not a CMake target: ${ARGV0}")
endif ()

endfunction ()




######################################### "Private" function ###########################################################


#[[
extract the BUNDLE_FILENAME and BUNDLE_FILE from a imported bundle target taking into account the used CMAKE_BUILD_TYPE
and if configured the MAP_IMPORTED_CONFIG_* or CMAKE_MAP_IMPORTED_CONFIG_*

_celix_extract_imported_bundle_info(<bundle_target>)

Note this is considered a private function
]]
function (_celix_extract_imported_bundle_info)
    set(BUNDLE ${ARGV0})
    #get_target_property(_CONFIGS ${ARGV0} "IMPORTED_CONFIGURATIONS") #Not needed?

    if (CMAKE_BUILD_TYPE)
        string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
    else ()
        set(BUILD_TYPE "NOCONFIG")
    endif ()

    get_target_property(BF ${BUNDLE} BUNDLE_FILE_${BUILD_TYPE})
    get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_${BUILD_TYPE})

    if (NOT BF)
        #BUNDLE_FILE(NAME) not found for the current BUILD_TYPE looking for MAP value (if there is a cmake build type)
        get_target_property(MAP_TO_CONFIG ${BUNDLE} MAP_IMPORTED_CONFIG_${BUILD_TYPE})
        if (NOT MAP_TO_CONFIG AND CMAKE_MAP_IMPORTED_CONFIG_${BUILD_TYPE})
            set(MAP_TO_CONFIG CMAKE_MAP_IMPORTED_CONFIG_${BUILD_TYPE})
        endif ()
        if (MAP_TO_CONFIG)
            get_target_property(BF ${BUNDLE} BUNDLE_FILE_${MAP_TO_CONFIG})
            get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_${MAP_TO_CONFIG})
        endif ()
    endif ()

    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME)
    endif ()

    #fallback steps
    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE_RELWITHDEBINFO)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_RELWITHDEBINFO)
    endif ()
    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE_RELEASE)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_RELEASE)
    endif ()
    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE_MINSIZEREL)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_MINSIZEREL)
    endif ()
    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE_NOCONFIG)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_NOCONFIG)
    endif ()
    if (NOT BF)
        get_target_property(BF ${BUNDLE} BUNDLE_FILE_DEBUG)
        get_target_property(BFN ${BUNDLE} BUNDLE_FILENAME_DEBUG)
    endif ()


    set(BUNDLE_FILE ${BF} PARENT_SCOPE)
    set(BUNDLE_FILENAME ${BFN} PARENT_SCOPE)
endfunction ()
