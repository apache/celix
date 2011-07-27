
include(FindAPR)
IF(APR_FOUND)
	MESSAGE(STATUS "Looking for APR - found")
ELSE(APR_FOUND)
	MESSAGE(FATAL_ERROR "Looking for APR - not found")
ENDIF(APR_FOUND)

include_directories(${APR_INCLUDE_DIR})
include_directories("framework/private/include")
include_directories("framework/public/include")

include(cmake/Packaging.cmake)