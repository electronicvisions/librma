cmake_minimum_required(VERSION 3.5.1)

project(librma2 C)
#include(CheckIncludeFile)

set(VERSION_MAJOR   2   CACHE STRING "Project major version number.")
set(VERSION_MINOR   2   CACHE STRING "Project minor version number.")
set(VERSION_PATCH   0   CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

set(EXTOLL_R2_INSTALL_DIR $ENV{EXTOLL_R2_INSTALL_DIR})
if (DEFINED EXTOLL_R2_INSTALL_DIR)
  message(STATUS "EXTOLL_R2_INSTALL_DIR is set to ${EXTOLL_R2_INSTALL_DIR}")
else()
  set(EXTOLL_R2_INSTALL_DIR /opt/extoll/x86_64)
  message(STATUS "EXTOLL_R2_INSTALL_DIR was not set. Setting it to ${EXTOLL_R2_INSTALL_DIR}")
endif()

set(EXTOLL_INSTALL_PREFIX ${EXTOLL_R2_INSTALL_DIR})
set(CMAKE_INSTALL_PREFIX ${EXTOLL_INSTALL_PREFIX})

set(EXTOLL_R2_HOME $ENV{EXTOLL_R2_HOME})
if(DEFINED EXTOLL_R2_HOME)
	message(STATUS "EXTOLL_R2_HOME is set to ${EXTOLL_R2_HOME}")
else()
	message(FATAL_ERROR "EXTOLL_R2_HOME is not set, abort.")
endif()

#Todo: is that all where we can find the header? do we need something like $PREFIX?
set(EXTOLL_INCLUDE_PATHS ${EXTOLL_R2_HOME}/include/)

#CHECK_INCLUDE_FILE(rma2drv.h RMA2_MOD_HEADER)
#find_file(RMA2_MOD_HEADER rma2drv.h ${EXTOLL_INCLUDE_PATHS})
#if(NOT RMA2_MOD_HEADER)
#	message(FATAL_ERROR "Could not find the RMA2 Module header.")
#endif()

find_file(RMA2_REGION_HEADER rma2_region.h ${EXTOLL_INCLUDE_PATHS})
if(NOT RMA2_REGION_HEADER)
	message(FATAL_ERROR "Could not find the RMA2 region header.")
endif()

find_file(RMA2_IOCTL_HEADER rma2_ioctl.h ${EXTOLL_INCLUDE_PATHS})
if(NOT RMA2_IOCTL_HEADER)
	message(FATAL_ERROR "Could not find the RMA2 ioctl header.")
endif()

find_file(PMAP_HEADER pmap.h ${EXTOLL_INCLUDE_PATHS})
if(NOT PMAP_HEADER)
	message(FATAL_ERROR "Could not find pmap header.")
endif()

find_file(ARCH_DEFINE arch_define.h ${EXTOLL_INCLUDE_PATHS})
if(NOT ARCH_DEFINE)
	message(FATAL_ERROR "Could not find Extolls arch_define header.")
endif()

#get_filename_component(EXTOLL_R2_INCLUDE_PATH ${RMA2_MOD_HEADER} DIRECTORY)
get_filename_component(EXTOLL_R2_INCLUDE_PATH ${RMA2_REGION_HEADER} DIRECTORY)
message(STATUS "RMA header files are in ${EXTOLL_R2_INCLUDE_PATH}")


include_directories(${EXTOLL_R2_INCLUDE_PATH})
include_directories(include)

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(doc)

EXECUTE_PROCESS(COMMAND "date" "+%s" OUTPUT_VARIABLE DATE)

#Packagin support
set(CPACK_GENERATOR "RPM;TGZ")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_RPM_PACKAGE_RELEASE "0.${DATE}")
#set(CPACK_RPM_PACKAGE_RELEASE "1")
#set(CPACK_PACKAGE_INSTALL_DIRECTORY ${EXTOLL_INSTALL_PREFIX})
#set(CPACK_PACKAGE_INSTALL_PREFIX ${EXTOLL_INSTALL_PREFIX})
#set(CPACK_PACKAGE_DEFAULT_LOCATION ${EXTOLL_INSTALL_PREFIX})
set(CPACK_PACKAGING_INSTALL_PREFIX ${EXTOLL_INSTALL_PREFIX})

#Packaging-specific dependendies
set(CPACK_RPM_PACKAGE_REQUIRES "extoll-driver >= 0.1.0")
include(CPack)
