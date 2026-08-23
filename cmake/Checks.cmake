include(CheckSymbolExists)
include(CheckTypeSize)

check_symbol_exists(sys_siglist "signal.h" HAVE_SYS_SIGLIST_DECLARATION)

if(HAVE_SYS_SIGLIST_DECLARATION)
  add_definitions(-DHAVE_SYS_SIGLIST_DECLARATION)
endif()
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
check_c_compiler_flag("-Wno-error=unused-but-set-variable"
                      WARN_NO_UNUSED_FUNCTION)
if(WARN_NO_UNUSED_FUNCTION)
  set(WERROR_FLAG "${WERROR_FLAG} -Wno-error=unused-but-set-variable")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL
                                             "Release")
  check_c_compiler_flag("-falign-functions=1" F_ALIGN_FUNCTIONS)
  check_c_compiler_flag("-falign-jumps=1" F_ALIGN_JUMPS)
  check_c_compiler_flag("-falign-labels=1" F_ALIGN_LABELS)
  check_c_compiler_flag("-falign-loops=1" F_ALIGN_LOOPS)

  if(F_ALIGN_COMMONS)
    add_cflags(-falign-commons=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_COMMONS)
  if(F_ALIGN_DOUBLE)
    add_cflags(-falign-double=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_DOUBLE)
  if(F_ALIGN_FUNCTIONS)
    add_cflags(-falign-functions=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_FUNCTIONS)
  if(F_ALIGN_JUMPS)
    add_cflags(-falign-jumps=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_JUMPS)
  if(F_ALIGN_LABELS)
    add_cflags(-falign-labels=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_LABELS)
  if(F_ALIGN_LOOPS)
    add_cflags(-falign-loops=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_LOOPS)
  if(F_ALIGN_STRINGOPS)
    add_cflags(-falign-stringops=1 ${CMAKE_BUILD_TYPE})
  endif(F_ALIGN_STRINGOPS)
endif()

# MinSizeRel means bytes over everything else: every flag below is probed
# before use, and each one trades speed, hardening or diagnostics for size.
#
#   -f*-unwind-tables    .eh_frame is 15% of an untuned binary; nothing unwinds
#   -fno-jump-tables     switch tables become compare chains
#   -f*-sections         only pays off together with --gc-sections
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  foreach(
    FLAG
    -fno-asynchronous-unwind-tables
    -fno-unwind-tables
    -fno-stack-protector
    -fno-jump-tables
    -fno-plt
    -fno-ident
    -fmerge-all-constants
    -ffunction-sections
    -fdata-sections)
    string(MAKE_C_IDENTIFIER "F${FLAG}" FLAG_VAR)
    check_c_compiler_flag("${FLAG}" ${FLAG_VAR})
    if(${FLAG_VAR})
      add_cflags("${FLAG}")
    endif(${FLAG_VAR})
  endforeach(FLAG)

  foreach(
    FLAG
    "-Wl,--gc-sections"
    "-Wl,--as-needed"
    "-Wl,--build-id=none"
    "-Wl,-z,norelro"
    "-Wl,-z,noseparate-code"
    "-Wl,--hash-style=gnu")
    string(MAKE_C_IDENTIFIER "LD${FLAG}" FLAG_VAR)
    check_ldflag("${FLAG}" ${FLAG_VAR})
  endforeach(FLAG)
endif()

check_inline()

if(NOT "${INLINE_KEYWORD}" MATCHES "^inline$")
  add_definitions("-Dinline=${INLINE_KEYWORD}")
endif(NOT "${INLINE_KEYWORD}" MATCHES "^inline$")

if(WIN32
   OR WIN64
   OR MINGW
   OR WINDOWS)
  check_library_exists(ws2_32 gethostname /usr/lib HAVE_WS2_32)

  if(NOT HAVE_WS2_32)
    check_library_exists(wsock32 gethostname /usr/lib HAVE_WSOCK32)
  endif(NOT HAVE_WS2_32)

endif(
  WIN32
  OR WIN64
  OR MINGW
  OR WINDOWS)

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
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)

if(HAVE_SYS_TYPES_H)
  set(CMAKE_REQUIRED_INCLUDES sys/types.h)
endif(HAVE_SYS_TYPES_H)

if(CMAKE_CROSSCOMPILING)
  message(STATUS "Cross compiling")
  message(STATUS "Host system name: ${CMAKE_HOST_SYSTEM_NAME}")
  message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
endif(CMAKE_CROSSCOMPILING)

set(CMAKE_REQUIRED_QUIET TRUE)

message(CHECK_START "Checking for pointer size")
check_type_size("void*" SIZEOF_POINTER)

if(NOT SIZEOF_POINTER STREQUAL "")
  math(EXPR POINTER_BITS "${SIZEOF_POINTER} * 8")
  message(CHECK_PASS "${POINTER_BITS}bit")
  set(POINTER_SIZE "${SIZEOF_POINTER}")
  add_definitions(-DPOINTER_SIZE=${POINTER_SIZE})
else(NOT SIZEOF_POINTER STREQUAL "")
  message(CHECK_FAIL "failed")
endif(NOT SIZEOF_POINTER STREQUAL "")

if(NOT CMAKE_CROSSCOMPILING)
  check_type_size(ssize_t SIZEOF_SSIZE_T)
  if(NOT SIZEOF_SSIZE_T STREQUAL "")
    set(HAVE_SSIZE_T 1)
    add_definitions(-D_SSIZE_T_=1)
  endif(NOT SIZEOF_SSIZE_T STREQUAL "")

  check_type_size(sigset_t SIZEOF_SIGSET_T)
  if(NOT SIZEOF_SIGSET_T STREQUAL "")
    set(HAVE_SIGSET_T 1)
  endif(NOT SIZEOF_SIGSET_T STREQUAL "")

  check_type_size(pid_t SIZEOF_PID_T)
  if(NOT SIZEOF_PID_T STREQUAL "")
    set(HAVE_PID_T 1)
  endif(NOT SIZEOF_PID_T STREQUAL "")

  check_type_size(uid_t SIZEOF_UID_T)
  if(NOT SIZEOF_UID_T STREQUAL "")
    set(HAVE_UID_T 1)
  endif(NOT SIZEOF_UID_T STREQUAL "")
endif(NOT CMAKE_CROSSCOMPILING)

set(CMAKE_REQUIRED_QUIET FALSE)

if(HAVE_ALLOCA_H)
  list(APPEND CMAKE_REQUIRED_INCLUDES alloca.h)
  check_symbol_exists(alloca alloca.h HAVE_ALLOCA_SYMBOL)
endif()

check_compile(
  HAVE_ALLOCA_ALLOCA_H
  "#include <stdlib.h>\n#include <alloca.h>\n\n\nint main() {\n  char* c=alloca(23);\n  (void)c;\n  return 0;\n}"
)
if(NOT HAVE_ALLOCA_ALLOCA_H)
  check_compile(
    HAVE_ALLOCA_MALLOC_H
    "#include <stdlib.h>\n#include <alloca.h>\n\n\nint main() {\n  char* c=alloca(23);\n  (void)c;\n  return 0;\n}"
  )
endif()

if(HAVE_ALLOCA_ALLOCA_H OR HAVE_ALLOCA_MALLOC_H)
  set(HAVE_ALLOCA TRUE)
  add_definitions(-DHAVE_ALLOCA)

  if(HAVE_ALLOCA_MALLOC_H)
    set(ALLOCA_HEADER
        "malloc.h"
        CACHE STRING "Header for alloca()")
  else()
    set(ALLOCA_HEADER
        "alloca.h"
        CACHE STRING "Header for alloca()")
  endif()
endif()

if(NOT HAVE_ALLOCA)
  set(ALLOCA_HEADER "")
endif()

check_include_file(sys/wait.h HAVE_SYS_WAIT_H)

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} unistd.h)
  list(APPEND CMAKE_REQUIRED_INCLUDES unistd.h)
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
check_function_exists(symlink HAVE_SYMLINK)
check_function_exists(link HAVE_LINK)

check_include_file(pwd.h HAVE_PWD_H)
check_include_file(grp.h HAVE_GRP_H)
check_function_exists(getpwuid_r HAVE_GETPWUID_R)
check_function_exists(getpwuid HAVE_GETPWUID)
check_function_exists(getgrgid_r HAVE_GETGRGID_R)
check_function_exists(getgrgid HAVE_GETGRGID)

check_include_file(signal.h HAVE_SIGNAL_H)
if(HAVE_SIGNAL_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} signal.h)
  list(APPEND CMAKE_REQUIRED_INCLUDES signal.h)
endif(HAVE_SIGNAL_H)

check_function_exists(sigprocmask HAVE_SIGPROCMASK)

if(NOT HAVE_SIGPROCMASK)
  check_function_exists(sigblock HAVE_SIGBLOCK)
  check_function_exists(sigsetmask HAVE_SIGSETMASK)
  check_function_exists(sigpause HAVE_SIGPAUSE)

endif(NOT HAVE_SIGPROCMASK)
check_function_exists(sigaction HAVE_SIGACTION)
check_function_exists(setpgid HAVE_SETPGID)

# check_function_exists(fork ...) alone isn't trustworthy on either of
# the two platform families below -- both need the result hardcoded
# instead of trusting the check:
#
#   native Windows (WIN32/WIN64/MSVC/MINGW/WINDOWS, but *not* Cygwin or
#   MSYS -- both of those provide a real POSIX fork() from their own C
#   library and go through the normal check like any other POSIX
#   target) has no libc-provided fork() for the check to find, but
#   lib/unix/fork.c supplies one (via RtlCloneUserProcess) that
#   job_fork() already links against on this platform -- so the
#   correct result here is TRUE despite what the check would say.
#
#   Emscripten/WASI is the opposite kind of false result: musl's libc
#   ships a real, linkable fork() symbol, so the check finds it (see
#   "Looking for fork - found" from `emcmake cmake`), but the
#   underlying syscall has no implementation in a single-threaded wasm
#   module and fails at runtime (ENOSYS) -- so the correct result here
#   is FALSE despite what the check would say. Detected the same way
#   CMakeLists.txt's own EMSCRIPTEN variable is (compiler basename
#   matching "em*", not CMAKE_SYSTEM_NAME) -- this file is include()'d
#   before that variable is set, and cfg-emscripten's own toolchain
#   file (cfg-cmake.sh's cfg-emscripten, which would otherwise make
#   CMAKE_SYSTEM_NAME reliable) never actually resolves due to an
#   unrelated bug there (${EMSCRIPTEN:=dirname $(which emcc)} is
#   missing a $() around dirname, so it evaluates to the literal
#   string "dirname /path/to/emcc" instead of running it), so
#   CMAKE_SYSTEM_NAME isn't set to "Emscripten" in practice either.
#   CMAKE_SYSTEM_NAME=WASI (cfg-wasi) doesn't have that problem --
#   it's passed directly as a cmake argument, not behind a toolchain
#   file's existence check -- so it's checked normally below.
#
#   MSYS has the identical CMAKE_SYSTEM_NAME problem as Emscripten
#   above, for the same reason (cfg-msys's own TOOLCHAIN, msys64.cmake/
#   msys32.cmake, is an external file this repo doesn't control) --
#   cross-checked against the compiler path too (cfg-msys's own
#   x86_64-pc-msys-gcc/i686-pc-msys-gcc), not just CMAKE_SYSTEM_NAME.
string(REGEX REPLACE ".*/" "" HAVE_FORK_COMPILER_NAME "${CMAKE_C_COMPILER}")

if((WIN32 OR WIN64 OR MSVC OR MINGW OR WINDOWS)
   AND NOT CYGWIN
   AND NOT CMAKE_SYSTEM_NAME MATCHES "MSYS"
   AND NOT HAVE_FORK_COMPILER_NAME MATCHES "msys")
  set(HAVE_FORK TRUE)
elseif(HAVE_FORK_COMPILER_NAME MATCHES "^em" OR CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(HAVE_FORK FALSE)
else()
  check_function_exists(fork HAVE_FORK)
endif()

unset(HAVE_FORK_COMPILER_NAME)

check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)

check_include_file(glob.h HAVE_GLOB_H)
if(HAVE_GLOB_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} glob.h)
endif(HAVE_GLOB_H)

check_function_exists(glob HAVE_GLOB)

check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
if(HAVE_SYS_MMAN_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/mman.h)
  list(APPEND CMAKE_REQUIRED_INCLUDES sys/mman.h)
endif(HAVE_SYS_MMAN_H)

check_function_exists(mmap HAVE_MMAP_FUNC)
check_function_exists(munmap HAVE_MUNMAP)
check_function_exists(mremap HAVE_MREMAP)

# whether the platform can support memory-mapped file I/O at all --
# either POSIX mmap(2)/munmap(2) (sys/mman.h present and both
# functions found), or Windows' CreateFileMapping/MapViewOfFile, which
# lib/mmap/ and lib/buffer/'s WINDOWS_NATIVE branches implement mmap(2)
# in terms of. USE_MMAP (an option(), see CMakeLists.txt) may only be
# ON when this is true; HAVE_MMAP (the compiler define lib/mmap/ and
# its callers actually check) tracks USE_MMAP's final, validated
# value, not raw platform capability.
if(HAVE_SYS_MMAN_H AND HAVE_MMAP_FUNC AND HAVE_MUNMAP)
  set(HAVE_MMAP_SUPPORT TRUE)
elseif(WIN32
       OR WIN64
       OR MINGW
       OR WINDOWS)
  set(HAVE_MMAP_SUPPORT TRUE)
endif()

if(USE_MMAP AND NOT HAVE_MMAP_SUPPORT)
  message(WARNING "USE_MMAP requested, but mmap()/munmap() aren't available "
                   "on this platform -- disabling")
  set(USE_MMAP
      OFF
      CACHE BOOL "Use mmap() for memory-mapped file I/O" FORCE)
endif()

set(HAVE_MMAP ${USE_MMAP})

check_type_size(pid_t PID_T)
if(PID_T GREATER 0)
  set(HAVE_PID_T TRUE)
endif(PID_T GREATER 0)

# if(HAVE_SIGNAL_H) check_symbol_exists(sigset_t signal.h HAVE_SIGSET_T)
# check_symbol_exists(sigset_t signal.h HAVE_SIGSET_T) check_type_size(sigset_t
# SIZEOF_SIGSET_T)
#
# if(SIZEOF_SIGSET_T) set(HAVE_SIGSET_T 1) else() set(HAVE_SIGSET_T 0) endif()
# endif()

check_function_exists(sys_siglist HAVE_SYS_SIGLIST)

if(BUILD_DEBUG OR CMAKE_BUILD_TYPE MATCHES Deb)
  check_cflag("-O0" F_OPT_NONE DEBUG)
  check_cflag("-ggdb" F_G_GDB DEBUG)
  if(NOT F_G_GDB)
    check_cflag("-g3" F_G3 DEBUG)
    if(NOT F_G3)
      check_cflag("-g" F_G DEBUG)
    endif(NOT F_G3)
  endif(NOT F_G_GDB)
else()
  check_c_compiler_flag("-w" F_NOWARN)
  if(F_NOWARN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
  endif(F_NOWARN)
endif()

string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_MINSIZEREL
                     "${CMAKE_C_FLAGS_MINSIZEREL}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_RELEASE
                     "${CMAKE_C_FLAGS_RELEASE}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_RELWITHDEBINFO
                     "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REGEX REPLACE "-O[1-9]" "-Os" CMAKE_C_FLAGS_DEBUG
                     "${CMAKE_C_FLAGS_DEBUG}")

check_library_exists(m pow "" HAVE_LIBM)

if(HAVE_LIBM)
  set(MATH_LIBRARY m)
endif(HAVE_LIBM)
