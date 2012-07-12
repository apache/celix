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

find_program(FOUND_JAR jar)
if(NOT FOUND_JAR)
	message(FATAL_ERROR "Need jar command!") 
endif()

SET(BUNDLES_INSTALL_DIR "share/celix/bundles" CACHE PATH "Directory to install bundles. Relative to CMAKE_INSTALL_PREFIX")

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bundles)

function(bundle)
    PARSE_ARGUMENTS(BUNDLE "SOURCES" "" ${ARGN})
    LIST(GET BUNDLE_DEFAULT_ARGS 0 BUNDLE_NAME)
  
	add_library(${BUNDLE_NAME} SHARED ${BUNDLE_SOURCES})
	file(WRITE ${CMAKE_BINARY_DIR}/CMakeFiles/bundle_${BUNDLE_NAME}_add.cmake "")
    
	set(LIB_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${BUNDLE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
	
	add_custom_command(TARGET ${BUNDLE_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory tojar
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/CMakeFiles/bundle_${BUNDLE_NAME}_add.cmake
		COMMAND ${CMAKE_COMMAND} -E copy ${LIB_NAME} tojar/ 
		COMMAND jar -cfm ${PROJECT_BINARY_DIR}/bundles/${BUNDLE_NAME}.zip ${CMAKE_CURRENT_SOURCE_DIR}/META-INF/MANIFEST.MF -C ${CMAKE_CURRENT_BINARY_DIR}/tojar .  
		COMMAND ${CMAKE_COMMAND} -E remove_directory tojar
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endfunction(bundle)

function(bundle_add BUNDLE_NAME)
	list (LENGTH ARGN ARGN_LENGTH)
	list(FIND ARGN "DESTINATION" DEST_INDEX)
	math(EXPR COPY_RANGE "${DEST_INDEX} -1")
	math(EXPR DEST_VALUE_INDEX "${DEST_INDEX} +1")
	math(EXPR REST_START_INDEX "${DEST_INDEX} +2")
	math(EXPR MAX_RANGE "${ARGN_LENGTH} -1")
	
	list(APPEND ARGUMENTS COPY)
	
	#replace relative files/directory paths with absolute paths 
	foreach(INDEX RANGE 0 ${COPY_RANGE}) 
		list(GET ARGN ${INDEX} PATH)
		if (IS_ABSOLUTE ${PATH})
			list(APPEND ARGUMENTS ${PATH})	
		else ()
			list(APPEND ARGUMENTS ${CMAKE_CURRENT_SOURCE_DIR}/${PATH})
		endif ()
	endforeach()
	
	#add tojar to the relative DESTINATION path 
	list(GET ARGN ${DEST_VALUE_INDEX} DEST_PATH)
	if (IS_ABSOLUTE ${DEST_PATH})
		message(FATAL_ERROR "Absolute path as DESTINATION is not supported!")
	endif ()
	list(APPEND ARGUMENTS DESTINATION)
	list(APPEND ARGUMENTS tojar/${DEST_PATH})
	
	if (${REST_START_INDEX} LESS ${MAX_RANGE} OR ${REST_START_INDEX} EQUAL ${MAX_RANGE})
		foreach(INDEX RANGE ${REST_START_INDEX} ${MAX_RANGE})
			list(GET ARGN ${INDEX} VALUE)
			list(APPEND ARGUMENTS ${VALUE})
		endforeach()
	endif () 
	
	file(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/bundle_${BUNDLE_NAME}_add.cmake "
		file(${ARGUMENTS})
	")
endfunction()

function(bundle_install BUNDLE_NAME)
	list(FIND ARGN COMPONENT COMP_INDEX)
	if (${COMP_INDEX} GREATER -1)
		math(EXPR COMP_VALUE_INDEX "${COMP_INDEX} +1")
		list(GET ARGN ${COMP_VALUE_INDEX} BUNDLE_COMPONENT)
	else ()
		set(BUNDLE_COMPONENT ${BUNDLE_NAME})
	endif ()
	
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_BINARY_DIR}/bundles/${BUNDLE_NAME}.zip)
	install(FILES ${PROJECT_BINARY_DIR}/bundles/${BUNDLE_NAME}.zip DESTINATION ${BUNDLES_INSTALL_DIR} COMPONENT ${BUNDLE_COMPONENT})
endfunction()

function(package)
	#dummy function
endfunction(package)
	
ADD_CUSTOM_TARGET(deploy)
MACRO(deploy)
    PARSE_ARGUMENTS(DEPLOY "BUNDLES" "" ${ARGN})
    LIST(GET DEPLOY_DEFAULT_ARGS 0 DEPLOY_NAME)
	
	SET(DEPLOY_COMPONENT deploy_${DEPLOY_NAME})
	SET(__deployTarget deploy_${DEPLOY_NAME})
	#SET(__deployConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${DEPLOY_NAME}-deploy.cmake)
		
	#SET(DEPLOY_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME})
	#CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfigDeploy.in ${__deployConfig} @ONLY)
	#install(FILES ${EXT_DIR}/celix/* DESTINATION . COMPONENT ${DEPLOY_COMPONENT})
	
	SET(BUNDLES "")
	SET(DEPS)
	FOREACH(BUNDLE ${DEPLOY_BUNDLES})
		SET(DEP_NAME ${DEPLOY_NAME}_${BUNDLE}) 
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip
      		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/bundles/${BUNDLE}.zip 
      			${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip
  			DEPENDS ${BUNDLE}
  			COMMENT "Deploying ${BUNDLE} to ${DEPLOY_NAME}"
      	)
	    SET(BUNDLES "${BUNDLES} bundles/${BUNDLE}.zip")
	    SET(DEPS ${DEPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip)
	ENDFOREACH(BUNDLE)
    ADD_CUSTOM_TARGET(${__deployTarget} 
    	#mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/deploy \;
		#	cd ${CMAKE_CURRENT_BINARY_DIR}/deploy \; 
		#	${CPACK_COMMAND} --config "${__deployConfig}"
    	DEPENDS ${DEPS} celix
    	COMMENT "Deploy target ${DEPLOY_NAME}")
    ADD_DEPENDENCIES(deploy ${__deployTarget})
	
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/config.properties.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/config.properties @ONLY)
	
	set(FW_PATH ${CMAKE_BINARY_DIR}/framework)
	set(UTILS_PATH ${CMAKE_BINARY_DIR}/utils)
	set(LAUNCHER ${CMAKE_BINARY_DIR}/launcher/celix)
	
	IF(UNIX)
	  IF(APPLE)
	    set(LIBRARY_PATH DYLD_LIBRARY_PATH)
	  ELSE(APPLE)
	    set(LIBRARY_PATH LD_LIBRARY_PATH)
	  ENDIF(APPLE)
	ENDIF(UNIX)
	
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/run.sh.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/run.sh @ONLY)
	
	GET_DIRECTORY_PROPERTY(PROPS ADDITIONAL_MAKE_CLEAN_FILES)
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROPS};${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles")
	
	#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME} DESTINATION . COMPONENT ${DEPLOY_COMPONENT})
	#ADD_CUSTOM_TARGET(clean_${__packageTarget}
	#    rm ${PROJECT_BINARY_DIR}/packages/IBS-${PACKAGE_NAME}
    #)
ENDMACRO(deploy)

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


