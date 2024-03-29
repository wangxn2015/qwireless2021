PROJECT(qwireless)   

#############################################################
# CMake settings

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.3)

IF(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0005 OLD)
ENDIF(COMMAND cmake_policy)

  SET (DEFAULT_BIN_SUBDIR     /usr/bin)
  SET (DEFAULT_LIB_SUBDIR     /usr/lib)
  SET (DEFAULT_DATA_SUBDIR    /usr/share/qgis)
  SET (DEFAULT_PLUGIN_SUBDIR  /usr/lib/qgis/plugins/)
  SET (DEFAULT_INCLUDE_SUBDIR /usr/include/qgis)

SET(CMAKE_COLOR_MAKEFILE ON)

# set path to additional CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
#SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# it's possible to set PLUGINS_ALSO_BINARIES to TRUE
# then some plugins that can run as standalone apps will be built
# also as standalone apps
SET (PLUGINS_ALSO_BINARIES FALSE)

# try to configure and build GRASS plugin by default
SET (WITH_GRASS FALSE CACHE BOOL "Determines whether GRASS plugin should be built")
SET (GRASS_PREFIX "" CACHE PATH "Path to GRASS base directory")

# try to configure and build python bindings by default
SET (WITH_BINDINGS TRUE CACHE BOOL "Determines whether python bindings should be built")

# By default bindings will be installed only to QGIS directory
# Someone might want to install it to python site-packages directory
# as otherwise user has to use PYTHONPATH environemnt variable to add
# QGIS bindings to package search path
SET (BINDINGS_GLOBAL_INSTALL FALSE CACHE BOOL "Install bindings to global python directory? (might need root)")

# Compile flag. Make it posible to turn it off.
SET (PEDANTIC FALSE CACHE BOOL "Determines if we should compile with -Wall -Werror.")

# whether unit tests should be build
SET (ENABLE_TESTS FALSE CACHE BOOL "Build unit tests?")

# hide this variable because building of python bindings might fail
# if set to other directory than expected
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH)


#############################################################
# check if lexer and parser are not missing
# http://www.mail-archive.com/cmake@cmake.org/msg02861.html

#INCLUDE(Flex)

#FIND_FLEX()

#IF (NOT FLEX_EXECUTABLE)
# MESSAGE(FATAL_ERROR "Couldn't find Flex")
#ENDIF (NOT FLEX_EXECUTABLE)

#INCLUDE(Bison)

#FIND_BISON()

#IF (NOT BISON_EXECUTABLE)
#  MESSAGE(FATAL_ERROR "Couldn't find Bison")
#ENDIF (NOT BISON_EXECUTABLE)

#############################################################
# search for dependencies

# required
FIND_PACKAGE(Spatialindex REQUIRED)

FIND_PACKAGE(Proj)

#FIND_PACKAGE(Eigen3)
#INCLUDE( ${EIGEN3_INCLUDE_DIR} )
# MESSAGE ( "  Found Eigen Include:" ${EIGEN3_INCLUDE_DIR})

#optional since we can use internal sqlite
IF (NOT WITH_INTERNAL_SQLITE3)
  FIND_PACKAGE(Sqlite3)
  IF (NOT SQLITE3_FOUND)
    MESSAGE (SEND_ERROR "sqlite3 dependency was not found (try compiling with sqlite3 internal)!")
  ENDIF (NOT SQLITE3_FOUND)
ENDIF (NOT WITH_INTERNAL_SQLITE3)

FIND_PACKAGE(FANN)

FIND_PACKAGE(GEOS)

FIND_PACKAGE(GDAL)

FIND_PACKAGE(Postgres) # PostgreSQL provider, SPIT plugin

FIND_PACKAGE(Expat)    # GPS importer plugin
FIND_PACKAGE(GSL)      # Georeferencer
IF (WITH_GRASS)
  FIND_PACKAGE(GRASS)    # GRASS plugin
ENDIF (WITH_GRASS)
IF (WITH_SPATIALITE)
  FIND_PACKAGE(SPATIALITE)    # SPATIALITE provider
ENDIF (WITH_SPATIALITE)

IF (WITH_BINDINGS)
  # python support:
  # - mapserver export tool
  # - bindings
  INCLUDE (Python) # file cmake/Python.cmake
ENDIF (WITH_BINDINGS)

IF (NOT PROJ_FOUND)
 	MESSAGE (SEND_ERROR "PROJ were not found!")
ENDIF(NOT PROJ_FOUND)
IF (NOT GEOS_FOUND)
	 MESSAGE (SEND_ERROR "GEOS were not found!")
ENDIF (NOT GEOS_FOUND)
IF (NOT GDAL_FOUND)
  MESSAGE (SEND_ERROR "GDAL were not found!")
ENDIF (NOT GDAL_FOUND)

IF (POSTGRES_FOUND)
  # following variable is used in qgsconfig.h
  SET (HAVE_POSTGRESQL TRUE)
ENDIF (POSTGRES_FOUND)

FIND_PACKAGE(PQXX)
MESSAGE(STATUS "pqxx include: ${PQXX_HEADER_PATH}")
MESSAGE(STATUS "pqxx library: ${PQXX_LIBRARY}")

IF (SPATIALITE_FOUND)
  # following variable is used in qgsconfig.h
  SET (HAVE_SPATIALITE TRUE)
ENDIF (SPATIALITE_FOUND)


#############################################################
# search for Qt4

SET( QT_USE_QT3SUPPORT TRUE )
SET( QT_USE_QTXML      TRUE )
SET( QT_USE_QTNETWORK  TRUE )
SET( QT_USE_QTSVG      TRUE )
SET( QT_USE_QTSQL      TRUE )
IF (ENABLE_TESTS)
  SET( QT_USE_QTTEST  TRUE )
  ENABLE_TESTING()
ENDIF (ENABLE_TESTS)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )

# Disable automatic conversion from QString to ASCII 8-bit strings (char *)
# (Keeps code compatible with Qt/Mac/64bit)
# ADD_DEFINITIONS(-DQT_NO_CAST_TO_ASCII)

FIND_PROGRAM(QT_LRELEASE_EXECUTABLE
    NAMES lrelease
    PATHS ${QT_BINARY_DIR}
    NO_DEFAULT_PATH
    )

###############################################################
FIND_PACKAGE(QGIS REQUIRED)
IF (NOT QGIS_FOUND)
  MESSAGE (SEND_ERROR "QGIS dependency was not found!")
ENDIF (NOT QGIS_FOUND)

#############################################################
# enable warnings

IF (PEDANTIC)
  MESSAGE ("Pedantic compiler settings enabled")
  IF(MSVC)
    ADD_DEFINITIONS( /W4 )

    # disable warnings
    ADD_DEFINITIONS( /wd4100 )  # unused formal parameters
    ADD_DEFINITIONS( /wd4127 )  # constant conditional expressions (used in Qt template classes)
    ADD_DEFINITIONS( /wd4510 )  # default constructor could not be generated (sqlite3_index_info, QMap)
    ADD_DEFINITIONS( /wd4512 )  # assignment operator could not be generated (sqlite3_index_info)
    ADD_DEFINITIONS( /wd4610 )  # user defined constructor required (sqlite3_index_info)
  ELSE (MSVC)
    ADD_DEFINITIONS( -Wall -Wno-long-long -Wformat-security )
    # Qt produces lots of warnings with strict aliasing (as of Qt 4.4.0 & GCC 4.3)
    # ADD_DEFINITIONS( -fstrict-aliasing -Wstrict-aliasing=1 )
  ENDIF (MSVC)
ENDIF (PEDANTIC)

#IF (CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS(-DQGISDEBUG=1)
#ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

#############################################################
# platform specific stuff

set(CMAKE_CXX_FLAGS "-std=c++11 -std=gnu++11")

IF (WIN32)
  SET (DEFAULT_LIB_SUBDIR     lib)
  SET (DEFAULT_DATA_SUBDIR    .)
  SET (DEFAULT_PLUGIN_SUBDIR  plugins)
  SET (DEFAULT_INCLUDE_SUBDIR include)
  
  IF (MSVC)
    # Python parts need to be build with nmake not vcexpress
    # if you dont add this clause, extra blank msvc projects
    # will pop up mid way through the build process and block
    # the build, plus the python bindings wont build correctly
    SET (CMAKE_MAKE_PROGRAM nmake)
    SET (DEFAULT_BIN_SUBDIR bin)
    # put all the build products into a single directory
    # under build (doesnt affect install target) to make for
    # easier debugging.
    # SET(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    # SET(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
    # SET(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})
    #tell msvc compiler to use main instead of winmain as the
    #application entry point
    #SET(QT_USE_QTMAIN TRUE) 
    # Turn on defines for non standard maths stuff
    ADD_DEFINITIONS(-D_USE_MATH_DEFINES)

    # Turn off deprecation warnings
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)
    # Some file access stuff not defined in native win32
    # environment
    ADD_DEFINITIONS(-DF_OK=0)
    ADD_DEFINITIONS(-DX_OK=1)
    ADD_DEFINITIONS(-DW_OK=2)
    ADD_DEFINITIONS(-DR_OK=4)
    
    ADD_DEFINITIONS(-DQGISDEBUG=1)

    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/win_build/vcdeps/. DESTINATION .)
  ELSE(MSVC)
    SET (DEFAULT_BIN_SUBDIR     .)
  ENDIF(MSVC)
ELSE (WIN32)

  IF (APPLE)
    # for Mac OS X, everything is put inside an application bundle
    SET (CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS)
    # path for library references
    SET (CMAKE_INSTALL_NAME_DIR @executable_path/lib)
    #this will define ${APP_SERVICES_LIBRARY}
    FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
  ENDIF (APPLE)

  # common for MAC and UNIX


ENDIF (WIN32)

IF (WIN32)
  # expect that classes are being imported
  # Note: MSVC doesn't like when the macros are quotes
  # and MSYS doesn't like them unqouted (bacause of braces)
  IF (MSVC)
    ADD_DEFINITIONS("-DCORE_EXPORT=__declspec(dllimport)")
    ADD_DEFINITIONS("-DGUI_EXPORT=__declspec(dllimport)")
    ADD_DEFINITIONS("-DPYTHON_EXPORT=__declspec(dllimport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("\"-DCORE_EXPORT=__declspec(dllimport)\"")
    ADD_DEFINITIONS("\"-DGUI_EXPORT=__declspec(dllimport)\"")
    ADD_DEFINITIONS("\"-DPYTHON_EXPORT=__declspec(dllimport)\"")
  ENDIF (MSVC)
ELSE (WIN32)
  # other compilers don't use that MSVC construct
  ADD_DEFINITIONS(-DCORE_EXPORT=)
  ADD_DEFINITIONS(-DGUI_EXPORT=)
  ADD_DEFINITIONS(-DPYTHON_EXPORT=)
ENDIF (WIN32)


#############################################################
# enable use of c++11 features where available
# full c++11 support in clang 3.3+: http://clang.llvm.org/cxx_status.html
# for Mac, this is probably Apple LLVM 4.2 (based on LLVM 3.2svn, in XCode 4.6+)
#   or definitely Apple LLVM 5.0 (based on LLVM 3.3svn, in Xcode 5+):
#   https://gist.github.com/yamaya/2924292

IF (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  IF (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    SET(USE_CXX_11 TRUE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  ENDIF()
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  IF ((NOT APPLE AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "3.2")
       OR (APPLE AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.1"))
    SET(USE_CXX_11 TRUE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-error=c++11-narrowing")
  ENDIF()
ELSEIF (MSVC AND MSVC_VERSION GREATER 1600)
  SET(USE_CXX_11 TRUE)
ELSE()
  SET(USE_CXX_11 FALSE)
ENDIF()

#allow override keyword if available
IF (NOT USE_CXX_11)
  ADD_DEFINITIONS("-Doverride=")
  ADD_DEFINITIONS("-Dnullptr=0")
ENDIF()


#############################################################
# enable warnings

IF (PEDANTIC)
  MESSAGE (STATUS "Pedantic compiler settings enabled")
  IF(MSVC)
    SET(_warnings "")
    IF (NOT USING_NMAKE)
      SET(_warnings "${_warnings} /W4" )
    ENDIF (NOT USING_NMAKE)

    # disable warnings
    SET(_warnings "${_warnings} /wd4100 ")  # unused formal parameters
    SET(_warnings "${_warnings} /wd4127 ")  # constant conditional expressions (used in Qt template classes)
    SET(_warnings "${_warnings} /wd4190 ")  # 'identifier' has C-linkage specified, but returns UDT 'identifier2' which is incompatible with C
    SET(_warnings "${_warnings} /wd4231 ")  # nonstandard extension used : 'identifier' before template explicit instantiation (used in Qt template classes)
    SET(_warnings "${_warnings} /wd4244 ")  # conversion from '...' to '...' possible loss of data
    SET(_warnings "${_warnings} /wd4251 ")  # needs to have dll-interface to be used by clients of class (occurs in Qt template classes)
    SET(_warnings "${_warnings} /wd4275 ")  # non dll-interface class '...' used as base for dll-interface class '...'
    SET(_warnings "${_warnings} /wd4505 ")  # unreferenced local function has been removed (QgsRasterDataProvider::extent)
    SET(_warnings "${_warnings} /wd4510 ")  # default constructor could not be generated (sqlite3_index_info, QMap)
    SET(_warnings "${_warnings} /wd4512 ")  # assignment operator could not be generated (sqlite3_index_info)
    SET(_warnings "${_warnings} /wd4610 ")  # user defined constructor required (sqlite3_index_info)
    SET(_warnings "${_warnings} /wd4706 ")  # assignment within conditional expression (pal)
    SET(_warnings "${_warnings} /wd4800 ")  # 'int' : forcing value to bool 'true' or 'false' (performance warning)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_warnings}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_warnings}")
  ELSE (MSVC)
    # add warnings via flags (not as definitions as on Mac -Wall can not be overridden per language )
    SET(_warnings "-Wall -Wextra -Wno-long-long -Wformat-security -Wno-strict-aliasing")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_warnings}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_warnings}")

    # Qt produces lots of warnings with strict aliasing (as of Qt 4.4.0 & GCC 4.3)
    # There are redundant declarations in Qt and GDAL
    # ADD_DEFINITIONS( -fstrict-aliasing -Wstrict-aliasing=1 -Wredundant-decls )

    IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage -Wno-overloaded-virtual")
    ENDIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

    # add any extra CXXFLAGS flags set by user. can be -D CXX_EXTRA_FLAGS or environment variable
    # command line -D option overrides environment variable
    # e.g. useful for suppressing transient upstream warnings in dependencies, like Qt
    SET(CXX_EXTRA_FLAGS "" CACHE STRING "Additional appended CXXFLAGS")
    IF ("${CXX_EXTRA_FLAGS}" STREQUAL "" AND DEFINED $ENV{CXX_EXTRA_FLAGS})
      SET(CXX_EXTRA_FLAGS "$ENV{CXX_EXTRA_FLAGS}")
    ENDIF ("${CXX_EXTRA_FLAGS}" STREQUAL "" AND DEFINED $ENV{CXX_EXTRA_FLAGS})
    IF (NOT "${CXX_EXTRA_FLAGS}" STREQUAL "")
      MESSAGE (STATUS "Appending CXX_EXTRA_FLAGS")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")
    ENDIF (NOT "${CXX_EXTRA_FLAGS}" STREQUAL "")
  ENDIF (MSVC)

ENDIF (PEDANTIC)


IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Qunused-arguments")
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Qunused-arguments")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Qunused-arguments")
ENDIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
  # spatialite crashes on ppc - see bugs.debian.org/603986
  ADD_DEFINITIONS( -fno-strict-aliasing )
ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")

IF (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  MESSAGE (STATUS "Debug output enabled")
  ADD_DEFINITIONS(-DQGISDEBUG=1)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)

IF(MSVC)
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DQGISDEBUG=1")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQGISDEBUG=1")
  # disable macros that offend std::numeric_limits<T>::min()/max()
  ADD_DEFINITIONS(-DNOMINMAX)
ENDIF(MSVC)

IF(ENABLE_COVERAGE)
  INCLUDE("cmake/modules/coverage/CodeCoverage.cmake")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage --coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage --coverage")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage --coverage")
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage --coverage")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage --coverage")
  SETUP_TARGET_FOR_COVERAGE(qgis_coverage ctest coverage)
ENDIF(ENABLE_COVERAGE)



###############################################################
FIND_PACKAGE(QGIS REQUIRED)
IF (NOT QGIS_FOUND)
  MESSAGE (SEND_ERROR "QGIS dependency was not found!")
ENDIF (NOT QGIS_FOUND)

    MESSAGE(STATUS "1) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
    MESSAGE(STATUS "1) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")

#############################################################
# user-changeable settings which can be used to customize
# layout of QGIS installation
# (default values are platform-specific)
#
SET (QGIS_BIN_SUBDIR     ${DEFAULT_BIN_SUBDIR}     CACHE STRING "Subdirectory where executables will be installed")
SET (QGIS_LIB_SUBDIR     ${DEFAULT_LIB_SUBDIR}     CACHE STRING "Subdirectory where libraries will be installed")
SET (QGIS_DATA_SUBDIR    ${DEFAULT_DATA_SUBDIR}    CACHE STRING "Subdirectory where QGIS data will be installed")
SET (QGIS_PLUGIN_SUBDIR  ${DEFAULT_PLUGIN_SUBDIR}  CACHE STRING "Subdirectory where plugins will be installed")
SET (QGIS_INCLUDE_SUBDIR ${DEFAULT_INCLUDE_SUBDIR} CACHE STRING "Subdirectory where header files will be installed")

# mark *_SUBDIR variables as advanced as this is not something
# that an average user would use
#MARK_AS_ADVANCED (QGIS_BIN_SUBDIR QGIS_LIB_SUBDIR QGIS_DATA_SUBDIR QGIS_PLUGIN_SUBDIR QGIS_INCLUDE_SUBDIR)

# full paths for the installation
SET (QGIS_BIN_DIR     ${QGIS_BIN_SUBDIR})
SET (QGIS_LIB_DIR     ${QGIS_LIB_SUBDIR})
SET (QGIS_DATA_DIR    ${QGIS_DATA_SUBDIR})
SET (QGIS_PLUGIN_DIR  ${QGIS_PLUGIN_SUBDIR})
SET (QGIS_INCLUDE_DIR ${QGIS_INCLUDE_SUBDIR})

    MESSAGE(STATUS "2) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
    MESSAGE(STATUS "2) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")
	MESSAGE(STATUS "2) Found QGIS Core Library: ${QGIS_CORE_LIBRARY}")
	MESSAGE(STATUS "2) Found QGIS GUI Library: ${QGIS_GUI_LIBRARY}")

# manual page - makes sense only on unix systems
IF (UNIX)
  SET (DEFAULT_MANUAL_SUBDIR  man)
  SET (QGIS_MANUAL_SUBDIR  ${DEFAULT_MANUAL_SUBDIR}  CACHE STRING "Subdirectory where manual files will be installed")
  MARK_AS_ADVANCED (QGIS_MANUAL_SUBDIR)
  SET (QGIS_MANUAL_DIR  ${CMAKE_INSTALL_PREFIX}/${QGIS_MANUAL_SUBDIR})
ENDIF (UNIX)

    MESSAGE(STATUS "3) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
    MESSAGE(STATUS "3) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")
	MESSAGE(STATUS "3) Found QGIS Core Library: ${QGIS_CORE_LIBRARY}")
	MESSAGE(STATUS "3) Found QGIS GUI Library: ${QGIS_GUI_LIBRARY}")

#############################################################
# Uninstall stuff see: http://www.vtk.org/Wiki/CMake_FAQ
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_templates/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

    MESSAGE(STATUS "4) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
    MESSAGE(STATUS "4) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")
	MESSAGE(STATUS "4) Found QGIS Core Library: ${QGIS_CORE_LIBRARY}")
	MESSAGE(STATUS "4) Found QGIS GUI Library: ${QGIS_GUI_LIBRARY}")


########################################################
# Files
SET (IMAGE_FILES
	DataBase/images/add.png			
	DataBase/images/back.png
	DataBase/images/configure.png
	DataBase/images/connect.png
	DataBase/images/copy.png
	DataBase/images/cut.png
	DataBase/images/dbgrun.png
	DataBase/images/down.png
	DataBase/images/logo_icon.png
	DataBase/images/logo_icon.xcf
	DataBase/images/new.png
	DataBase/images/next.png
	DataBase/images/open.png
	DataBase/images/paste.png
	DataBase/images/previous.png
	DataBase/images/rap_logo.png
	DataBase/images/rap_logo.xcf
	DataBase/images/reload.png
	DataBase/images/remove.png
	DataBase/images/save.png
	DataBase/images/up.png
)

SET (XML_FILES
	DataBase/settings.xml
	DataBase/structure.xml
)

SET (qwireless_MOC_HDRS
#	qrap.h
        cMouseEvents.h
        qwireless.h
        DataBase/Config.h
        DataBase/cSettings.h
        DataBase/cAlert.h
        DataBase/cBase.h
        DataBase/Types.h
        DataBase/cDatabase.h
        DataBase/LoginDialog.h
        Interface/cPlaceSite.h
        DataBase/cSiteEditTools.h
        DataBase/MainWindow.h
        DataBase/RapTab.h
        DataBase/cRapTableTab.h
        DataBase/cRapFormBuilder.h
        DataBase/RapDbCommunicator.h
        DataBase/cRapSites.h
        DataBase/cFilter.h
        DataBase/cRapCells.h
        DataBase/cRapInventory.h
        DataBase/cRapSupportingTables.h
        DataBase/cRapLinks.h
        DataBase/cRapServedKeyLocations.h
        DataBase/cRapRasterFiles.h
        DataBase/cRapMeasurements.h

        Prediction/cGeoP.h
        Prediction/cRaster.h
        Prediction/cGDAL.h
        Prediction/cMemmngr.h
        Prediction/cBIN.h

)

SET (qwireless_SRCS
#	qrap.cpp
        cMouseEvents.cpp
        qwireless.cpp
        DataBase/MainWindow.cpp
        DataBase/cSettings.cpp
        DataBase/cAlert.cpp
        DataBase/cBase.cpp
        DataBase/cDatabase.cpp
        DataBase/LoginDialog.cpp
        Interface/cPlaceSite.cpp
        DataBase/cSiteEditTools.cpp
        DataBase/RapTab.cpp
        DataBase/cRapTableTab.cpp
        DataBase/cRapFormBuilder.cpp
        DataBase/RapDbCommunicator.cpp
        DataBase/cRapSites.cpp
        DataBase/cSiteEditTools.cpp
        DataBase/cRapSites.cpp
        DataBase/cFilter.cpp
        DataBase/cRapCells.cpp
        DataBase/cRapInventory.cpp
        DataBase/cRapSupportingTables.cpp
        DataBase/cRapLinks.cpp
        DataBase/cRapServedKeyLocations.cpp
        DataBase/cRapRasterFiles.cpp
        DataBase/cRapMeasurements.cpp

        Prediction/cGeoP.cpp
        Prediction/cRaster.cpp
        Prediction/cGDAL.cpp
        Prediction/cMemmngr.cpp
        Prediction/cBIN.cpp
)

SET (qwireless_PLUGIN_SRCS
     ${qwireless_SRCS}
)

SET (qwireless_UIS 
        Interface/Site.ui


)



SET (qwireless_PLUGIN_MOC_HDRS
	${qwireless_MOC_HDR}
)


SET (qwireless_RCCS  qrap.qrc 
        DataBase/application.qrc 
)

	MESSAGE(STATUS "5) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
	MESSAGE(STATUS "5) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")
	MESSAGE(STATUS "5) Found QGIS Core Library: ${QGIS_CORE_LIBRARY}")
	MESSAGE(STATUS "5) Found QGIS GUI Library: ${QGIS_GUI_LIBRARY}")

INSTALL(CODE "MESSAGE(${CMAKE_CURRENT_BINARY_DIR})")

########################################################
# Build

QT4_WRAP_UI (qwireless_UIS_H  ${qwireless_UIS})

QT4_WRAP_CPP (qwireless_MOC_SRCS  ${qwireless_MOC_HDRS})
 
QT4_ADD_RESOURCES(qwireless_RCC_SRCS ${qwireless_RCCS})

#ADD_LIBRARY (qrapplugin MODULE ${qrap_SRCS}  ${qrap_MOC_SRCS}  ${qrap_RCC_SRCS} ${qrap_UIS_H})
ADD_LIBRARY (qwirelessplugin MODULE ${qwireless_SRCS}  ${qwireless_MOC_SRCS}  ${qwireless_RCC_SRCS} ${qwireless_UIS_H})

INCLUDE_DIRECTORIES(
      	${CMAKE_CURRENT_BINARY_DIR}
     	${QGIS_INCLUDE_DIR}
     	${GRASS_INCLUDE_DIR}
     	${GDAL_INCLUDE_DIR}
     	${PROJ_INCLUDE_DIR}
	${POSTGRES_INCLUDE_DIR}
	${PQXX_HEADER_PATH}
#	${EIGEN3_INCLUDE_DIR}
	${QWT_INCLUDE_DIR}
	${QT_INCLUDES}
	/usr/include
#	/usr/include/c++
#	/usr/include/qgis
	/usr/include/eigen3
#	/usr/include/gdal
   	/usr/include/Poco/
#	/usr/include/libcmaes/src/
	/usr/include/qwt-qt4
#	/usr/include/qwt

	../libcmaes/src
	../libcmaes
)


TARGET_LINK_LIBRARIES(qwirelessplugin
	${QGIS_CORE_LIBRARY}
	${QGIS_GUI_LIBRARY}
	${GRASS_LIBRARIES}
	${POSTGRES_LIBRARY}
	${QT_LIBRARIES}
  	${QT_QTCORE_LIBRARY} 
  	${QT_QTGUI_LIBRARY} 
  	${QT_QTXML_LIBRARY} 
  	${QT_QTSVG_LIBRARY} 
  	${QT_QTNETWORK_LIBRARY} 
  	${QT_QTSQL_LIBRARY}
    	libpqxx.so
    	libPocoXML.so
	libqgis_core.so
  	libqgis_gui.so
	libqwt-qt4.so
	libdoublefann.so.2
	libfann.so.2
	libfixedfann.so.2
	libfloatfann.so.2

)


    MESSAGE(STATUS "6) Found QGIS Plugins Dir: ${QGIS_PLUGIN_DIR}")
	MESSAGE(STATUS "6) Found QGIS Core Library: ${QGIS_CORE_LIBRARY}")
	MESSAGE(STATUS "6) Found QGIS GUI Library: ${QGIS_GUI_LIBRARY}")
    MESSAGE(STATUS "6) Found QGIS Include Dir: ${QGIS_INCLUDE_DIR}")

    MESSAGE(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

########################################################
# Install
INSTALL(CODE "MESSAGE(\"Installing core...\")")


INSTALL(TARGETS qwirelessplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

INSTALL(CODE "MESSAGE(\"Installing Settings Files...\")")
INSTALL(FILES ${XML_FILES}
       DESTINATION ${QGIS_PLUGIN_DIR}
)

      
INSTALL(CODE "MESSAGE(\"Installing Images...\")")
INSTALL(FILES ${IMAGE_FILES}
        DESTINATION ${QGIS_PLUGIN_DIR}/images)


