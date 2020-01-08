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

set(ENABLE_SHARED OFF)
set(BUILD_SHARED_LIBS FALSE)

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  check_c_compiler_flag("-flto" F_LTO)
  check_c_compiler_flag("-falign-functions" F_ALIGN_FUNCTIONS)
  check_c_compiler_flag("-falign-jumps" F_ALIGN_JUMPS)
  check_c_compiler_flag("-falign-labels" F_ALIGN_LABELS)
  check_c_compiler_flag("-falign-loops" F_ALIGN_LOOPS)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(DEBUG ON)
else()
  set(DEBUG OFF)

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

list(APPEND ALL_BUILTINS basename break cd dirname dump echo eval exec exit export false fdtable hash history hostname pwd set shift source test true unset)
list(APPEND MINIMAL_BUILTINS break cd eval exec exit export hash history pwd set shift source unset)
list(APPEND DEFAULT_BUILTINS ${MINIMAL_BUILTINS} echo fdtable true false dump)

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
  SET("${RETVAR}" ${RET} PARENT_SCOPE)
  #return(${RET})
endfunction(isin)

set(BUILTINS_ENABLED "") 
set(BUILTINS_DISABLED "") 

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  if(NOT DEFINED ${ENABLE_${NAME}})

    isin("ENABLE_${NAME}" ${BUILTIN} ${DEFAULT_BUILTINS} )


 
    option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" "${ENABLE_${NAME}}")
  endif(NOT DEFINED ${ENABLE_${NAME}})

  #message(STATUS "Builtin ${BUILTIN} - ${ENABLE_${NAME}}")
endforeach(BUILTIN ${ALL_BUILTINS})

option(NO_TREE_PRINT "Disable tree_print for history" OFF)
option(DEBUG_COLOR "Color debug output" OFF)
option(DEBUG_OUTPUT "Print debug output" OFF)

if(DEBUG_OUTPUT)
    add_definitions(-DDEBUG=1)
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




if("${CMAKE_BUILD_TYPE}"  STREQUAL "Debug")
  set(DEBUG "debug")
  add_definitions(-DDEBUG=1)
else()
  set(DEBUG "")
  add_definitions(-DNDEBUG=1)
endif()

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

check_type_size(sigset_t SIGSET_T)

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

add_definitions(-Wall -DHAVE_CONFIG_H=1 -D__USE_XOPEN_EXTENDED=1 -D__USE_POSIX=1)

add_definitions(-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
add_definitions(-D_GNU_SOURCE)

set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")


include_directories("${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/lib" "${CMAKE_SOURCE_DIR}/src")

#add_definitions(-DHAVE_CONFIG_H=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_MODULE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

file(GLOB LIBSOURCES lib/*.h lib/*/*.c)

if(NOT HAVE_LIBOWFAT) 
	set(LIBSOURCES ${LIBSOURCES} lib/open/open_read.c lib/open/open_trunc.c lib/scan/scan_uint.c lib/scan/scan_8longn.c lib/scan/scan_ulongn.c lib/scan/scan_8long.c lib/scan/scan_ulong.c lib/scan/scan_long.c lib/scan/scan_longlong.c lib/scan/scan_ulonglong.c lib/scan/scan_int.c lib/buffer/buffer_munmap.c lib/buffer/buffer_frombuf.c lib/buffer/buffer_putsa.c lib/buffer/buffer_mmapread.c lib/buffer/buffer_putsflush.c lib/buffer/buffer_putulong.c lib/buffer/buffer_getc.c lib/buffer/buffer_fromsa.c lib/buffer/buffer_flush.c lib/buffer/buffer_free.c lib/buffer/buffer_stubborn.c lib/buffer/buffer_putnlflush.c lib/buffer/buffer_stubborn2.c lib/buffer/buffer_putm_internal.c lib/buffer/buffer_get.c lib/buffer/buffer_put.c lib/buffer/buffer_tosa.c lib/buffer/buffer_puts.c lib/buffer/buffer_close.c lib/buffer/buffer_putflush.c lib/buffer/buffer_putspace.c lib/buffer/buffer_init.c lib/buffer/buffer_feed.c lib/fmt/fmt_xlonglong.c lib/fmt/fmt_long.c lib/fmt/fmt_xlong.c lib/fmt/fmt_ulong0.c lib/fmt/fmt_ulong.c lib/fmt/fmt_minus.c lib/byte/byte_zero.c lib/byte/byte_diff.c lib/byte/byte_copyr.c lib/byte/byte_copy.c lib/byte/byte_rchr.c lib/byte/byte_chr.c lib/mmap/mmap_private.c lib/mmap/mmap_read.c lib/mmap/mmap_unmap.c lib/stralloc/stralloc_cats.c lib/stralloc/stralloc_diffs.c lib/stralloc/stralloc_zero.c lib/stralloc/stralloc_free${DEBUG}.c lib/stralloc/stralloc_catlong0.c lib/stralloc/stralloc_copyb.c lib/stralloc/stralloc_copys.c lib/stralloc/stralloc_copy.c lib/stralloc/stralloc_ready${DEBUG}.c lib/stralloc/stralloc_catulong0.c lib/stralloc/stralloc_cat.c lib/stralloc/stralloc_catb.c lib/stralloc/stralloc_init.c lib/stralloc/stralloc_readyplus${DEBUG}.c lib/str/str_diff.c lib/str/str_len.c lib/str/str_rchr.c lib/str/str_chr.c lib/str/str_copy.c)
endif(NOT HAVE_LIBOWFAT) 

add_library(libshish ${LIBSOURCES})
set_target_properties(libshish PROPERTIES
	LIBRARY_OUTPUT_NAME "shish"
	OUTPUT_NAME "shish"
	PREFIX "lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )


file(GLOB SOURCES src/*.h src/*/*.c)
set(MATH_LIBRARY m)



if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(ELECTRICFENCE_LIBRARY efence)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")

string(REPLACE " " ";" SOURCES "${SOURCES}")


foreach(DISABLED ${BUILTINS_DISABLED})
  set(SRC "src/builtin/builtin_${DISABLED}.c")
  list(LENGTH SOURCES N)
  list(REMOVE_ITEM SOURCES "${SRC}")
  list(FILTER SOURCES EXCLUDE REGEX  "${SRC}")
  list(LENGTH SOURCES N2)
  if(${N} EQUAL ${N2})
    message("ERROR ${SRC}  -  ${N} == ${N2}")
  endif(${N} EQUAL ${N2})
endforeach(DISABLED ${BUILTINS_DISABLED})

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  #add_definitions("-DBUILTIN_${NAME}=1")
  set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
endforeach(ENABLED ${BUILTINS_ENABLED})


if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
 list(FILTER SOURCES EXCLUDE REGEX  "src/tree/tree.*print.*\.c")
endif(NO_TREE_PRINT)

add_executable(shish ${SOURCES})
target_link_libraries(shish libshish 
  ${LIBOWFAT_LIBRARY}
  ${MATH_LIBRARY}
  # ${ELECTRICFENCE_LIBRARY}
  )

message(STATUS "\nBuiltin flags:\n  ${BUILTIN_FLAGS}\n")

SET_SOURCE_FILES_PROPERTIES( src/builtin/builtin_table.c PROPERTIES COMPILE_FLAGS "${BUILTIN_FLAGS}" )


install(TARGETS shish DESTINATION bin)


string(REPLACE ";" "\n  " BUILTINS_ENABLED "${BUILTINS_ENABLED}")
string(REPLACE ";" "\n  " BUILTINS_DISABLED "${BUILTINS_DISABLED}")

message(STATUS "\nEnabled builtins:\n  ${BUILTINS_ENABLED}\n")
message(STATUS "\nDisabled builtins:\n  ${BUILTINS_DISABLED}\n")

