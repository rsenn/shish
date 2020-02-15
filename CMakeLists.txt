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
include(CheckCSourceCompiles)

option(WARN_WERROR "Halt the compilation with an error on compiler warnings." OFF)

include(CheckCCompilerFlag)

check_c_compiler_flag("-Wall" WARN_ALL)
if(WARN_ALL)
    set(WERROR_FLAG "${WERROR_FLAG} -Wall")
endif()

check_c_compiler_flag("-Wno-unused-variable" WARN_NO_UNUSED_VARIABLE)
if(WARN_NO_UNUSED_VARIABLE)
    set(WERROR_FLAG "${WERROR_FLAG} -Wno-unused-variable")
endif()
check_c_compiler_flag("-Wno-unused-function" WARN_NO_UNUSED_FUNCTION)
if(WARN_NO_UNUSED_FUNCTION)
    set(WERROR_FLAG "${WERROR_FLAG} -Wno-unused-function")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(PEDANTIC_COMPILE_FLAGS -pedantic-errors -Wall -Wextra -pedantic
      -Wold-style-cast -Wundef
      -Wredundant-decls -Wwrite-strings -Wpointer-arith
      -Wcast-qual -Wformat=2 -Wmissing-include-dirs
      -Wcast-align -Wnon-virtual-dtor
      -Wctor-dtor-privacy -Wdisabled-optimization
      -Winvalid-pch -Woverloaded-virtual
      -Wconversion
      -Wno-ctor-dtor-privacy -Wno-format-nonliteral -Wno-shadow)
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.6)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wnoexcept
         -Wno-dangling-else -Wno-unused-local-typedefs)
  endif()
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wdouble-promotion
          -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast
          -Wvector-operation-performance -Wsized-deallocation)
  endif()
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 6.0)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2
          -Wnull-dereference -Wduplicated-cond)
  endif()
  set(WERROR_FLAG "${WERROR_FLAG} -Werror)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -pedantic -Wconversion -Wno-sign-conversion)
  set(WERROR_FLAG "${WERROR_FLAG} -Werror)
endif()
if(MSVC)
  set(PEDANTIC_COMPILE_FLAGS /W3)
  set(WERROR_FLAG /WX)
endif()
if(WARN_WERROR)
  SET(WARN_C_COMPILER_FLAGS "-Wall ${WERROR_FLAG}")
endif()
if(WARN_PEDANTIC)
  SET(WARN_C_COMPILER_FLAGS "-Wall ${PEDANTIC_COMPILE_FLAGS}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_C_COMPILER_FLAGS}")


function(DUMP VAR)
  message("\n\nVariable dump of: " ${ARGV} "\n")
  foreach(VAR ${ARGV})
  message("\t${VAR} = ${${VAR}}")
  endforeach(VAR ${ARGV})
  message("\n")
endfunction(DUMP VAR)
string(REGEX REPLACE ".*/" "" COMPILER_NAME "${CMAKE_C_COMPILER}")

if(COMPILER_NAME MATCHES "em.*")
  set(EMSCRIPTEN TRUE)
  DUMP(EMSCRIPTEN EMCC_SUFFIX EMSCRIPTEN_PREFIX EMSCRIPTEN_TOOLCHAIN_PATH COMPILER_NAME CMAKE_EXECUTABLE_SUFFIX CMAKE_EXE_LINKER_FLAGS)
  set(EMSCRIPTEN_EXE_SUFFIX "html")
endif(COMPILER_NAME MATCHES "em.*")

macro(check_inline)
	foreach(KEYWORD "__inline__" "__inline" "inline")
		if(NOT INLINE_KEYWORD)
			set(CMAKE_REQUIRED_DEFINITIONS "-DTESTKEYWORD=${KEYWORD}")
			check_c_source_compiles("typedef int foo_t;
	static TESTKEYWORD foo_t static_foo(){return 0;}
	foo_t foo(){return 0;}
	int main(int argc, char *argv[]){return 0;}" HAVE_${KEYWORD})
			if(HAVE_${KEYWORD})
				set(INLINE_KEYWORD "${KEYWORD}")
			endif(HAVE_${KEYWORD})
		endif(NOT INLINE_KEYWORD)
	endforeach(KEYWORD)
  message(STATUS "inline keyword: ${INLINE_KEYWORD}")
endmacro(check_inline)


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
check_c_compiler_flag("-falign-functions=1" F_ALIGN_FUNCTIONS)
check_c_compiler_flag("-falign-jumps=1" F_ALIGN_JUMPS)
check_c_compiler_flag("-falign-labels=1" F_ALIGN_LABELS)
check_c_compiler_flag("-falign-loops=1" F_ALIGN_LOOPS)
endif()


if(F_ALIGN_COMMONS)
  add_definitions("-falign-commons=1")
endif(F_ALIGN_COMMONS)
if(F_ALIGN_DOUBLE)
  add_definitions("-falign-double=1")
endif(F_ALIGN_DOUBLE)
if(F_ALIGN_FUNCTIONS)
  add_definitions("-falign-functions=1")
endif(F_ALIGN_FUNCTIONS)
if(F_ALIGN_JUMPS)
  add_definitions("-falign-jumps=1")
endif(F_ALIGN_JUMPS)
if(F_ALIGN_LABELS)
  add_definitions("-falign-labels=1")
endif(F_ALIGN_LABELS)
if(F_ALIGN_LOOPS)
  add_definitions("-falign-loops=1")
endif(F_ALIGN_LOOPS)
if(F_ALIGN_STRINGOPS)
  add_definitions("-falign-stringops=1")
endif(F_ALIGN_STRINGOPS)


list(APPEND MINIMAL_BUILTINS break cd eval exec exit export hash history pwd set shift source unset)
list(APPEND EXTRA_BUILTINS basename cat chmod dirname expr hostname ln mkdir
rm rmdir
which
mktemp
uname
# ls   mv printf   sort tr  uniq
 )
list(APPEND DEFAULT_BUILTINS ${MINIMAL_BUILTINS} type echo fdtable true false)


list(APPEND ALL_BUILTINS ${MINIMAL_BUILTINS} ${DEFAULT_BUILTINS}  ${EXTRA_BUILTINS} basename break cd dirname dump echo eval exec exit export expr false fdtable hash help history hostname ln pwd set shift source test true type unset
)
list(SORT ALL_BUILTINS)
list(REMOVE_DUPLICATES ALL_BUILTINS)

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


if(ENABLE_ALL_BUILTINS)
  foreach(BUILTIN ${ALL_BUILTINS})
    string(TOUPPER ${BUILTIN} NAME)
    message("Enable ${BUILTIN}")
    set(ENABLE_${NAME} ON)
    set(BUILTIN_${NAME} ON)
  endforeach(BUILTIN ${ALL_BUILTINS})
endif(ENABLE_ALL_BUILTINS)

foreach(BUILTIN ${ALL_BUILTINS})
	string(TOUPPER ${BUILTIN} NAME)
	if(NOT DEFINED ${ENABLE_${NAME}})
		isin("ENABLE_${NAME}" ${BUILTIN} ${DEFAULT_BUILTINS} )
  endif(NOT DEFINED ${ENABLE_${NAME}})

  if(ENABLE_ALL_BUILTINS)
      option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" ON)
  else(ENABLE_ALL_BUILTINS)

    if(DEFINED ENABLE_${NAME})
  		option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" "${ENABLE_${NAME}}")
  	else(DEFINED ENABLE_${NAME})
  		option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" OFF)
  	endif(DEFINED ENABLE_${NAME})
  endif(ENABLE_ALL_BUILTINS)
	#message(STATUS "Builtin ${BUILTIN} - ${ENABLE_${NAME}}")
endforeach(BUILTIN ${ALL_BUILTINS})

isin(ENABLE_DUMP dump ${BUILTINS_ENABLED})

set(DEBUG_OUTPUT_DEFAULT OFF)

if(ENABLE_DUMP)
  set(DEBUG_OUTPUT_DEFAULT ON)
endif(ENABLE_DUMP)


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

foreach(BUILTIN ${ALL_BUILTINS})

  string(TOUPPER ${BUILTIN} NAME)
	if(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
		list(APPEND BUILTINS_ENABLED ${BUILTIN})
	else(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
		list(APPEND BUILTINS_DISABLED ${BUILTIN})
	endif(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
endforeach(BUILTIN ${ALL_BUILTINS})

if(DEBUG_ALLOC)
  set(NAME_DEBUG "debug")
  add_definitions(-DDEBUG_ALLOC=1)
else()
  set(NAME_DEBUG "")
endif()

check_inline()

if(NOT "${INLINE_KEYWORD}" MATCHES "^inline$")
  if(NOT "${INLINE_KEYWORD}" MATCHES "^$")
    add_definitions("-Dinline=${INLINE_KEYWORD}")
  endif(NOT "${INLINE_KEYWORD}" MATCHES "^$")
endif(NOT "${INLINE_KEYWORD}" MATCHES "^inline$")

if(WIN32 OR WIN64 OR MINGW OR WINDOWS)
  check_library_exists(ws2_32 gethostname /usr/lib HAVE_WS2_32)

  if(NOT HAVE_WS2_32)
    check_library_exists(wsock32 gethostname /usr/lib HAVE_WSOCK32)
  endif(NOT HAVE_WS2_32)

  check_function_exists(fork HAVE_FORK)

  if(NOT HAVE_FORK)
    set(FORK_SOURCE "src/fork.c")
  endif(NOT HAVE_FORK)

endif(WIN32 OR WIN64 OR MINGW OR WINDOWS)

if(HAVE_WSOCK32)
  set(WINSOCK2_LIBRARY wsock32)
endif(HAVE_WSOCK32)
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

check_type_size(sigset_t SIGSET_T)
message("sigset_t size: ${SIGSET_T}")

check_type_size(pid_t PID_T)
if(SIGSET_T GREATER 0)
  set(HAVE_SIGSET_T TRUE)
endif(SIGSET_T GREATER 0)

check_include_file(glob.h HAVE_GLOB_H)
if(HAVE_GLOB_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} glob.h)
endif(HAVE_GLOB_H)

check_function_exists(glob HAVE_GLOB)

check_type_size(pid_t PID_T)
if(PID_T GREATER 0)
  set(HAVE_PID_T TRUE)
endif(PID_T GREATER 0)

if(HAVE_PID_T)
  add_definitions(-DHAVE_PID_T=1)
endif(HAVE_PID_T)


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
  check_c_compiler_flag("-g" F_G)
  check_c_compiler_flag("-ggdb" F_G_GDB)
  check_c_compiler_flag("-O0" F_OPT_NONE)

  if(F_G)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
  endif(F_G)
  if(F_G_GDB)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
  endif(F_G_GDB)
  if(F_OPT_NONE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
  endif(F_OPT_NONE)
else(BUILD_DEBUG)
  check_c_compiler_flag("-w" F_NOWARN)

  if(F_NOWARN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
  endif(F_NOWARN)
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
  file(GLOB ADDSOURCES lib/open/*.c lib/scan/*.c lib/buffer/*.c lib/fmt/*.c lib/byte/*.c lib/mmap/*.c lib/str/*.c lib/wait/*.c lib/unix/*.c)
  set(LIBSOURCES ${LIBSOURCES} ${ADDSOURCES} lib/stralloc/stralloc_cats.c lib/stralloc/stralloc_diffs.c lib/stralloc/stralloc_zero.c lib/stralloc/stralloc_free${NAME_DEBUG}.c lib/stralloc/stralloc_catlong0.c lib/stralloc/stralloc_copyb.c lib/stralloc/stralloc_copys.c lib/stralloc/stralloc_copy.c lib/stralloc/stralloc_ready${NAME_DEBUG}.c lib/stralloc/stralloc_catulong0.c lib/stralloc/stralloc_cat.c lib/stralloc/stralloc_catb.c lib/stralloc/stralloc_init.c lib/stralloc/stralloc_readyplus${NAME_DEBUG}.c lib/str/str_diff.c lib/str/str_len.c lib/str/str_rchr.c lib/str/str_chr.c lib/str/str_copy.c)
endif(NOT HAVE_LIBOWFAT)

#[[add_library(libshish ${LIBSOURCES})
set_target_properties(libshish PROPERTIES
  LIBRARY_OUTPUT_NAME "shish"
  OUTPUT_NAME "shish"
  PREFIX "lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
]]
file(GLOB SOURCES src/*.h  src/eval/*.c src/exec/*.c src/expand/*.c src/expr/*.c src/fd/*.c src/fdstack/*.c src/fdtable/*.c src/history/*.c src/job/*.c src/parse/*.c src/prompt/*.c src/redir/*.c src/sh/*.c src/sig/*.c src/source/*.c src/term/*.c src/tree/*.c src/var/*.c src/vartab/*.c)


#if(BUILD_DEBUG)
#  #file(GLOB ADDSOURCES src/debug/*.c)
#  #
#  set(DEBUG_SOURCES "${SOURCES}")
#  list(FILTER DEBUG_SOURCES INCLUDE REGEX ".*debug.c$")
#  foreach(SRC ${DEBUG_SOURCES})
#    string(REGEX REPLACE "debug\.c" ".c" OSRC "${SRC}")
#    message("Remove ${OSRC}")
#    list(REMOVE_ITEM SOURCES "${OSRC}")
#  endforeach(SRC ${DEBUG_SOURCES})
#else(BUILD_DEBUG)
#  list(FILTER SOURCES EXCLUDE REGEX ".*debug.c$")
#endif(BUILD_DEBUG)

if(DEBUG_ALLOC)
  list(APPEND SOURCES src/debug/debug_alloc.c src/debug/debug_free.c src/debug/debug_realloc.c src/debug/debug_memory.c )
endif(DEBUG_ALLOC)
if(DEBUG_OUTPUT)
  list(APPEND SOURCES src/debug/debug_begin.c src/debug/debug_char.c src/debug/debug_end.c src/debug/debug_error.c src/debug/debug_list.c src/debug/debug_node.c src/debug/debug_ptr.c src/debug/debug_redir.c src/debug/debug_space.c src/debug/debug_stralloc.c src/debug/debug_str.c src/debug/debug_sublist.c src/debug/debug_subnode.c src/debug/debug_subst.c src/debug/debug_ulong.c src/debug/debug_unquoted.c)
endif(DEBUG_OUTPUT)

list(APPEND SOURCES src/builtin/builtin_error.c src/builtin/builtin_search.c src/builtin/builtin_table.c)
list(APPEND SOURCES src/parse/parse_newnode${NAME_DEBUG}.c src/tree/tree_newnode${NAME_DEBUG}.c)

if(FORK_SOURCE)
  list(APPEND SOURCES ${FORK_SOURCE})
endif(FORK_SOURCE)


check_library_exists(m pow /usr/lib HAVE_LIBM)

if(HAVE_LIBM)
  set(MATH_LIBRARY m)
endif(HAVE_LIBM)

foreach(BUILTIN ${BUILTINS_ENABLED})
  list(APPEND SOURCES src/builtin/builtin_${BUILTIN}.c)
endforeach(BUILTIN ${BUILTINS_ENABLED})

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

foreach(DISABLED ${BUILTINS_DISABLED})
  set(SRC "src/builtin/builtin_${DISABLED}.c")
  list(LENGTH SOURCES N)
  list(REMOVE_ITEM SOURCES "${SRC}")
  #list(FILTER SOURCES EXCLUDE REGEX  "${SRC}")
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


set(BUILTIN_CONFIG "")
foreach(BUILTIN ${ALL_BUILTINS})
	string(TOUPPER ${BUILTIN} NAME)
  if(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 1")
    list(APPEND SOURCES "src/builtin/builtin_${BUILTIN}.c")
  else(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 0")
  endif(${BUILD_BUILTIN_${NAME}})
endforeach(BUILTIN ${ALL_BUILTINS})

file(WRITE "${CMAKE_BINARY_DIR}/src/builtin_config.h" "${BUILTIN_CONFIG}")
message("${BUILTIN_CONFIG}")

if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
  #list(FILTER SOURCES EXCLUDE REGEX  "src/tree/tree.*print.*\.c")
  list(REMOVE_ITEM SOURCES "src/tree/tree_print.c")
  list(REMOVE_ITEM SOURCES "src/tree/tree_printlist.c")
endif(NO_TREE_PRINT)


#
#configure_file("${CMAKE_SOURCE_DIR}/src/builtin_config.h.cmake" "${CMAKE_BINARY_DIR}/src/builtin_config.h"
#	#@ONLY
#	#ESCAPE_QUOTES
#	NEWLINE_STYLE UNIX)

list(APPEND SOURCES "${CMAKE_BINARY_DIR}/src/builtin_config.h")
add_definitions(-DHAVE_BUILTIN_CONFIG_H=1)


if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif(ENABLE_LTO)


add_executable(shish ${SOURCES} ${LIBSOURCES})
target_link_libraries(shish
  ${LIBOWFAT_LIBRARY}
  ${MATH_LIBRARY}
  ${ELECTRICFENCE_LIBRARY}
  ${WINSOCK2_LIBRARY}
)
if(EMSCRIPTEN)
  set_target_properties(shish PROPERTIES SUFFIX ".${EMSCRIPTEN_EXE_SUFFIX}")
endif(EMSCRIPTEN)

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
set(OUTSTR "${OUTSTR}\nAdditional libraries:\n  ${MATH_LIBRARY} ${WINSOCK2_LIBRARY}\n")
message(STATUS "${OUTSTR}")


