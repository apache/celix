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

macro(SET_HEADER NAME VALUE)
    SET(BUNDLE_${NAME} ${VALUE})
endmacro()

macro(SET_HEADERS HEADERS)
    if(NOT HEADERS)
        set(BUNDLE_HEADERS ${HEADERS})
    else()
        list(APPEND BUNDLE_HEADERS ${HEADERS})
    endif()
endmacro()

function(bundles)
    list(GET ARGN 0 BUNDLE)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS INSTALL )
    set(ONE_VAL_ARGS DESTINATION)
    set(MULTI_VAL_ARGS SOURCES LINK_LIBRARIES EXPORT_VERSION ACTIVATOR PRIVATE_LIBRARIES EXPORT_LIBRARIES IMPORT_LIBRARIES FILES DIRECTORIES INSTALL_FILES)
    cmake_parse_arguments(BUNDLE "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN}) 
    #original PARSE_ARGUMENTS(_BUNDLE "SOURCES;LINK_LIBRARIES;EXPORT_VERSION;ACTIVATOR;PRIVATE_LIBRARIES;EXPORT_LIBRARIES;IMPORT_LIBRARIES;FILES;DIRECTORIES;INSTALL_FILES" "PRIVATE;EXPORT;INSTALL" ${ARGN})

    if(BUNDLE_SOURCES) 
        add_bundle(${BUNDLE} SOURCES ${BUNDLE_SOURCES} VERSION 0.0.0) 
    else()
        add_bundle(${BUNDLE} ACTIVATOR ${BUNDLE_ACTIVATOR} VERSION 0.0.0)
    endif()

    if(BUNDLE_FILES)
        bundle_files(${BUNDLE} ${BUNDLE_FILES} DESTINATION .)
    if(BUNDLE_EXPORT_VERSION) 
        message(WARNING "EXPORT_VERSION argument not supported")
    endif()
    if(BUNDLE_LINK_LIBRARIES) 
        target_link_libraries(${BUNDLE} ${BUNDLE_LINK_LIBRARIES})
    endif()
    if(BUNDLE_PRIVATE_LIBRARIES) 
        bundle_private_libs(${BUNDLE} ${BUNDLE_PRIVATE_LIBS})
    endif()
    if(BUNDLE_EXPORT_LIBRARIES)
        bundle_export_libs(${BUNDLE} ${BUNDLE_PRIVATE_LIBS})
    endif()
    if(BUNDLE_IMPORT_LIBRARIES)
        message(FATAL_ERROR "TODO IMPORT LIBRARIES")
    endif()
    if(BUNDLE_DIRECTORIES)
        message(WARNING "DIRECTORIES argument not supported")
    endif()
    if(BUNDLE_INSTALL_FILES)
        message(WARNING "INSTALL_FILES argument not supported")
    endif()
    if(BUNDLE_INSTALL)
        bundle_install(${BUNDLE})
    endif()

    if(BUNDLE_NAME)
        bundle_name(${BUNDLE} ${BUNDLE_NAME})
    endif()
    if(BUNDLE_SYMBOLICNAME)
        bundle_symbolic_name(${BUNDLE} ${BUNDLE_SYMBOLICNAME})
    endif()
    if(BUNDLE_VERSION)
        bundle_version(${BUNDLE} ${BUNDLE_VERSION})
    endif()
    if (BUNDLE_HEADERS)
        bundle_headers(${BUNDLE} ${BUNDLE_HEADERS})
    endif()

    message(STATUS "bundle function is deprecated. update to add_bundle")
endfunction()


function(deploy)
    list(GET ARGN 0 DEPLOY)
    list(REMOVE_AT ARGN 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS )
    set(MULTI_VAL_ARGS BUNDLES DRIVERS ENDPOINTS PROPERTIES)
    cmake_parse_arguments(DEPLOY "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN}) 
    #original    PARSE_ARGUMENTS(DEPLOY "BUNDLES;DRIVERS;ENDPOINTS;PROPERTIES" "" ${ARGN})                                                                                                                                      

    add_deploy(${DEPLOY} BUNDLES ${DEPLOY_BUNDLES} PROPERTIES ${DEPLOY_PROPERTIES})
    if(DEPLOY_DRIVERS)
        deploy_bundles_dir(${DEPLOY} DIR_NAME "drivers" BUNDLES ${DEPLOY_DRIVERS})
    endif()
    if(DEPLOY_ENDPOINTS)
        deploy_bundles_dir(${DEPLOY} DIR_NAME "endpoints" BUNDLES ${DEPLOY_ENDPOINTS})
    endif()
    message(STATUS "deploy function is deprecated. update to add_deploy")
endfunction()
