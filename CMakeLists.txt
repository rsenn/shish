cmake_minimum_required(VERSION 2.6)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

set(PACKAGE_NAME shish)
set(PACKAGE_VERSION 0.8)
set(PACKAGE_TARNAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")

project(${PACKAGE_NAME} C)
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'MinSizeRel' as none was specified.")
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckFunctionExists)

find_program(STRIP NAMES strip gstrip)

check_c_compiler_flag(-flto F_LTO)

if(F_LTO)
  add_definitions(-flto)
endif(F_LTO)

check_include_file(alloca.h HAVE_ALLOCA_H)

if(HAVE_ALLOCA_H)
  check_symbol_exists(alloca alloca.h HAVE_ALLOCA)
endif()

check_include_file(signal.h HAVE_SIGNAL_H)
#if(HAVE_SIGNAL_H)
#  check_symbol_exists(sigset_t signal.h HAVE_SIGSET_T)
#check_symbol_exists(sigset_t signal.h HAVE_SIGSET_T)
#  check_type_size(sigset_t SIZEOF_SIGSET_T)
#
#  if(SIZEOF_SIGSET_T)
#          set(HAVE_SIGSET_T 1)
#  else()
#          set(HAVE_SIGSET_T 0)
#  endif()
#endif()

check_function_exists(sys_siglist HAVE_SYS_SIGLIST)

configure_file("${CMAKE_SOURCE_DIR}/config.h.cmake" "${CMAKE_BINARY_DIR}/config.h")

add_definitions(-Wall -DHAVE_CONFIG_H=1)

set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")


include_directories(${CMAKE_BINARY_DIR} lib src)

#add_definitions(-DHAVE_CONFIG_H=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY src)

file(GLOB LIBSOURCES lib/*.h lib/*/*.c)
file(GLOB SOURCES src/*.h src/*/*.c)

add_library(sh ${LIBSOURCES})

add_executable(shish ${SOURCES})
target_link_libraries(shish sh)

if(STRIP)
  add_custom_command(TARGET shish POST_BUILD COMMAND ${STRIP} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shish)
endif(STRIP)
