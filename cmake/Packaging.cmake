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

#IF(UNIX)
#	SET(CPACK_GENERATOR "TGZ;STGZ")
#ENDIF(UNIX)
#IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#	SET(CPACK_GENERATOR "TBZ2;STGZ")
#ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#IF(WIN32)
#	SET(CPACK_GENERATOR "NSIS;ZIP")
#ENDIF(WIN32)

SET(CPACK_GENERATOR "ZIP")

ADD_CUSTOM_TARGET(bundles)
ADD_CUSTOM_TARGET(packages)

MACRO(bundle)
    PARSE_ARGUMENTS(BUNDLE "FILES;DIRECTORIES" "" ${ARGN})
    LIST(GET BUNDLE_DEFAULT_ARGS 0 BUNDLE_NAME)
	
	install (TARGETS ${BUNDLE_NAME} DESTINATION . COMPONENT ${BUNDLE_NAME})
    install (DIRECTORY MANIFEST DESTINATION . COMPONENT ${BUNDLE_NAME} FILES_MATCHING PATTERN "*" PATTERN ".svn" EXCLUDE)
    
    if (BUNDLE_FILES)
	    install (FILES ${BUNDLE_FILES} DESTINATION . COMPONENT ${BUNDLE_NAME})
    endif(BUNDLE_FILES)
    if (BUNDLE_DIRECTORIES)
	    install (DIRECTORY ${BUNDLE_DIRECTORIES} DESTINATION . COMPONENT ${BUNDLE_NAME})
    endif(BUNDLE_DIRECTORIES)
    
	SET(__bundleTarget bundle_${BUNDLE_NAME})
	SET(__bundleConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${BUNDLE_NAME}-bundle.cmake)

	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfig.in ${__bundleConfig} @ONLY)
	ADD_CUSTOM_TARGET(${__bundleTarget}
		mkdir -p ${PROJECT_BINARY_DIR}/bundles \;
		cd ${PROJECT_BINARY_DIR}/bundles \; 
		${CPACK_COMMAND} --config "${__bundleConfig}"
	)
	ADD_DEPENDENCIES(bundles ${__bundleTarget})
	ADD_DEPENDENCIES(${__bundleTarget} ${BUNDLE_NAME})
ENDMACRO(bundle)
	
MACRO(package)
    PARSE_ARGUMENTS(PACKAGE "FILES;DIRECTORIES" "" ${ARGN})
    LIST(GET PACKAGE_DEFAULT_ARGS 0 PACKAGE_NAME)
	
	SET(PACKAGE_COMPONENT package_${PACKAGE_NAME})
	SET(__packageTarget package_${PACKAGE_NAME})
	SET(__bundleTarget bundle_${PACKAGE_NAME})
	SET(__packageConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${PACKAGE_NAME}-pkg.cmake)
	
	install (FILES ${PROJECT_BINARY_DIR}/bundles/${PACKAGE_NAME}.zip DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
	
	if (PACKAGE_FILES)
	    install (FILES ${PACKAGE_FILES} DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
    endif(PACKAGE_FILES)
    if (PACKAGE_DIRECTORIES)
	    install (DIRECTORY ${PACKAGE_DIRECTORIES} DESTINATION . COMPONENT ${PACKAGE_COMPONENT})
    endif(PACKAGE_DIRECTORIES)

	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfigPKG.in ${__packageConfig} @ONLY)
	ADD_CUSTOM_TARGET(${__packageTarget}
		mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/packages \;
		cd ${CMAKE_CURRENT_BINARY_DIR}/packages \; 
		${CPACK_COMMAND} --config "${__packageConfig}"
	)
	ADD_DEPENDENCIES(${__packageTarget} ${__bundleTarget})
	ADD_DEPENDENCIES(packages ${__packageTarget})
	ADD_CUSTOM_TARGET(clean_${__packageTarget}
	    rm ${PROJECT_BINARY_DIR}/packages/IBS-${PACKAGE_NAME}
    )
ENDMACRO(package)


ADD_CUSTOM_TARGET(deploy)
MACRO(deploy)
    PARSE_ARGUMENTS(DEPLOY "BUNDLES" "" ${ARGN})
    LIST(GET DEPLOY_DEFAULT_ARGS 0 DEPLOY_NAME)
	
	SET(DEPLOY_COMPONENT deploy_${DEPLOY_NAME})
	SET(__deployTarget deploy_${DEPLOY_NAME})
	SET(__deployConfig ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig-${DEPLOY_NAME}-deploy.cmake)
		
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfigDeploy.in ${__deployConfig} @ONLY)
	ADD_CUSTOM_TARGET(${__deployTarget}
		mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/deploy \;
		cd ${CMAKE_CURRENT_BINARY_DIR}/deploy \; 
		${CPACK_COMMAND} --config "${__deployConfig}"
	)
	#install(FILES ${EXT_DIR}/celix/* DESTINATION . COMPONENT ${DEPLOY_COMPONENT})
	
	SET(BUNDLES "")
	foreach(BUNDLE ${DEPLOY_BUNDLES})
		add_custom_command(TARGET ${__deployTarget}
      		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/bundles/${BUNDLE}.zip 
      			${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/bundles/${BUNDLE}.zip
      	)
	    SET(BUNDLES "${BUNDLES} bundles/${BUNDLE}.zip")
	    ADD_DEPENDENCIES(${__deployTarget} bundle_${BUNDLE})
	endforeach(BUNDLE)
	
	CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/config.properties.in ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME}/config.properties @ONLY)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deploy/${DEPLOY_NAME} DESTINATION . COMPONENT ${DEPLOY_COMPONENT})
	ADD_DEPENDENCIES(deploy ${__deployTarget})
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


