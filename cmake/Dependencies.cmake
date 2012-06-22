# dep_opt(name "descr" default DEPS a b c
#option(BUILD_DEVICE_ACCESS "Option to enable building the Device Access Service bundles" OFF)
#if(BUILD_DEVICE_ACCESS)
#    get_property(log_doc CACHE BUILD_LOG_SERVICE PROPERTY HELPSTRING)
#    set(BUILD_LOG_SERVICE "ON" CACHE BOOL "c" FORCE)

MACRO(dep_opt)
    PARSE_ARGUMENTS(OPTION "DEPS" "" ${ARGN})
    LIST(GET OPTION_DEFAULT_ARGS 0 OPTION_NAME)
    LIST(GET OPTION_DEFAULT_ARGS 1 OPTION_DESCRIPTION)
    LIST(GET OPTION_DEFAULT_ARGS 2 OPTION_DEFAULT)
    
    string(TOUPPER ${OPTION_NAME} UC_OPTION_NAME)
    set(NAME "BUILD_${UC_OPTION_NAME}")
    
    option(${NAME} "${OPTION_DESCRIPTION}" ${OPTION_DEFAULT})
    
    IF (${NAME})
        FOREACH (DEP ${OPTION_DEPS})
            string(TOUPPER ${DEP} UC_DEP)
            set(DEP_NAME "BUILD_${UC_DEP}")
            get_property(doc_string CACHE ${DEP_NAME} PROPERTY HELPSTRING)
            set(${DEP_NAME} "ON" CACHE BOOL "${doc_string}" FORCE)
        ENDFOREACH (DEP)
    ENDIF (${NAME})
ENDMACRO(dep_opt)