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

find_program(JAR_COMMAND jar)

if(JAR_COMMAND)
    message(STATUS "Using jar to create bundles")
else()
    find_program(ZIP_COMMAND zip)
    if(ZIP_COMMAND)
        message(STATUS "Using zip to create bundles")
    else()
        get_filename_component(__cmake_path ${CMAKE_COMMAND} PATH)
        find_program(CPACK_COMMAND cpack ${__cmake_path})
        if(CPACK_COMMAND)
            message(STATUS "Using cpack to create bundles.")
            message(WARNING "Please note that using jar and/or zip is prefered. When packaging bundles with cpack you must use 'make install-all' to install the project instead of 'make install'")
        else()
            message(FATAL_ERROR "A jar,zip or cpack command is needed to jar,zip or pack bundles")
        endif()
    endif()
endif()


##### setup bundles/deploy target
add_custom_target(bundles ALL)
add_custom_target(deploy ALL)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/deploy")
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

    #TODO add support qualifier 
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
    list(GET ARGN 0 BUNDLE_TARGET_NAME)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS NO_ACTIVATOR)
    set(ONE_VAL_ARGS VERSION ACTIVATOR SYMBOLIC_NAME NAME DESCRIPTION) 
    set(MULTI_VAL_ARGS SOURCES PRIVATE_LIBRARIES EXPORT_LIBRARIES HEADERS)
    cmake_parse_arguments(BUNDLE "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    ##check arguments
    if(NOT BUNDLE_TARGET_NAME)
        message(FATAL_ERROR "add_bunde function requires first target name argument")
    endif()
    if((NOT (BUNDLE_SOURCES OR BUNDLE_ACTIVATOR)) AND (NOT BUNDLE_NO_ACTIVATOR))
        message(FATAL_ERROR "Bundle contain no SOURCES or ACTIVATOR target and the option NO_ACTIVATOR is not set")
    endif()
    if(BUNDLE_SOURCES AND BUNDLE_ACTIVATOR)
        message(FATAL_ERROR "add_bundle function requires a value for SOURCES or ACTIVATOR not both")
    endif()
    if(BUNDLE_ACTIVATOR)
        check_lib(${BUNDLE_ACTIVATOR})
    endif()

    #setting defaults
    if(NOT BUNDLE_VERSION) 
        set(BUNDLE_VERSION "0.0.0")
        message(WARNING "Bundle version for ${BUNDLE_NAME} not provided. Using 0.0.0")
    endif()
    if (NOT BUNDLE_NAME)
        set(BUNDLE_NAME ${BUNDLE_TARGET_NAME})
    endif()
    if (NOT BUNDLE_SYMBOLIC_NAME)
        set(BUNDLE_SYMBOLIC_NAME ${BUNDLE_TARGET_NAME})
    endif()
    set(BUNDLE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_TARGET_NAME}.zip") 
    set(BUNDLE_CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_TARGET_NAME}_content")
    set(BUNDLE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE_TARGET_NAME}_gen")


    ###### Setting up dependency for bundles target
    get_target_property(DEPS bundles "BUNDLES_DEPS")
    list(APPEND DEPS "${BUNDLE_FILE}")
    set_target_properties(bundles PROPERTIES "BUNDLES_DEPS" "${DEPS}")
    #####

    ####### Setting target for activator lib if neccesary ####################
    if(BUNDLE_SOURCES)
        #create lib from sources
        add_library(${BUNDLE_TARGET_NAME} SHARED ${BUNDLE_SOURCES})
        set_library_version(${BUNDLE_TARGET_NAME} ${BUNDLE_VERSION})
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_TARGET_IS_LIB" TRUE)
    else()
        add_custom_target(${BUNDLE_TARGET_NAME})
    endif()
    add_custom_target(${BUNDLE_TARGET_NAME}_bundle
        DEPENDS "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_FILE>"
    )
    add_dependencies(bundles ${BUNDLE_TARGET_NAME}_bundle)
    #######################################################################
   

    ##### MANIFEST configuration and generation ##################
    #Step1 configure the file so that the target name is present in in the template
    configure_file(${CELIX_CMAKE_DIRECTORY}/cmake_celix/Manifest.template.in ${BUNDLE_GEN_DIR}/MANIFEST.step1)

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

    ###### Packaging the bundle using using jar,zip or cpack and a content dir. Configuring dependencies ######
    if(JAR_COMMAND)
        add_custom_command(OUTPUT ${BUNDLE_FILE}
            COMMAND ${JAR_COMMAND} -cfm ${BUNDLE_FILE} ${BUNDLE_GEN_DIR}/MANIFEST.MF -C ${BUNDLE_CONTENT_DIR} .
            COMMENT "Packaging ${BUNDLE_TARGET_NAME}"
            DEPENDS  ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_DEPEND_TARGETS>" ${BUNDLE_GEN_DIR}/MANIFEST.MF
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    elseif(ZIP_COMMAND)
        add_custom_command(OUTPUT ${BUNDLE_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUNDLE_GEN_DIR}/MANIFEST.MF META-INF/MANIFEST.MF
            COMMAND ${ZIP_COMMAND} -rq ${BUNDLE_FILE} *
            COMMENT "Packaging ${BUNDLE_TARGET_NAME}"
            DEPENDS  ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_DEPEND_TARGETS>" ${BUNDLE_GEN_DIR}/MANIFEST.MF
            WORKING_DIRECTORY ${BUNDLE_CONTENT_DIR}
        )
    elseif(CPACK_COMMAND)
        install(DIRECTORY ${BUNDLE_CONTENT_DIR}/ DESTINATION . COMPONENT ${BUNDLE_TARGET_NAME}_install_cmp)
        install(FILES ${BUNDLE_GEN_DIR}/MANIFEST.MF DESTINATION META-INF COMPONENT ${BUNDLE_TARGET_NAME}_install_cmp)
        file(GENERATE OUTPUT ${BUNDLE_GEN_DIR}/cpack.cmake CONTENT "
SET(CPACK_CMAKE_GENERATOR \"Unix Makefiles\")
SET(CPACK_GENERATOR \"ZIP\")
SET(CPACK_INSTALL_CMAKE_PROJECTS \"${CMAKE_CURRENT_BINARY_DIR};${BUNDLE_TARGET_NAME};${BUNDLE_TARGET_NAME}_install_cmp;/\")
SET(CPACK_PACKAGE_DESCRIPTION \"\")
SET(CPACK_PACKAGE_FILE_NAME \"${BUNDLE_TARGET_NAME}\")
SET(CPACK_PACKAGE_NAME \"${BUNDLE_TARGET_NAME}\")
SET(CPACK_PACKAGE_VERSION \"0.0.1\")
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY \"0\")
")
        add_custom_command(OUTPUT ${BUNDLE_FILE}
            COMMAND ${CPACK_COMMAND} ARGS -C Debug --config ${BUNDLE_GEN_DIR}/cpack.cmake
            COMMENT "Packaging ${BUNDLE_TARGET_NAME}"
            DEPENDS  ${BUNDLE_TARGET_NAME} "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_DEPEND_TARGETS>" ${BUNDLE_GEN_DIR}/MANIFEST.MF
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
    ###################################################################################


    ###################################
    ##### Additional Cleanup info #####
    ###################################
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_GEN_DIR>;$<TARGET_PROPERTY:${BUNDLE_TARGET_NAME},BUNDLE_CONTENT_DIR>")

    #############################
    ### BUNDLE TARGET PROPERTIES
    #############################
    #internal use
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_DEPEND_TARGETS" "") #bundle target dependencies. Note can be extended after the add_bundle call
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_GEN_DIR" ${BUNDLE_GEN_DIR}) #location for generated output. 

    #bundle specific
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_CONTENT_DIR" ${BUNDLE_CONTENT_DIR}) #location where the content to be jar/zipped. 
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_FILE" ${BUNDLE_FILE}) #target bundle file (.zip)

    #name and version
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_NAME" ${BUNDLE_NAME}) #The bundle name default target name
    set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_SYMBOLIC_NAME" ${BUNDLE_SYMBOLIC_NAME}) #The bundle symbolic name. Default target name
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
        bundle_private_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_TARGET_NAME})
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" "$<TARGET_SONAME_FILE_NAME:${BUNDLE_TARGET_NAME}>")

        if(APPLE)
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES INSTALL_RPATH "@loader_path")
        else()
            set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
        endif()
    else() #ACTIVATOR 
        bundle_private_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_ACTIVATOR})
        get_filename_component(ACT_NAME ${BUNDLE_ACTIVATOR} NAME)
        set_target_properties(${BUNDLE_TARGET_NAME} PROPERTIES "BUNDLE_ACTIVATOR" ${ACT_NAME})
    endif()

    bundle_private_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_PRIVATE_LIBRARIES})
    bundle_export_libs(${BUNDLE_TARGET_NAME} ${BUNDLE_EXPORT_LIBRARIES})
    bundle_headers(${BUNDLE_TARGET_NAME} ${BUNDLE_HEADERS})
endfunction()

function(get_bundle_file BUNDLE OUT)
    check_bundle(${BUNDLE})
    get_target_property(${OUT} ${BUNDLE} "BUNDLE_FILE")
endfunction()

function(bundle_export_libs)
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)
    bundle_libs(${BUNDLE} "EXPORT" ${ARGN})
endfunction()

function(bundle_private_libs)
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)
    bundle_libs(${BUNDLE} "PRIVATE" ${ARGN})
endfunction()

function(bundle_libs)
    #0 is bundle TARGET
    #1 is type (PRIVATE,EXPORT
    #2..n is libs
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    list(GET ARGN 0 TYPE)
    list(REMOVE_AT ARGN 0)

    #check if arg 0 is corrent
    check_bundle(${BUNDLE})
    get_target_property(BUNDLE_DIR ${BUNDLE} "BUNDLE_CONTENT_DIR")
    get_target_property(BUNDLE_GEN_DIR ${BUNDLE} "BUNDLE_GEN_DIR")


    get_target_property(LIBS ${BUNDLE} "BUNDLE_${TYPE}_LIBS")
    get_target_property(DEPS ${BUNDLE} "BUNDLE_DEPEND_TARGETS")

    foreach(LIB IN ITEMS ${ARGN})
        if(IS_ABSOLUTE ${LIB} AND EXISTS ${LIB})
            get_filename_component(LIB_NAME ${LIB} NAME) 
            set(OUT "${BUNDLE_DIR}/${LIB_NAME}") 
            add_custom_command(OUTPUT ${OUT} 
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB} ${OUT} 
            )
            list(APPEND LIBS ${LIB_NAME})
            list(APPEND DEPS ${OUT}) 
        else()
            #Assuming target
            #NOTE add_custom_command does not support generator expression in OUTPUT value (e.g. $<TARGET_FILE:${LIB}>)
            #Using a two step approach to be able to use add_custom_command instead of add_custom_target
            set(OUT "${BUNDLE_GEN_DIR}/lib-${LIB}-copy-timestamp")
            add_custom_command(OUTPUT ${OUT}
                COMMAND ${CMAKE_COMMAND} -E touch ${OUT}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${LIB}>" "${BUNDLE_DIR}/$<TARGET_SONAME_FILE_NAME:${LIB}>"
                DEPENDS ${LIB}
            )
            list(APPEND DEPS "${OUT}") #NOTE depending on ${OUT} not on $<TARGET_FILE:${LIB}>.
            list(APPEND LIBS "$<TARGET_SONAME_FILE_NAME:${LIB}>")
        endif()

        get_target_property(IS_LIB ${BUNDLE} "BUNDLE_TARGET_IS_LIB")
        if ("${LIB}" STREQUAL "${BUNDLE}")
            #ignore. Do not have to link agaist own lib
        elseif(IS_LIB)
            target_link_libraries(${BUNDLE} ${LIB})
        endif()
    endforeach()


    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_${TYPE}_LIBS" "${LIBS}")
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DEPEND_TARGETS" "${DEPS}")
endfunction()

#TODO
#
# bundle_import_libs(<target> LIB <lib> RANGE <version_range> ?)
#
#
function(bundle_import_libs)
    #0 is bundle TARGET
    #1..n is libs
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    message(WARNING "IMPORT BUNDLES STILL TODO")

endfunction()

function(bundle_files)
    #0 is bundle TARGET
    #1..n is header name / header value
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

function(bundle_headers)
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

function(bundle_symbolic_name BUNDLE SYMBOLIC_NAME) 
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_SYMBOLIC_NAME" ${SYMBOLIC_NAME})
endfunction()

function(bundle_name BUNDLE NAME) 
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_NAME" ${NAME})
endfunction()

function(bundle_version BUNDLE VERSION) 
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_VERSION" ${VERSION})
endfunction()

function(bundle_description BUNDLE DESC) 
    set_target_properties(${BUNDLE} PROPERTIES "BUNDLE_DESCRIPTION" ${DESC})
endfunction()

function(install_bundle)
    #0 is bundle TARGET
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS PROJECT_NAME BUNDLE_NAME) 
    set(MULTI_VAL_ARGS HEADERS RESOURCES)
    cmake_parse_arguments(INSTALL "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    
    if (NOT INSTALL_PROJECT_NAME)
        string(TOLOWER ${PROJECT_NAME} INSTALL_PROJECT_NAME)
    endif()
    if (NOT INSTALL_BUNDLE_NAME)
        set(INSTALL_BUNDLE_NAME ${BUNDLE})
    endif()

    install(FILES "$<TARGET_PROPERTY:${BUNDLE},BUNDLE_FILE>" DESTINATION share/${INSTALL_PROJECT_NAME}/bundles COMPONENT ${BUNDLE})
    if(INSTALL_HEADERS)
        install (FILES ${INSTALL_HEADERS} DESTINATION include/${INSTALL_PROJECT_NAME}/${INSTALL_BUNDLE_NAME} COMPONENT ${BUNDLE})
    endif()
    if (INSTALL_RESOURCES)
        install (FILES ${INSTALL_RESOURCES} DESTINATION share/${INSTALL_PROJECT_NAME}/${INSTALL_BUNDLE_NAME} COMPONENT ${BUNDLE})
    endif()

endfunction()

function(add_deploy)
    list(GET ARGN 0 DEPLOY_TARGET)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS COPY)
    set(ONE_VAL_ARGS GROUP NAME)
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
    set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEPLOY_TARGET}-timestamp")

    add_custom_target(${DEPLOY_TARGET}
        DEPENDS ${TIMESTAMP_FILE}
    )
    add_dependencies(deploy ${DEPLOY_TARGET})

    add_custom_command(OUTPUT "${TIMESTAMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
        DEPENDS  "$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_TARGET_DEPS>" "${DEPLOY_LOCATION}/config.properties" "${DEPLOY_LOCATION}/run.sh" 
        WORKING_DIRECTORY "${DEPLOY_LOCATION}"
        COMMENT "Deploying ${DEPLOY_PRINT_NAME}" VERBATIM
    )


    file(GENERATE 
        OUTPUT "${DEPLOY_LOCATION}/config.properties.step1"
        CONTENT "cosgi.auto.start.1=$<JOIN:$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_BUNDLES>, >
$<JOIN:$<TARGET_PROPERTY:${DEPLOY_TARGET},DEPLOY_PROPERTIES>,
>
"
    )

    file(GENERATE
        OUTPUT "${DEPLOY_LOCATION}/config.properties"
        INPUT "${DEPLOY_LOCATION}/config.properties.step1"
    )

    set(CONTAINER_NAME ${DEPLOY_NAME})
    set(PROGRAM_NAME "${CMAKE_BIN_DIRECTORY}/launcher/celix")
    set(PROJECT_ATTR "build")
    set(WORKING_DIRECTORY ${DEPLOY_LOCATION})
    set(PATHS "${CMAKE_BIN_DIRECTORY}/framework:${CMAKE_BIN_DIRECTORY}/utils:${CMAKE_BIN_DIRECTORY}/dfi")
    configure_file("${CELIX_CMAKE_DIRECTORY}/cmake_celix/RunConfig.in"  "${DEPLOY_LOCATION}/${DEPLOY_NAME}")


    if(APPLE) 
        file(GENERATE
            OUTPUT ${DEPLOY_LOCATION}/run.sh
            CONTENT "export DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:celix_framework>:$<TARGET_FILE_DIR:celix_utils>:$<TARGET_FILE_DIR:celix_dfi>:\${DYLD_LIBRARY_PATH}
$<TARGET_FILE:celix> $@
"
    )
    else() 
        file(GENERATE
            OUTPUT ${DEPLOY_LOCATION}/run.sh
            CONTENT "export LD_LIBRARY_PATH=$<TARGET_FILE_DIR:celix_framework>:$<TARGET_FILE_DIR:celix_utils>:$<TARGET_FILE_DIR:celix_dfi>:\${LD_LIBRARY_PATH}
$<TARGET_FILE:celix> $@
"
    )
    endif()

    #TODO eclipse launcher file
    #####

    ##### Deploy Target Properties #####
    #internal use
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_TARGET_DEPS" "") #bundles to deploy.
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_BUNDLES" "") #bundles to deploy.
    set_target_properties(${DEPLOY_TARGET} PROPERTIES "DEPLOY_COPY_BUNDLES" ${DEPLOY_COPY}) #copy bundles in bundle dir or link using abs paths.

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
            set(OUT "${DEPLOY_LOC}/${BD_DIR_NAME}/${BUNDLE_FILENAME}.zip")
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
                    list(APPEND BUNDLES ${BUNDLE_FILENAME})
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

   if(DEPLOY_COPY) 
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
