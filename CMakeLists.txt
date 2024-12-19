cmake_minimum_required(VERSION 3.27)

project("BitLockerTool" VERSION 0.0.1 DESCRIPTION "A custom tool for BitLocker" LANGUAGES C CXX)

include(_cmake/ProjectOptions.cmake)
setup_project_settings("BitLockerTool")

add_executable(BitLockerTool)

add_library(BitLockerTool_Options INTERFACE)
setup_project_options("BitLockerTool" "BitLockerTool_Options")

include(_cmake/ProjectWarnings.cmake)
add_library(BitLockerTool_Warings INTERFACE)
setup_project_warnings("BitLockerTool" "BitLockerTool_Warings")

# Download dependencies
include(_cmake/CPM.cmake)

CPMAddPackage(
  NAME Boost
  VERSION 1.87.0
  GITHUB_REPOSITORY "boostorg/boost"
  GIT_TAG "boost-1.87.0"
  GIT_SHALLOW ON
  OPTIONS
  "BOOST_ENABLE_CMAKE ON"
  "BOOST_INCLUDE_LIBRARIES:STRING=asio\\\\;process\'"
  "BOOST_EXCLUDE_LIBRARIES \'python\\\\;parameter_python\'"
  "BOOST_ENABLE_MPI OFF"
  "BOOST_ENABLE_PYTHON OFF"
  "BUILD_SHARED_LIBS OFF"
  "BOOST_ASIO_HAS_IO_URING ON"
  "BOOST_ASIO_DISABLE_EPOLL ON"
  "BOOST_ASIO_HAS_IO_URING_AS_DEFAULT ON"
  FIND_PACKAGE_ARGUMENTS "COMPONENTS asio process"
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 11.0.2
  OPTIONS "FMT_HEADER_ONLY ON" 
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  NAME ctre
	GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions
	GIT_SHALLOW ON
	GIT_TAG v3.9.0
  OPTIONS
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# specify header/source files
target_sources(BitLockerTool
  PUBLIC
    FILE_SET HEADERS 
    BASE_DIRS src
    FILES
      src/DiskPart.hpp
      src/Common.hpp
      src/Command.hpp
      src/Unit.hpp
  PRIVATE
    src/BitLockerTool.cpp
    src/DiskPart.cpp
    src/Command.cpp
)

# link dependencies
target_link_libraries(BitLockerTool
  PRIVATE
  $<BUILD_INTERFACE:BitLockerTool_Options>
  $<BUILD_INTERFACE:BitLockerTool_Warings>

  fmt::fmt-header-only
  Boost::asio
  Boost::process
  ctre::ctre
  Shell32
)

set_target_properties(BitLockerTool PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")