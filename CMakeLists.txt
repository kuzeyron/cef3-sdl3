# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# See the included README.md file for usage instructions.
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")

cmake_minimum_required(VERSION 3.19)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)
# set (CMAKE_CXX_STANDARD 11) ### FORBIDDEN TO USE

# Project name.
project(sdlcef)

# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY OS_FOLDERS ON)


#
# CEF configuration.
#

# Specify the CEF distribution version.
set(CEF_VERSION "136.1.6+g1ac1b14+chromium-136.0.7103.114")

# Determine the platform.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(CEF_PLATFORM "macosx64")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "linux64")
  else()
    set(CEF_PLATFORM "linux32")
  endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "windows64")
  else()
    set(CEF_PLATFORM "windows32")
  endif()
endif()

# Add this project's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Download and extract the CEF binary distribution (executes DownloadCEF.cmake).
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/third_party/cef")
message(STATUS "CEF_ROOT is: ${CEF_ROOT}")

# Add the CEF binary distribution's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# Load the CEF configuration (executes FindCEF.cmake).
find_package(CEF REQUIRED)

# include folder for the project
# include_directories(${CMAKE_SOURCE_DIR}/include)

#
# Target configuration.
#

# Include the libcef_dll_wrapper target (executes libcef_dll/CMakeLists.txt).
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

# Allow includes relative to the current source directory.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Configure building of the examples.
option(WITH_EXAMPLES "Enable or disable building of examples." OFF)
if(WITH_EXAMPLES)
  # Include example application targets.
  add_subdirectory(examples)
endif()

# TODO: Include other application targets here.

# Include the SDL target
add_subdirectory(src)

# Display configuration settings.
PRINT_CEF_CONFIG()

if(NOT WITH_EXAMPLES)
  message(STATUS "")
  message(STATUS "NOTE: Targets from the examples folder are not included.")
  message(STATUS "Add -DWITH_EXAMPLES=On to the CMake command-line to include them.")
  message(STATUS "")
endif()
