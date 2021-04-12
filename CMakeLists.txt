cmake_minimum_required(VERSION 3.0)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

set(PACKAGE_NAME shish)
set(PACKAGE_VERSION 0.8)
set(PACKAGE_TARNAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")

project(${PACKAGE_NAME} C)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'MinSizeRel' as none was specified.")
  set(CMAKE_BUILD_TYPE
      MinSizeRel
      CACHE STRING "Choose the type of build." FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel
                                             RelWithDebInfo)

option(WARN_WERROR "Halt the compilation with an error on compiler warnings."
       OFF)

include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)

include(cmake/Functions.cmake)
include(cmake/Checks.cmake)
include(cmake/Pedantic.cmake)
include(cmake/Debug.cmake)

string(REGEX REPLACE ".*/" "" COMPILER_NAME "${CMAKE_C_COMPILER}")

if(COMPILER_NAME MATCHES "em.*")
  set(EMSCRIPTEN TRUE)
  dump(
    EMSCRIPTEN
    EMCC_SUFFIX
    EMSCRIPTEN_PREFIX
    EMSCRIPTEN_TOOLCHAIN_PATH
    COMPILER_NAME
    CMAKE_EXECUTABLE_SUFFIX
    CMAKE_EXE_LINKER_FLAGS)
  set(EMSCRIPTEN_EXE_SUFFIX "html")
endif(COMPILER_NAME MATCHES "em.*")

option(BUILD_DEBUG "Build in debug mode" OFF)

if("${CMAKE_BUILD_TYPE}" MATCHES ".*Deb.*")
  set(BUILD_DEBUG TRUE)
endif("${CMAKE_BUILD_TYPE}" MATCHES ".*Deb.*")

if(BUILD_DEBUG)
  add_definitions(-D_DEBUG=1)
  set(ENABLE_DUMP
      TRUE
      CACHE BOOL "Enable `dump' builtin" FORCE)
else(BUILD_DEBUG)
  add_definitions(-DNDEBUG=1)
endif(BUILD_DEBUG)

if(BUILD_DEBUG)
  option(USE_EFENCE "Enable electric fence" OFF)

  check_library_exists(efence malloc /usr/lib HAVE_EFENCE)
endif(BUILD_DEBUG)

if(HAVE_EFENCE)
  if(USE_EFENCE)
    set(ELECTRICFENCE_LIBRARY efence)
  endif(USE_EFENCE)
endif(HAVE_EFENCE)

if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif(ENABLE_LTO)

option(LINK_STATIC "Link executables statically" OFF)

if(LINK_STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif(LINK_STATIC)

set(ENABLE_SHARED OFF)
set(BUILD_SHARED_LIBS FALSE)

check_c_compiler_flag("-flto" F_LTO)

if(F_LTO)
  option(ENABLE_LTO "Enable link-time optimization" OFF)
endif(F_LTO)

include(cmake/Builtins.cmake)

option(NO_TREE_PRINT "Disable tree_print for history" OFF)

option(DEBUG_ALLOC "Debug memory allocations" OFF)
option(DEBUG_COLOR "Color debug output" OFF)
option(DEBUG_OUTPUT "Print debug output" ${DEBUG_OUTPUT_DEFAULT})

if(DEBUG_OUTPUT)
  add_definitions(-DDEBUG_OUTPUT=1)
endif()
if(DEBUG_COLOR)
  add_definitions(-DCOLOR_DEBUG=1)
endif()

if(DEBUG_ALLOC)
  set(NAME_DEBUG "debug")
  add_definitions(-DDEBUG_ALLOC=1)
else()
  set(NAME_DEBUG "")
endif()

configure_file("${CMAKE_SOURCE_DIR}/config.h.cmake"
               "${CMAKE_BINARY_DIR}/config.h")

set(SOURCE_DEFINITIONS)
set(SOURCE_TYPES
    ATFILE
    GNU
    LARGEFILE
    LARGE_FILE
    LARGEFILE64
    POSIX
    POSIX_C
    XOPEN
    XOPEN_EXTENDED)
foreach(S ${SOURCE_TYPES})
  list(APPEND SOURCE_DEFINITIONS -D_${S}_SOURCE)
endforeach(S ${SOURCE_TYPES})

add_definitions(-DHAVE_CONFIG_H=1)
add_definitions(${SOURCE_DEFINITIONS})

add_definitions(-D_FILE_OFFSET_BITS=64)

if(BUILD_DEBUG)
  add_definitions(-D_DEBUG)
endif(BUILD_DEBUG)

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

# add_definitions(-DHAVE_CONFIG_H=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_MODULE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

file(GLOB CMAKE_SOURCES CMakeLists.txt cmake/*.cmake)

include(cmake/libowfat.cmake)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

 
file(GLOB LIBSHELL_SOURCES src/*.h src/*/*.c)

list(FILTER LIBSHELL_SOURCES EXCLUDE REGEX
     "(sh_fmt\\.c|sh_parse2ast\\.c|sh_init\\.c|sh_main\\.c)")

# string(REGEX REPLACE ";" "\n" SHELL_SOURCES "${LIBSHELL_SOURCES}")
# message("LIBSHELL_SOURCES: ${SHELL_SOURCES}")

if(DEBUG_ALLOC)
  list(
    APPEND
    LIBSHELL_SOURCES
    src/debug/debug_alloc.c
    src/debug/debug_free.c
    src/debug/debug_realloc.c
    src/debug/debug_error.c
    src/debug/debug_memory.c)
endif(DEBUG_ALLOC)
if(DEBUG_OUTPUT)
  list(
    APPEND
    LIBSHELL_SOURCES
    src/debug/debug_char.c
    src/debug/debug_error.c
    # src/debug/debug_node.c
    src/debug/debug_ptr.c
    src/debug/debug_space.c
    src/debug/debug_subst.c
    src/debug/debug_unquoted.c)
endif(DEBUG_OUTPUT)

list(APPEND BUILTIN_SOURCES src/builtin/builtin_error.c
     src/builtin/builtin_search.c src/builtin/builtin_table.c)
list(APPEND LIBSHELL_SOURCES src/parse/parse_newnode${NAME_DEBUG}.c
     src/tree/tree_newnode${NAME_DEBUG}.c)

if(FORK_SOURCE)
  list(APPEND LIBSHELL_SOURCES ${FORK_SOURCE})
endif(FORK_SOURCE)

string(REPLACE " " ";" LIBSHELL_SOURCES "${LIBSHELL_SOURCES}")

if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
  list(REMOVE_ITEM LIBSHELL_SOURCES "src/tree/tree_print.c")
  list(REMOVE_ITEM LIBSHELL_SOURCES "src/tree/tree_printlist.c")
endif(NO_TREE_PRINT)

# configure_file("${CMAKE_SOURCE_DIR}/src/builtin_config.h.cmake"
# "${CMAKE_BINARY_DIR}/src/builtin_config.h" #@ONLY

list(APPEND LIBSHELL_SOURCES "${CMAKE_BINARY_DIR}/src/builtin_config.h")

list(APPEND LIBSHELL_SOURCES ${BUILTIN_SOURCES})
list(APPEND ALL_SOURCES ${LIBSHELL_SOURCES})

add_library(libshell STATIC ${LIBSHELL_SOURCES})
set_target_properties(libshell PROPERTIES OUTPUT_NAME shell)

set(SHISH_SOURCES src/sh/sh_init.c src/sh/sh_main.c)
list(APPEND ALL_SOURCES ${SHISH_SOURCES})

add_executable(shish ${SHISH_SOURCES})
target_link_libraries(shish PRIVATE libshell ${LIBOWFAT_LIBRARY}
                                    ${MATH_LIBRARY} ${ELECTRICFENCE_LIBRARY})
if(EMSCRIPTEN)
  set_target_properties(shish PROPERTIES SUFFIX ".${EMSCRIPTEN_EXE_SUFFIX}")
endif(EMSCRIPTEN)

option(BUILD_SHFORMAT "Build shformat program" ON)

if(BUILD_SHFORMAT)
  file(GLOB SHFORMAT_SOURCES src/*.h src/debug/*.c src/sh/sh_fmt.c)

  list(FILTER SHFORMAT_SOURCES EXCLUDE REGEX "dump")
  list(APPEND ALL_SOURCES ${SHFORMAT_SOURCES})

  add_executable(shformat ${SHFORMAT_SOURCES})
  target_link_libraries(
    shformat PRIVATE libshell ${LIBOWFAT_LIBRARY} ${MATH_LIBRARY}
                     ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shformat PUBLIC SHFORMAT=1)
  install(TARGETS shformat DESTINATION bin)

endif(BUILD_SHFORMAT)

option(BUILD_SHPARSE2AST "Build shformat program" OFF)

if(BUILD_SHPARSE2AST)
  file(GLOB SHPARSE2AST_SOURCES src/*.h src/debug/*.c src/sh/sh_parse2ast.c)

  list(FILTER SHPARSE2AST_SOURCES EXCLUDE REGEX "dump")
  list(APPEND ALL_SOURCES ${SHPARSE2AST_SOURCES})

  add_executable(shparse2ast ${SHPARSE2AST_SOURCES})
  target_link_libraries(
    shparse2ast PRIVATE libshell ${LIBOWFAT_LIBRARY} ${MATH_LIBRARY}
                        ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shparse2ast PUBLIC SHPARSE2AST=1 DEBUG_OUTPUT=1)
  install(TARGETS shparse2ast DESTINATION bin)

endif(BUILD_SHPARSE2AST)

set_source_files_properties(src/builtin/builtin_table.c
                            PROPERTIES COMPILE_FLAGS "${BUILTIN_FLAGS}")

install(TARGETS shish DESTINATION bin)

add_custom_target(
  cmake-format
  COMMAND cmake-format --line-width 240 -i -l info CMakeLists.txt
          ${CMAKE_SOURCES}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Format CMake files"
  SOURCES ${CMAKE_SOURCES})

add_custom_target(
  clang-format
  COMMAND clang-format -style=file -i ${ALL_SOURCES}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Format *.c/*.h source files"
  SOURCES ${ALL_SOURCES})
