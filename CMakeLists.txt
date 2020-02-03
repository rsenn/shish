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

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCCompilerFlag)


set(BUILD_DEBUG FALSE)

if("${CMAKE_BUILD_TYPE}" MATCHES ".*Deb.*")
   set(BUILD_DEBUG TRUE)
endif("${CMAKE_BUILD_TYPE}"  MATCHES ".*Deb.*")


if(BUILD_DEBUG)
  add_definitions(-DDEBUG=1)
else(BUILD_DEBUG)
  add_definitions(-DNDEBUG=1)
endif(BUILD_DEBUG)




set(ENABLE_SHARED OFF)
set(BUILD_SHARED_LIBS FALSE)

check_c_compiler_flag("-flto" F_LTO)

if(F_LTO)
  option(ENABLE_LTO "Enable link-time optimization" OFF)

endif(F_LTO)



if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
check_c_compiler_flag("-falign-functions" F_ALIGN_FUNCTIONS)
check_c_compiler_flag("-falign-jumps" F_ALIGN_JUMPS)
check_c_compiler_flag("-falign-labels" F_ALIGN_LABELS)
check_c_compiler_flag("-falign-loops" F_ALIGN_LOOPS)
endif()


if(F_ALIGN_COMMONS)
  add_definitions("-falign-commons")
endif(F_ALIGN_COMMONS)
if(F_ALIGN_DOUBLE)
  add_definitions("-falign-double")
endif(F_ALIGN_DOUBLE)
if(F_ALIGN_FUNCTIONS)
  add_definitions("-falign-functions")
endif(F_ALIGN_FUNCTIONS)
if(F_ALIGN_JUMPS)
  add_definitions("-falign-jumps")
endif(F_ALIGN_JUMPS)
if(F_ALIGN_LABELS)
  add_definitions("-falign-labels")
endif(F_ALIGN_LABELS)
if(F_ALIGN_LOOPS)
  add_definitions("-falign-loops")
endif(F_ALIGN_LOOPS)
if(F_ALIGN_STRINGOPS)
  add_definitions("-falign-stringops")
endif(F_ALIGN_STRINGOPS)

list(APPEND ALL_BUILTINS basename break cd dirname dump echo eval exec exit export expr false fdtable hash help history hostname pwd set shift source test type true unset)
list(APPEND MINIMAL_BUILTINS break cd eval exec exit export hash history pwd set shift source unset)
list(APPEND DEFAULT_BUILTINS ${MINIMAL_BUILTINS} type echo fdtable true false dump)

function(isin)
  set(ARGUMENTS "${ARGV}")
  list(GET ARGUMENTS 0 RETVAR)
  list(GET ARGUMENTS 1 ITEM)
  list(REMOVE_AT ARGUMENTS 0 1)
  #message("ARGUMENTS: ${ARGUMENTS}")
  list(FIND ARGUMENTS "${ITEM}" FOUND)

  if(${FOUND} EQUAL -1)
    set(RET FALSE)
  else()
   set(RET TRUE)
  endif()
  set("${RETVAR}" ${RET} PARENT_SCOPE)
  #return(${RET})
endfunction(isin)

set(BUILTINS_ENABLED "")
set(BUILTINS_DISABLED "")

foreach(BUILTIN ${ALL_BUILTINS})
	string(TOUPPER ${BUILTIN} NAME)
	if(NOT DEFINED ${ENABLE_${NAME}})
		isin("ENABLE_${NAME}" ${BUILTIN} ${DEFAULT_BUILTINS} )
		option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" "${ENABLE_${NAME}}")
	else(NOT DEFINED ${ENABLE_${NAME}})
		option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" OFF)
	endif(NOT DEFINED ${ENABLE_${NAME}})
	#message(STATUS "Builtin ${BUILTIN} - ${ENABLE_${NAME}}")
endforeach(BUILTIN ${ALL_BUILTINS})

option(NO_TREE_PRINT "Disable tree_print for history" OFF)
option(DEBUG_ALLOC "Debug memory allocations" OFF)
option(DEBUG_COLOR "Color debug output" OFF)
option(DEBUG_OUTPUT "Print debug output" OFF)

if(DEBUG_OUTPUT)
  add_definitions(-DDEBUG_OUTPUT=1)
endif()
if(DEBUG_COLOR)
  add_definitions(-DCOLOR_DEBUG=1)
endif()

foreach(BUILTIN ${ALL_BUILTINS})

string(TOUPPER ${BUILTIN} NAME)
	if(${BUILTIN_${NAME}} STREQUAL ON)
		list(APPEND BUILTINS_ENABLED ${BUILTIN})
	else(${BUILTIN_${NAME}} STREQUAL ON)
		list(APPEND BUILTINS_DISABLED ${BUILTIN})
	endif(${BUILTIN_${NAME}} STREQUAL ON)
endforeach(BUILTIN ${ALL_BUILTINS})

if(DEBUG_ALLOC)
  set(NAME_DEBUG "debug")
  add_definitions(-DDEBUG_ALLOC=1)
else()
  set(NAME_DEBUG "")
endif()

if(WIN32 OR WIN64)
  check_library_exists(ws2_32 gethostname /usr/lib HAVE_WS2_32)
endif(WIN32 OR WIN64)

if(HAVE_WS2_32)
  set(WINSOCK2_LIBRARY ws2_32)
endif(HAVE_WS2_32)

check_library_exists(owfat_debug buffer_init /usr/lib64 HAVE_LIBOWFAT)

if(HAVE_LIBOWFAT)
set(LIBOWFAT_LIBRARY owfat_debug)
endif(HAVE_LIBOWFAT)


if(NOT HAVE_LIBOWFAT)
check_library_exists(owfat buffer_init /usr/lib64 HAVE_LIBOWFAT)
if(HAVE_LIBOWFAT)
set(LIBOWFAT_LIBRARY owfat)
endif(HAVE_LIBOWFAT)
endif(NOT HAVE_LIBOWFAT)



check_include_file(alloca.h HAVE_ALLOCA_H)

if(HAVE_ALLOCA_H)
check_symbol_exists(alloca alloca.h HAVE_ALLOCA)
endif()

check_include_file(sys/wait.h HAVE_SYS_WAIT_H)

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} unistd.h)
endif(HAVE_UNISTD_H)

check_function_exists(sethostname HAVE_SETHOSTNAME)
check_function_exists(readlink HAVE_READLINK)


check_include_file(fcntl.h HAVE_FCNTL_H)
if(HAVE_FCNTL_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} fcntl.h)
endif(HAVE_FCNTL_H)

check_function_exists(fcntl HAVE_FCNTL)

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
if(HAVE_SYS_TYPES_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/types.h)
endif(HAVE_SYS_TYPES_H)

check_include_file(sys/stat.h HAVE_SYS_STAT_H)
if(HAVE_SYS_STAT_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/stat.h)
endif(HAVE_SYS_STAT_H)

check_function_exists(lstat HAVE_LSTAT)

check_include_file(signal.h HAVE_SIGNAL_H)
if(HAVE_SIGNAL_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} signal.h)
endif(HAVE_SIGNAL_H)

#check_type_size(sigset_t SIGSET_T)

check_include_file(glob.h HAVE_GLOB_H)
if(HAVE_GLOB_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} glob.h)
endif(HAVE_GLOB_H)

check_function_exists(glob HAVE_GLOB)


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

#add_definitions(-Wall   -unused-variable -Wno-unuset-but-set-variable -DHAVE_CONFIG_H=1 -D__USE_XOPEN_EXTENDED=1 -D__USE_POSIX=1)
#add_definitions(-w   -DHAVE_CONFIG_H=1 -D__USE_XOPEN_EXTENDED=1 -D__USE_POSIX=1)

add_definitions(-DHAVE_CONFIG_H=1 -D__USE_XOPEN_EXTENDED=1 -D__USE_POSIX=1)

add_definitions(-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
add_definitions(-D_GNU_SOURCE)

if(BUILD_DEBUG)
  set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0")
else(BUILD_DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
endif(BUILD_DEBUG)

string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

if(BUILD_DEBUG)
  add_definitions(-DDEBUG=1)
endif(BUILD_DEBUG)

include_directories("${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/lib" "${CMAKE_SOURCE_DIR}/src")

#add_definitions(-DHAVE_CONFIG_H=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_MODULE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

file(GLOB LIBSOURCES lib/*.h lib/*/*.c)

if(NOT HAVE_LIBOWFAT)
  file(GLOB ADDSOURCES lib/open/*.c lib/scan/*.c lib/buffer/*.c lib/fmt/*.c lib/byte/*.c lib/mmap/*.c lib/str/*.c)
  set(LIBSOURCES ${LIBSOURCES} ${ADDSOURCES} lib/stralloc/stralloc_cats.c lib/stralloc/stralloc_diffs.c lib/stralloc/stralloc_zero.c lib/stralloc/stralloc_free${NAME_DEBUG}.c lib/stralloc/stralloc_catlong0.c lib/stralloc/stralloc_copyb.c lib/stralloc/stralloc_copys.c lib/stralloc/stralloc_copy.c lib/stralloc/stralloc_ready${NAME_DEBUG}.c lib/stralloc/stralloc_catulong0.c lib/stralloc/stralloc_cat.c lib/stralloc/stralloc_catb.c lib/stralloc/stralloc_init.c lib/stralloc/stralloc_readyplus${NAME_DEBUG}.c lib/str/str_diff.c lib/str/str_len.c lib/str/str_rchr.c lib/str/str_chr.c lib/str/str_copy.c)
endif(NOT HAVE_LIBOWFAT)

add_library(libshish ${LIBSOURCES})
set_target_properties(libshish PROPERTIES
  LIBRARY_OUTPUT_NAME "shish"
  OUTPUT_NAME "shish"
  PREFIX "lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#[[  COMPILE_FLAGS "${F_LTO}"
#  LINK_FLAGS "${F_LTO}"
#  ]]
)
#if(F_LTO)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
#set(LINK_FLAGS "${LINK_FLAGS} -flto")
#endif(F_LTO)
##[[
# ndif(F_LTO)
# ]]

file(GLOB SOURCES src/*.h src/debug/*.c src/eval/*.c src/exec/*.c src/expand/*.c src/expr/*.c src/fd/*.c src/fdstack/*.c src/fdtable/*.c src/history/*.c src/job/*.c src/parse/*.c src/prompt/*.c src/redir/*.c src/sh/*.c src/sig/*.c src/source/*.c src/term/*.c src/tree/*.c src/var/*.c src/vartab/*.c)


if(BUILD_DEBUG)
  file(GLOB ADDSOURCES src/debug/*.c)
  set(SOURCES "${SOURCES};${ADDDSOURCES}")
  set(DEBUG_SOURCES "${SOURCES}")
  list(FILTER DEBUG_SOURCES INCLUDE REGEX ".*debug\.c")
  foreach(SRC ${DEBUG_SOURCES})
    string(REGEX REPLACE "debug\.c" ".c" OSRC "${SRC}")
    message("Remove ${OSRC}")
    list(REMOVE_ITEM SOURCES "${OSRC}")
  endforeach(SRC ${DEBUG_SOURCES})
else(BUILD_DEBUG)
  list(FILTER SOURCES EXCLUDE REGEX ".*debug\.c")
endif(BUILD_DEBUG)



list(APPEND SOURCES src/builtin/builtin_error.c src/builtin/builtin_search.c src/builtin/builtin_table.c)
list(APPEND SOURCES src/parse/parse_newnode${NAME_DEBUG}.c src/tree/tree_newnode${NAME_DEBUG}.c)

set(MATH_LIBRARY m)

foreach(BUILTIN ${BUILTINS_ENABLED})
  list(APPEND SOURCES src/builtin/builtin_${BUILTIN}.c)
endforeach(BUILTIN ${BUILTINS_ENABLED})

if(BUILD_DEBUG)

 check_library_exists(efence malloc /usr/lib HAVE_EFENCE)
   if(HAVE_EFENCE)
   set(ELECTRICFENCE_LIBRARY efence)
   endif(HAVE_EFENCE)
endif(BUILD_DEBUG)

string(REPLACE " " ";" SOURCES "${SOURCES}")


foreach(DISABLED ${BUILTINS_DISABLED})
  set(SRC "src/builtin/builtin_${DISABLED}.c")
  list(LENGTH SOURCES N)
  list(REMOVE_ITEM SOURCES "${SRC}")
  list(FILTER SOURCES EXCLUDE REGEX  "${SRC}")
  list(LENGTH SOURCES N2)
#  if(${N} EQUAL ${N2})
#   message("ERROR ${SRC}  -  ${N} == ${N2}")
#  endif(${N} EQUAL ${N2})
  unset(BUILTIN_${NAME})
  set(BUILD_BUILTIN_${NAME} "0" CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(DISABLED ${BUILTINS_DISABLED})

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  #add_definitions("-DBUILTIN_${NAME}=1")
  set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
  set(BUILD_BUILTIN_${NAME} 1 CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(ENABLED ${BUILTINS_ENABLED})


if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
  list(FILTER SOURCES EXCLUDE REGEX  "src/tree/tree.*print.*\.c")
endif(NO_TREE_PRINT)


configure_file("${CMAKE_SOURCE_DIR}/src/builtin_config.h.cmake" "${CMAKE_BINARY_DIR}/src/builtin_config.h"
	#@ONLY
	#ESCAPE_QUOTES
	NEWLINE_STYLE UNIX)

list(APPEND SOURCES "${CMAKE_BINARY_DIR}/src/builtin_config.h")


if(ENABLE_LTO)
  add_compile_options("-flto")
  add_link_options("-flto")
endif(ENABLE_LTO)


add_executable(shish ${SOURCES})
target_link_libraries(shish libshish
  ${LIBOWFAT_LIBRARY}
  ${MATH_LIBRARY}
  ${ELECTRICFENCE_LIBRARY}
  ${WINSOCK2_LIBRARY}
)


#message(STATUS "\nBuiltin flags:\n  ${BUILTIN_FLAGS}\n")

set_source_files_properties( src/builtin/builtin_table.c PROPERTIES COMPILE_FLAGS "${BUILTIN_FLAGS}" )

install(TARGETS shish DESTINATION bin)


list(SORT BUILTINS_ENABLED)
list(SORT BUILTINS_DISABLED)

string(REPLACE ";" " " BUILTINS_ENABLED "${BUILTINS_ENABLED}")
string(REPLACE ";" " " BUILTINS_DISABLED "${BUILTINS_DISABLED}")

set(OUTSTR "")
set(OUTSTR "${OUTSTR}\nEnabled builtins:\n  ${BUILTINS_ENABLED}\n")
set(OUTSTR "${OUTSTR}\nDisabled builtins:\n  ${BUILTINS_DISABLED}\n")
message(STATUS "${OUTSTR}")
