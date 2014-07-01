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

MACRO(celix_subproject)
    PARSE_ARGUMENTS(OPTION "DEPS" "" ${ARGN})
    LIST(GET OPTION_DEFAULT_ARGS 0 OPTION_NAME)
    LIST(GET OPTION_DEFAULT_ARGS 1 OPTION_DESCRIPTION)
    LIST(GET OPTION_DEFAULT_ARGS 2 OPTION_DEFAULT)
    
    string(TOUPPER ${OPTION_NAME} UC_OPTION_NAME)
    set(NAME "BUILD_${UC_OPTION_NAME}")
    
    option(${NAME} "${OPTION_DESCRIPTION}" ${OPTION_DEFAULT})
    
    get_property(BUILD GLOBAL PROPERTY ${NAME}_INTERNAL)
    if (NOT DEFINED BUILD)
        set(BUILD "OFF")
    endif (NOT DEFINED BUILD)
    
    IF (${NAME} OR ${BUILD})
        set(${OPTION_NAME} "ON")
    	set_property(GLOBAL PROPERTY ${NAME}_INTERNAL "ON")
    	
        FOREACH (DEP ${OPTION_DEPS})
            string(TOUPPER ${DEP} UC_DEP)
            set(DEP_NAME "BUILD_${UC_DEP}")
            set_property(GLOBAL PROPERTY ${DEP_NAME}_INTERNAL "ON")
        ENDFOREACH (DEP)
    ELSE (${NAME} OR ${BUILD})
        set(${OPTION_NAME} "OFF")
    ENDIF (${NAME} OR ${BUILD})
ENDMACRO(celix_subproject)

MACRO(is_enabled name)
	string(TOUPPER ${name} UC_NAME)
    set(NAME "BUILD_${UC_NAME}")
    
    get_property(BUILD GLOBAL PROPERTY ${NAME}_INTERNAL)
    if (NOT DEFINED BUILD)
        set(BUILD "OFF")
    endif (NOT DEFINED BUILD)
    
    IF (${BUILD})
        set(${name} "ON")
    ELSE (${BUILD})
        set(${name} "OFF")
    ENDIF (${BUILD})
ENDMACRO(is_enabled)