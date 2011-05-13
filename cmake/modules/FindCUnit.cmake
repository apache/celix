#
# Find the CppUnit includes and library
#
# This module defines
# CPPUNIT_INCLUDE_DIR, where to find tiff.h, etc.
# CPPUNIT_LIBRARIES, the libraries to link against to use CppUnit.
# CPPUNIT_FOUND, If false, do not try to use CppUnit.

# also defined, but not for general use are
# CPPUNIT_LIBRARY, where to find the CppUnit library.
# CPPUNIT_DEBUG_LIBRARY, where to find the CppUnit library in debug mode.

include(FindCurses)

FIND_PATH(CUNIT_INCLUDE_DIR CUnit/Basic.h
  /usr/local/include
  /usr/include
  /opt/local/include  
)

# On unix system, debug and release have the same name
FIND_LIBRARY(CUNIT_LIBRARY cunit
             ${CUNIT_INCLUDE_DIR}/../lib
             /usr/local/lib
             /usr/lib
             )
FIND_LIBRARY(CUNIT_DEBUG_LIBRARY cunit
             ${CUNIT_INCLUDE_DIR}/../lib
             /usr/local/lib
             /usr/lib
             )

IF(CUNIT_INCLUDE_DIR)
  IF(CUNIT_LIBRARY)
    SET(CUNIT_FOUND "YES")
    SET(CUNIT_LIBRARIES ${CUNIT_LIBRARY} ${CURSES_LIBRARY})
    SET(CUNIT_DEBUG_LIBRARIES ${CUNIT_DEBUG_LIBRARY} ${CURSES_DEBUG_LIBRARY})
  ENDIF(CUNIT_LIBRARY)
  IF(CUNIT_INCLUDE_DIR)
    SET(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR} ${CURSES_INCLUDE_DIR})
  ENDIF(CUNIT_INCLUDE_DIR)
ENDIF(CUNIT_INCLUDE_DIR)
