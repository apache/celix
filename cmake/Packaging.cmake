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

GET_FILENAME_COMPONENT(__cmake_path ${CMAKE_COMMAND} PATH)
FIND_PROGRAM(CPACK_COMMAND cpack ${__cmake_path})
MESSAGE(STATUS "Found CPack at: ${CPACK_COMMAND}")
IF(NOT CPACK_COMMAND)
	MESSAGE(FATAL_ERROR "Need CPack!")
ENDIF(NOT CPACK_COMMAND)

include(CPackComponent)

macro( CELIX_ADD_COMPONENT_GROUP _group )
  set( _readVariable )
  set( _parentGroup )
  foreach( _i ${ARGN} )
    if( _readVariable )
      set( ${_readVariable} "${_i}" )
      break()
    else( _readVariable )
      if( "${_i}" STREQUAL PARENT_GROUP )
        set( _readVariable _parentGroup )
      endif( "${_i}" STREQUAL PARENT_GROUP )
    endif( _readVariable )
  endforeach( _i ${ARGN} )

  cpack_add_component_group( ${_group} ${ARGN} )
  add_custom_target( install-${_group} )
  if( _parentGroup )
    add_dependencies( install-${_parentGroup} install-${_group} )
  endif( _parentGroup )
endmacro( CELIX_ADD_COMPONENT_GROUP _group )

macro( CELIX_ADD_COMPONENT _component )
  set( _readVariable )
  set( _group )
  foreach( _i ${ARGN} )
    if( _readVariable )
      set( ${_readVariable} "${_i}" )
      break()
    else( _readVariable )
      if( "${_i}" STREQUAL GROUP )
        set( _readVariable _group )
      endif( "${_i}" STREQUAL GROUP )
    endif( _readVariable )
  endforeach( _i ${ARGN} )

  cpack_add_component( ${_component} ${ARGN} )
  add_custom_target( install-${_component}
                     COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${_component} -P
"${CMAKE_BINARY_DIR}/cmake_install.cmake" )
  add_dependencies( install-${_group} install-${_component} )
endmacro( CELIX_ADD_COMPONENT _component _group )

find_program(JAR_COMMAND jar)
if(JAR_COMMAND)
	message(STATUS "Using JAR to repack bundles, bundles can be used by Apache ACE")
else(JAR_COMMAND)
    message("No JAR support, generated bundles are not usable for Apache ACE")
endif(JAR_COMMAND)

SET(CPACK_GENERATOR "ZIP")

celix_add_component_group(all)
celix_add_component_group(bundles PARENT_GROUP all)

MACRO(SET_HEADER header content)
	SET(INT_${header} "${content}")
	string(STRIP ${INT_${header}} INT_${header})
ENDMACRO(SET_HEADER)

MACRO(SET_HEADERS content)
	SET(INT_BUNDLE_EXTRAHEADER "${INT_BUNDLE_EXTRAHEADER}\n${content}")
	string(STRIP ${INT_BUNDLE_EXTRAHEADER} INT_BUNDLE_EXTRAHEADER)
ENDMACRO(SET_HEADERS)

MACRO(bundle)
    PARSE_ARGUMENTS(BUNDLE "SOURCES;FILES;DIRECTORIES;INSTALL_FILES" "INSTALL" ${ARGN})
    LIST(GET BUNDLE_DEFAULT_ARGS 0 INT_BUNDLE_NAME)
    
	add_library(${INT_BUNDLE_NAME} SHARED ${BUNDLE_SOURCES})
	set_property(TARGET ${INT_BUNDLE_NAME} PROPERTY BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip") 
	
	if (NOT INT_BUNDLE_SYMBOLICNAME)
		set(INT_BUNDLE_SYMBOLICNAME ${INT_BUNDLE_NAME})
	endif (NOT INT_BUNDLE_SYMBOLICNAME)
	if (NOT INT_BUNDLE_VERSION)
		set(INT_BUNDLE_VERSION ${DEFAULT_VERSION})
	endif (NOT INT_BUNDLE_VERSION)
	if (NOT INT_BUNDLE_DESCRIPTION)
		set(INT_BUNDLE_DESCRIPTION "${INT_BUNDLE_SYMBOLICNAME} bundle")
	endif (NOT INT_BUNDLE_DESCRIPTION)
	
	set(INT_BUNDLE_NAME_INSTALL ${INT_BUNDLE_NAME}_install)
	
	SET(__bundleManifest ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.MF)
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/manifest.in ${__bundleManifest} @ONLY)
	install (FILES ${__bundleManifest} DESTINATION ./META-INF COMPONENT ${INT_BUNDLE_NAME_INSTALL})
    
	install (TARGETS ${INT_BUNDLE_NAME} DESTINATION . COMPONENT ${INT_BUNDLE_NAME_INSTALL})
    if (BUNDLE_FILES)
	    install (FILES ${BUNDLE_FILES} DESTINATION . COMPONENT ${INT_BUNDLE_NAME_INSTALL})
    endif(BUNDLE_FILES)
    if (BUNDLE_DIRECTORIES)
	    install (DIRECTORY ${BUNDLE_DIRECTORIES} DESTINATION . COMPONENT ${INT_BUNDLE_NAME_INSTALL})
    endif(BUNDLE_DIRECTORIES)

	SET(__bundleConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${INT_BUNDLE_NAME}-bundle.cmake)
	SET(BUNDLE_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfig.in ${__bundleConfig} @ONLY)

	if(JAR_COMMAND)
		ADD_CUSTOM_COMMAND(TARGET ${INT_BUNDLE_NAME}
		POST_BUILD
			COMMAND ${CPACK_COMMAND} ARGS -C Debug --config ${__bundleConfig}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/ziptojar
			COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/ziptojar ${JAR_COMMAND} -xf ${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip
			COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/ziptojar ${JAR_COMMAND} -cfm ${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip META-INF/MANIFEST.MF .
			COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/ziptojar
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
	else(JAR_COMMAND)
		ADD_CUSTOM_COMMAND(TARGET ${INT_BUNDLE_NAME}
		POST_BUILD
			COMMAND ${CPACK_COMMAND} ARGS -C Debug --config ${__bundleConfig}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
	endif(JAR_COMMAND)
	
	if (BUNDLE_INSTALL)
		CELIX_ADD_COMPONENT(${INT_BUNDLE_NAME}
	    	DISPLAY_NAME ${INT_BUNDLE_SYMBOLICNAME}
	        DESCRIPTION ${INT_BUNDLE_DESCRIPTION}
	        GROUP bundles
	    )
	    add_dependencies( install-${INT_BUNDLE_NAME} ${INT_BUNDLE_NAME} )
	    
	    if (BUNDLE_INSTALL_FILES)
		    install (FILES ${BUNDLE_INSTALL_FILES} DESTINATION include/celix/${INT_BUNDLE_NAME} COMPONENT ${INT_BUNDLE_NAME})
	    endif(BUNDLE_INSTALL_FILES)
	    
	    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip DESTINATION share/celix/bundles COMPONENT ${INT_BUNDLE_NAME})
    endif(BUNDLE_INSTALL)
	
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip)
ENDMACRO(bundle)

MACRO(install_bundle)
    PARSE_ARGUMENTS(BUNDLE "HEADERS;RESOURCES" "" ${ARGN})
    LIST(GET BUNDLE_DEFAULT_ARGS 0 INT_BUNDLE_NAME)
    
	CELIX_ADD_COMPONENT(${INT_BUNDLE_NAME}
    	DISPLAY_NAME ${INT_BUNDLE_NAME}
        DESCRIPTION ${INT_BUNDLE_NAME}
        GROUP bundles
    )
    add_dependencies( install-${INT_BUNDLE_NAME} ${INT_BUNDLE_NAME} )
    
    if (BUNDLE_HEADERS)
	    install (FILES ${BUNDLE_HEADERS} DESTINATION include/celix/${INT_BUNDLE_NAME} COMPONENT ${INT_BUNDLE_NAME})
    endif(BUNDLE_HEADERS)
    if (BUNDLE_RESOURCES)
	    install (FILES ${BUNDLE_RESOURCES} DESTINATION share/celix/${INT_BUNDLE_NAME} COMPONENT ${INT_BUNDLE_NAME})
    endif(BUNDLE_RESOURCES)
    
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${INT_BUNDLE_NAME}.zip DESTINATION share/celix/bundles COMPONENT ${INT_BUNDLE_NAME})
ENDMACRO(install_bundle)
	
MACRO(package)
    PARSE_ARGUMENTS(PACKAGE "FILES;DIRECTORIES" "" ${ARGN})
    LIST(GET PACKAGE_DEFAULT_ARGS 0 PACKAGE_NAME)
	
	SET(PACKAGE_COMPONENT package_${PACKAGE_NAME})
	
	get_property(bundle_file TARGET ${INT_BUNDLE_NAME} PROPERTY BUNDLE)
	
	install (FILES ${bundle_file} DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
	
	if (PACKAGE_FILES)
	    install (FILES ${PACKAGE_FILES} DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
    endif(PACKAGE_FILES)
    if (PACKAGE_DIRECTORIES)
	    install (DIRECTORY ${PACKAGE_DIRECTORIES} DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
    endif(PACKAGE_DIRECTORIES)

	SET(__packageConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${PACKAGE_NAME}-pkg.cmake)
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfigPKG.in ${__packageConfig} @ONLY)
	ADD_CUSTOM_COMMAND(TARGET ${PACKAGE_NAME}
		POST_BUILD
		COMMAND	${CPACK_COMMAND} --config "${__packageConfig}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
	
	GET_DIRECTORY_PROPERTY(PROPS ADDITIONAL_MAKE_CLEAN_FILES)
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROPS};${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-dev.zip")
ENDMACRO(package)


ADD_CUSTOM_TARGET(deploy)
MACRO(deploy)
    PARSE_ARGUMENTS(DEPLOY "BUNDLES;DRIVERS;ENDPOINTS" "" ${ARGN})
    LIST(GET DEPLOY_DEFAULT_ARGS 0 DEPLOY_NAME)
    
	SET(DEPLOY_COMPONENT deploy_${DEPLOY_NAME})
	SET(__deployTarget deploy_${DEPLOY_NAME})
		
	SET(BUNDLES "")
	SET(DEPS)
	
	FOREACH(BUNDLE ${DEPLOY_BUNDLES})
		SET(DEP_NAME ${DEPLOY_NAME}_${BUNDLE}) 
		get_property(bundle_file TARGET ${BUNDLE} PROPERTY BUNDLE)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip
      		COMMAND ${CMAKE_COMMAND} -E copy ${bundle_file} 
      			${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip
  			DEPENDS ${BUNDLE}
  			COMMENT "Deploying ${BUNDLE} to ${DEPLOY_NAME}"
      	)
	    SET(BUNDLES "${BUNDLES} bundles/${BUNDLE}.zip")
	    SET(DEPS ${DEPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip)
	ENDFOREACH(BUNDLE)
	FOREACH(BUNDLE ${DEPLOY_DRIVERS})
        SET(DEP_NAME ${DEPLOY_NAME}_${BUNDLE}) 
        get_property(bundle_file TARGET ${BUNDLE} PROPERTY BUNDLE)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/drivers/${BUNDLE}.zip
            COMMAND ${CMAKE_COMMAND} -E copy ${bundle_file} 
                ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/drivers/${BUNDLE}.zip
            DEPENDS ${BUNDLE}
            COMMENT "Deploying ${BUNDLE} to ${DEPLOY_NAME}"
        )
        #SET(BUNDLES "${BUNDLES} drivers/${BUNDLE}.zip")
        SET(DEPS ${DEPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/drivers/${BUNDLE}.zip)
    ENDFOREACH(BUNDLE)
    FOREACH(BUNDLE ${DEPLOY_ENDPOINTS})
        SET(DEP_NAME ${DEPLOY_NAME}_${BUNDLE}) 
        get_property(bundle_file TARGET ${BUNDLE} PROPERTY BUNDLE)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/endpoints/${BUNDLE}.zip
            COMMAND ${CMAKE_COMMAND} -E copy ${bundle_file} 
                ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/endpoints/${BUNDLE}.zip
            DEPENDS ${BUNDLE}
            COMMENT "Deploying ${BUNDLE} to ${DEPLOY_NAME}"
        )
        #SET(BUNDLES "${BUNDLES} drivers/${BUNDLE}.zip")
        SET(DEPS ${DEPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/endpoints/${BUNDLE}.zip)
    ENDFOREACH(BUNDLE)
	
	IF(NOT(CELIX_FOUND)) #celix project
		set(DEPS ${DEPS};celix)
	ENDIF()
    ADD_CUSTOM_TARGET(${__deployTarget} 
    	DEPENDS ${DEPS}
    	COMMENT "Deploy target ${DEPLOY_NAME}")
    ADD_DEPENDENCIES(deploy ${__deployTarget})
    
    GET_DIRECTORY_PROPERTY(PROPS ADDITIONAL_MAKE_CLEAN_FILES)
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles")
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/drivers")
	
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/config.properties.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/config.properties @ONLY)
	
	IF(UNIX)
		if(CELIX_FOUND)
			GET_FILENAME_COMPONENT(FW_PATH ${CELIX_FRAMEWORK_LIBRARY} PATH)
			GET_FILENAME_COMPONENT(UTILS_PATH ${CELIX_UTILS_LIBRARY} PATH)
			set(LAUNCHER ${CELIX_LAUNCHER})
		ELSE(CELIX_FOUND)
			set(FW_PATH ${CMAKE_BINARY_DIR}/framework)
			set(UTILS_PATH ${CMAKE_BINARY_DIR}/utils)
			set(LAUNCHER ${CMAKE_BINARY_DIR}/launcher/celix)
		ENDIF(CELIX_FOUND)
		
		
		IF(UNIX)
		  IF(APPLE)
		    set(LIBRARY_PATH DYLD_LIBRARY_PATH)
		  ELSE(APPLE)
		    set(LIBRARY_PATH LD_LIBRARY_PATH)
		  ENDIF(APPLE)
		ENDIF(UNIX)
	
		CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/run.sh.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/run.sh @ONLY)

		# Generate an Eclipse launch file to be able to run the deployment from Eclipse	
		# Linux/unix is assumed since we do only support VS on windows
		string(REPLACE "/" ";" LIST ${PROJECT_BINARY_DIR})
		list(LENGTH LIST len)
		MATH(EXPR test "${len} - 1")
		LIST(GET LIST ${test} last)
	
		SET(CONTAINER_NAME ${DEPLOY_NAME})
		SET(PROGRAM_NAME ${LAUNCHER})
		SET(PROJECT_ATTR ${last})
		SET(WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/")
		
		CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/RunConfig.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/${DEPLOY_NAME}.launch @ONLY)
	ENDIF(UNIX)
	
	IF(WIN32)
		GET_FILENAME_COMPONENT(apr_path ${APR_LIBRARY} PATH)
		GET_FILENAME_COMPONENT(aprutil_path ${APRUTIL_LIBRARY} PATH)
		GET_FILENAME_COMPONENT(zlib_path ${ZLIB_LIBRARY} PATH)
		GET_FILENAME_COMPONENT(curl_path ${CURL_LIBRARY} PATH)
		
		IF(CELIX_FOUND)
			SET(celixutils_path "${CELIX_FRAMEWORK_LIBRARY}/utils/${CMAKE_BUILD_TYPE}")
			SET(celixframework_path "${CELIX_FRAMEWORK_LIBRARY}/framework/${CMAKE_BUILD_TYPE}")
		ELSE(CELIX_FOUND)
			SET(celixutils_path "${PROJECT_BINARY_DIR}/utils/${CMAKE_BUILD_TYPE}")
			SET(celixframework_path "${PROJECT_BINARY_DIR}/framework/${CMAKE_BUILD_TYPE}")
		ENDIF(CELIX_FOUND)
		
		SET(PATH "%PATH%;${apr_path};${aprutil_path};${zlib_path};${curl_path};${celixutils_path};${celixframework_path}")
		
		CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/deploy_${DEPLOY_NAME}.vcxproj.user @ONLY)
	ENDIF(WIN32)
ENDMACRO(deploy)

# macro for scanning subdirectories for deploy.cmake files
# these files contain the deployment of targets
MACRO(deploy_targets)
    FILE(GLOB_RECURSE new_list deploy.cmake)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        SET(dir_list ${dir_list} ${file_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    FOREACH(file_path ${dir_list})
		include(${file_path})
	ENDFOREACH()
ENDMACRO()

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
	     SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
	     SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

