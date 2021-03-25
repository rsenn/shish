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

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

option(WARN_WERROR "Halt the compilation with an error on compiler warnings." OFF)

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

set(BUILD_DEBUG FALSE)

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

configure_file("${CMAKE_SOURCE_DIR}/config.h.cmake" "${CMAKE_BINARY_DIR}/config.h")

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

include_directories("${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}")

# add_definitions(-DHAVE_CONFIG_H=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_MODULE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

file(GLOB LIBSOURCES lib/*.h lib/*/*.c)

if(NOT HAVE_LIBOWFAT)
  file(
    GLOB
    ADDSOURCES
    lib/open/*.c
    lib/scan/*.c
    lib/buffer/*.c
    lib/fmt/*.c
    lib/byte/*.c
    lib/mmap/*.c
    lib/sig/*.c
    lib/str/*.c
    lib/wait/*.c
    lib/unix/*.c
    lib/utf8/*.c)
  set(LIBSOURCES
      ${LIBSOURCES}
      ${ADDSOURCES}
      lib/stralloc/stralloc_cats.c
      lib/stralloc/stralloc_diffs.c
      lib/stralloc/stralloc_zero.c
      lib/stralloc/stralloc_free${NAME_DEBUG}.c
      lib/stralloc/stralloc_catlong0.c
      lib/stralloc/stralloc_copyb.c
      lib/stralloc/stralloc_copys.c
      lib/stralloc/stralloc_copy.c
      lib/stralloc/stralloc_ready${NAME_DEBUG}.c
      lib/stralloc/stralloc_catulong0.c
      lib/stralloc/stralloc_cat.c
      lib/stralloc/stralloc_catb.c
      lib/stralloc/stralloc_init.c
      lib/stralloc/stralloc_readyplus${NAME_DEBUG}.c
      lib/str/str_diff.c
      lib/str/str_len.c
      lib/str/str_rchr.c
      lib/str/str_chr.c
      lib/str/str_copy.c)
endif(NOT HAVE_LIBOWFAT)

file(
  GLOB
  SOURCES
  src/*.h
  src/debug/debug_argv.c
  src/debug/debug_position.c
  src/eval/*.c
  src/exec/*.c
  src/expand/*.c
  src/expr/*.c
  src/fd/*.c
  src/fdstack/*.c
  src/fdtable/*.c
  src/history/*.c
  src/job/*.c
  src/parse/*.c
  src/prompt/*.c
  src/redir/*.c
  # src/sh/*.c
  src/sig/*.c
  src/source/*.c
  src/term/*.c
  src/tree/*.c
  src/var/*.c
  src/vartab/*.c)

if(DEBUG_ALLOC)
  list(
    APPEND
    SOURCES
    src/debug/debug_alloc.c
    src/debug/debug_free.c
    src/debug/debug_realloc.c
    src/debug/debug_error.c
    src/debug/debug_memory.c)
endif(DEBUG_ALLOC)
if(DEBUG_OUTPUT)
  list(
    APPEND
    SOURCES
    src/debug/debug_begin.c
    src/debug/debug_char.c
    src/debug/debug_end.c
    src/debug/debug_error.c
    src/debug/debug_list.c
    src/debug/debug_node.c
    src/debug/debug_ptr.c
    src/debug/debug_redir.c
    src/debug/debug_space.c
    src/debug/debug_stralloc.c
    src/debug/debug_str.c
    src/debug/debug_sublist.c
    src/debug/debug_subnode.c
    src/debug/debug_subst.c
    src/debug/debug_ulong.c
    src/debug/debug_unquoted.c)
endif(DEBUG_OUTPUT)

list(APPEND BUILTIN_SOURCES src/builtin/builtin_error.c src/builtin/builtin_search.c src/builtin/builtin_table.c)
list(APPEND SOURCES src/parse/parse_newnode${NAME_DEBUG}.c src/tree/tree_newnode${NAME_DEBUG}.c)

if(FORK_SOURCE)
  list(APPEND SOURCES ${FORK_SOURCE})
endif(FORK_SOURCE)

if(BUILD_DEBUG)
  option(USE_EFENCE "Enable electric fence" OFF)

  check_library_exists(efence malloc /usr/lib HAVE_EFENCE)
endif(BUILD_DEBUG)

if(HAVE_EFENCE)
  if(USE_EFENCE)
    set(ELECTRICFENCE_LIBRARY efence)
  endif(USE_EFENCE)
endif(HAVE_EFENCE)

string(REPLACE " " ";" SOURCES "${SOURCES}")

if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
  list(REMOVE_ITEM SOURCES "src/tree/tree_print.c")
  list(REMOVE_ITEM SOURCES "src/tree/tree_printlist.c")
endif(NO_TREE_PRINT)

#
# configure_file("${CMAKE_SOURCE_DIR}/src/builtin_config.h.cmake" "${CMAKE_BINARY_DIR}/src/builtin_config.h" #@ONLY

list(APPEND SOURCES "${CMAKE_BINARY_DIR}/src/builtin_config.h")

if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif(ENABLE_LTO)

option(LINK_STATIC "Link executables statically" OFF)

if(LINK_STATIC)
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif(LINK_STATIC)

list(APPEND SOURCES ${BUILTIN_SOURCES})

set(SHISH_SOURCES
    ${SOURCES}
    src/sh/sh_cmdlist.c
    src/sh/sh_error.c
    src/sh/sh_errorn.c
    src/sh/sh_exit.c
    src/sh/sh_forked.c
    src/sh/sh_getcwd.c
    src/sh/sh_gethome.c
    src/sh/sh_init.c
    src/sh/sh_loop.c
    src/sh/sh_main.c
    src/sh/sh_msg.c
    src/sh/sh_popargs.c
    src/sh/sh_pop.c
    src/sh/sh_pushargs.c
    src/sh/sh_push.c
    src/sh/sh_root.c
    src/sh/sh_setargs.c
    src/sh/sh_subshell.c
    src/sh/sh_usage.c)

add_executable(shish ${SHISH_SOURCES} ${LIBSOURCES})
target_link_libraries(shish ${LIBOWFAT_LIBRARY} ${MATH_LIBRARY} ${ELECTRICFENCE_LIBRARY})
if(EMSCRIPTEN)
  set_target_properties(shish PROPERTIES SUFFIX ".${EMSCRIPTEN_EXE_SUFFIX}")
endif(EMSCRIPTEN)

option(BUILD_SHFORMAT "Build shformat program" ON)

if(BUILD_SHFORMAT)
  file(
    GLOB
    SHFORMAT_SOURCES
    src/debug/*.c
    src/redir/redir_addhere.c
    src/redir/redir_dup.c
    src/redir/redir_here.c
    src/redir/redir_open.c
    src/redir/redir_parse.c
    src/parse/*.c
    src/tree/*.c
    src/*.h
    src/source/*.c
    src/fdstack/*.c
    src/fdtable/*.c
    src/fd/*.c
    src/var/*.c
    src/vartab/*.c
    src/varstack/*.c
    src/sh/sh_root.c
    src/sh/sh_error.c
    src/sh/sh_errorn.c
    src/sh/sh_exit.c
    src/sh/sh_fmt.c
    src/sh/sh_msg.c
    src/sh/sh_usage.c)

  list(FILTER SHFORMAT_SOURCES EXCLUDE REGEX "dump")

  add_executable(shformat ${SHFORMAT_SOURCES} ${LIBSOURCES})
  target_link_libraries(shformat ${LIBOWFAT_LIBRARY} ${MATH_LIBRARY} ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shformat PUBLIC SHFORMAT=1)
  install(TARGETS shformat DESTINATION bin)

endif(BUILD_SHFORMAT)

option(BUILD_SHPARSE2AST "Build shformat program" ON)

if(BUILD_SHPARSE2AST)
  file(
    GLOB
    SHPARSE2AST_SOURCES
    src/debug/*.c
    src/redir/redir_addhere.c
    src/redir/redir_dup.c
    src/redir/redir_here.c
    src/redir/redir_open.c
    src/redir/redir_parse.c
    src/parse/*.c
    src/tree/*.c
    src/*.h
    src/source/*.c
    src/fdstack/*.c
    src/fdtable/*.c
    src/fd/*.c
    src/var/*.c
    src/vartab/*.c
    src/varstack/*.c
    src/sh/sh_root.c
    src/sh/sh_error.c
    src/sh/sh_errorn.c
    src/sh/sh_exit.c
    src/sh/sh_parse2ast.c
    src/sh/sh_msg.c
    src/sh/sh_usage.c)

  list(FILTER SHPARSE2AST_SOURCES EXCLUDE REGEX "dump")

  add_executable(shparse2ast ${SHPARSE2AST_SOURCES} ${LIBSOURCES})
  target_link_libraries(shparse2ast ${LIBOWFAT_LIBRARY} ${MATH_LIBRARY} ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shparse2ast PUBLIC SHPARSE2AST=1 DEBUG_OUTPUT=1)
  install(TARGETS shparse2ast DESTINATION bin)

endif(BUILD_SHPARSE2AST)

set_source_files_properties(src/builtin/builtin_table.c PROPERTIES COMPILE_FLAGS "${BUILTIN_FLAGS}")

install(TARGETS shish DESTINATION bin)
