cmake_minimum_required (VERSION 2.8.12)

project(rc_genicam_3dviewer)
include(cmake/project_version.cmake)

enable_testing()

include_directories(${CMAKE_SOURCE_DIR})

# - Find manditory package cvkit -

find_package(CVKIT 2.0.3 REQUIRED)
find_package(RC_GENICAM_API 2.2.3 REQUIRED)

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
if (CMAKE_MAJOR_VERSION VERSION_LESS "3.1.0")
  add_definitions(-std=c++11)
else ()
  set(CMAKE_CXX_STANDARD 11)
endif ()

# - Disable some warnings when compiling with Visual Studio under Windows -

if (MSVC)
  add_definitions(/DGENICAM_NO_AUTO_IMPLIB)

  add_definitions("/wd4251")
  add_definitions("/wd4505")
  add_definitions("/wd5045")
  add_definitions("/wd4100")
  add_definitions("/wd4365")
  add_definitions("/wd4003")
  add_definitions("/wd4514")
  add_definitions("/wd4710")
  add_definitions("/wd4820")
  add_definitions("/wd4435")
  add_definitions("/wd4668")
  add_definitions("/wd4127")
  add_definitions("/wd4265")
  add_definitions("/wd4623")
  add_definitions("/wd4996")
  add_definitions("/wd4437")
  add_definitions("/wd4571")
  add_definitions("/wd4355")
  add_definitions("/wd4061")
  add_definitions("/wd4800")
  add_definitions("/wd4711")
  add_definitions("/wd4774")
  add_definitions("/wd4625")
  add_definitions("/wd4626")
  add_definitions("/wd5026")
  add_definitions("/wd5027")
  add_definitions("/wd5039")
  add_definitions("/wd5204")
endif ()

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
set(CPACK_DEBIAN_PACKAGE_DEPENDS "cvkit (>= 2.2.1), rc-genicam-api (>= 2.0.3) | ros-$ENV{ROS_DISTRO}-rc-genicam-api (>= 2.0.3)")

include(cmake/package_debian.cmake)
