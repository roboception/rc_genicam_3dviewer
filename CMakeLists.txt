cmake_minimum_required (VERSION 2.8.12)

# Don't set PROJECT_VERSION to empty string when no VERSION is given to project() command.
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 OLD)
endif()

project(rc_genicam_3dviewer)
include(cmake/project_version.cmake)

enable_testing()

include_directories(${CMAKE_SOURCE_DIR})

# - Find manditory package cvkit -

find_package(CVKIT 1.9.0 REQUIRED)
find_package(RC_GENICAM_API 2.0.0 REQUIRED)

include_directories(${CVKIT_INCLUDE_DIRS})
include_directories(${RC_GENICAM_API_INCLUDE_DIRS})

# - Configuration for code optimization -

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE CACHE STRING "Build type: DEBUG or RELEASE" FORCE)
endif ()

include(cmake/optimization_flags.cmake)
# uncomment next line to use cuda
#include(cmake/cuda.cmake)

# - Standard definitions -

add_definitions(-Wall)

# - Build individual parts -

add_subdirectory(tools)

# export project targets

# ??? Declare (shared) libraries that are created by your project. For all
# libraries that appear here, the following line should exist in a cmake
# file in a sub directory:
# install(TARGETS rcXXX EXPORT PROJECTTargets COMPONENT bin DESTINATION lib)
#set(PROJECT_LIBRARIES rcXXX)

# ??? Declare static libraries that are created by your project. For all
# libraries that appear here, the following line should exist in a cmake
# file in a sub directory:
# install(TARGETS rcXXX_static EXPORT PROJECTTargets COMPONENT dev DESTINATION lib)
#set(PROJECT_STATIC_LIBRARIES rcXXX_static)

# ??? install generated version.h file
# the destination path might have to be adjusted for your project
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/project_version.h COMPONENT dev DESTINATION include/${PROJECT_NAME})

include(cmake/configure_link_libs.cmake)

# add a target to generate API documentation with Doxygen

add_subdirectory(doc)

# - Define information for packaging -

# optionally set description and maintainer (if not set will use defaults from package_debian.cmake)
#set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Roboception package description")

# optionally set package maintainer (otherwise defaults to Roboception <info@roboception.de>)
#set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.name@roboception.de>")

# optionally specify dependencies of the debian package here (comma separated!)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "cvkit (>= 1.9.0), rc_genicam_api (>= 2.0.0)")

include(cmake/package_debian.cmake)
