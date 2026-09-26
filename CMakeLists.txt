cmake_minimum_required(VERSION 3.10..4.0)

project(shish C)

set(CMAKE_LEGACY_CYGWIN_WIN32 0)

set(PACKAGE_NAME shish)
set(PACKAGE_VERSION 0.9)
set(PACKAGE_TARNAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'MinSizeRel' as none was specified.")
  set(CMAKE_BUILD_TYPE
      MinSizeRel
      CACHE STRING "Choose the type of build." FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

option(WARN_WERROR "Halt the compilation with an error on compiler warnings." OFF)
option(USE_MMAP "Use mmap() for memory-mapped file I/O" ON)
option(MINSIZE_STRIP "Strip unneeded sections from a MinSizeRel build" ON)

include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)

include(cmake/Targets.cmake)
include(cmake/Functions.cmake)
include(cmake/Checks.cmake)
include(cmake/Pedantic.cmake)

detect_pedantic_flags(PEDANTIC_COMPILE_FLAGS)

include(cmake/Debug.cmake)

compiler_flags_debug(CMAKE_C_FLAGS_DEBUG)

include(cmake/Builtins.cmake)

configure_shish_builtins()

include(cmake/Digest.cmake)

if(BUILTIN_DIGEST)
  configure_digest_builtin()
endif()

check_sys_siglist_declaration(HAVE_SYS_SIGLIST HAVE_SYS_SIGLIST_DECLARATION)
check_no_unused_warn_flags()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  check_falign_flags()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  #check_fno_optim_flags()
endif()

check_inline(INLINE_KEYWORD)

if(NOT "${INLINE_KEYWORD}" MATCHES "^inline$")
  add_definitions("-Dinline=${INLINE_KEYWORD}")
endif()

check_windows_native(WINDOWS_NATIVE)

if(WINDOWS_NATIVE)
  check_winsock2(HAVE_WINSOCK SOCKET_LIBRARY)
else()
  check_library_exists(socket "connect" /usr/lib HAVE_LIBSOCKET)
  if(HAVE_LIBSOCKET)
    set(SOCKET_LIBRARY "socket")
  else()
    set(SOCKET_LIBRARY "")
  endif()
endif()

check_includes_def(alloca.h fcntl.h glob.h inttypes.h signal.h stdint.h sys/stat.h sys/types.h sys/wait.h unistd.h)

have_includes(CMAKE_EXTRA_INCLUDE_FILES fcntl.h glob.h signal.h sys/stat.h sys/types.h sys/wait.h unistd.h)
have_includes(CMAKE_REQUIRED_INCLUDES signal.h sys/types.h unistd.h)

string(REGEX REPLACE ".*/" "" COMPILER_NAME "${CMAKE_C_COMPILER}")

check_pointer_size()
# dump(POINTER_SIZE)

if(NOT CMAKE_CROSSCOMPILING)
  check_type_sizes()
endif()

check_alloca()

check_functions(fcntl)
check_functions(readlink)
check_functions(sethostname lstat getppid glob)

#add_definitions(-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64)
set_add(CMAKE_REQUIRED_DEFINITIONS  
  #-D_GNU_SOURCE
 -D_FILE_OFFSET_BITS=64
)

check_compile(HAVE_GLOB 
  "#include <stdio.h>\n#include <sys/stat.h>\n#include <glob.h>\n\nint\nmain() {\n\tglob_t g = {0};\n\tint result = glob(\"{src,lib}/{*.h,*/*.c}\", GLOB_BRACE | GLOB_TILDE, 0, &g);\n\n\tprintf(\"result = %d\\n\", result);\n\n\tint c = g.gl_pathc;\n\tchar** v = g.gl_pathv;\n\n\tfor(int i = 0; i < c; i++)\n\t\tprintf(\"entry: %s\\n\", v[i]);\n\n\treturn 0;\n}\n"
  #COMPILE_DEFINITIONS "-D_GNU_SOURCE;-D_FILE_OFFSET_BITS=64"
)

#dump(HAVE_GLOB)

# check_functions(sigaction sigismember signal sigprocmask sigsuspend)

check_pw_functions()
check_sig_functions()
check_fork_function()

check_memory_mapping()

check_type_size(pid_t PID_T)
if(PID_T GREATER 0)
  set(HAVE_PID_T TRUE)
endif()

if(BUILD_DEBUG OR CMAKE_BUILD_TYPE MATCHES Deb)
else()
  check_cflag("-w" F_NOWARN CMAKE_C_FLAGS)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  compiler_flags_optimize_size()
endif()

check_libmath(LIBMATH)

check_emscripten()

option_debug_mode()

option_link_static()
option_link_time_optimization()

check_termios_winsize()

option(NO_TREE_PRINT "Disable tree_print for history" OFF)

if(BUILD_DEBUG)
  option(DEBUG_COLOR "Color debug output" OFF)
  option(DEBUG_OUTPUT "Print debug output" ${DEBUG_OUTPUT_DEFAULT})

  if(DEBUG_OUTPUT)
    add_definitions(-DDEBUG_OUTPUT=1)
  endif()
  if(DEBUG_COLOR)
    add_definitions(-DDEBUG_COLOR=1)
  endif()
endif()

configure_file("${CMAKE_SOURCE_DIR}/config.h.cmake" "${CMAKE_BINARY_DIR}/config.h")

check_source_definitions()

add_definitions(-DHAVE_CONFIG_H=1)

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}/src" 
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

#[[set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_MODULE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")]]

include(cmake/libowfat.cmake)

configure_libowfat()

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

# ------------------------------------------------------------------------------
# Build target "libtext" for all sources in text/
# ------------------------------------------------------------------------------
file(GLOB LIBTEXT_SOURCES text/*.h text/*/*.c)
 
# grep's backend: text/dfa (in-tree, no libc regex dependency) by default, or the system's <regex.h> if this is turned on.
option(GREP_USE_SYSTEM_REGEX "Use the system's regex.h for grep instead of text/dfa" OFF)
if(GREP_USE_SYSTEM_REGEX)
  set_source_files_properties(src/builtin/extra/builtin_grep.c PROPERTIES COMPILE_DEFINITIONS GREP_USE_SYSTEM_REGEX=1)
endif()
 
# ------------------------------------------------------------------------------
# Build target "libshell" for all sources in src/
# ------------------------------------------------------------------------------
file(GLOB LIBSHELL_SOURCES src/*.h src/*/*.c)

# needs third_party/, so only built when the digest builtin is on (BUILTIN_SOURCES)
list(FILTER LIBSHELL_SOURCES EXCLUDE REGEX "builtin_digest\\.c")
list(FILTER LIBSHELL_SOURCES EXCLUDE REGEX "(sh_fmt\\.c|sh_parse2ast\\.c|sh_init\\.c|sh_main\\.c|sh_util_wasm\\.c)")

if(DEBUG_OUTPUT)
  file(GLOB DEBUG_SOURCES src/debug.h src/debug/*.c)
  set_add(LIBSHELL_SOURCES  ${DEBUG_SOURCES})
else()
  list(FILTER LIBSHELL_SOURCES EXCLUDE REGEX "debug_")
  set_add(LIBSHELL_SOURCES  src/debug/debug_squoted.c src/debug/debug_argv.c )
endif()

#dump_list(LIBSHELL_SOURCES)

set_add(BUILTIN_SOURCES src/builtin/builtin_error.c src/builtin/builtin_search.c src/builtin/builtin_table.c)

if(NO_TREE_PRINT)
  add_definitions(-DNO_TREE_PRINT=1)
  list(REMOVE_ITEM LIBSHELL_SOURCES "src/tree/tree_print.c")
  list(REMOVE_ITEM LIBSHELL_SOURCES "src/tree/tree_printlist.c")
endif()

set_add(LIBSHELL_SOURCES
  "${CMAKE_BINARY_DIR}/src/builtin_config.h"
  ${BUILTIN_SOURCES} 
  ${LIBTEXT_SOURCES}
) 

add_library(libshell STATIC ${LIBSHELL_SOURCES})
set_target_properties(libshell PROPERTIES OUTPUT_NAME shell)

set_add(SHISH_SOURCES src/sh/sh_init.c src/sh/sh_main.c)
 
set_init(ALL_SOURCES ${SHISH_SOURCES} ${LIBSHELL_SOURCES})

add_executable(shish ${SHISH_SOURCES})
target_link_libraries(shish PRIVATE libshell ${LIBOWFAT_LIBRARY} ${LIBMATH} ${ELECTRICFENCE_LIBRARY})

if(EMSCRIPTEN)
  set_target_properties(shish PROPERTIES SUFFIX ".${EMSCRIPTEN_EXE_SUFFIX}")
endif()

strip_minsize(shish)

option(BUILD_SHFORMAT "Build shformat program" ON)

if(BUILD_SHFORMAT)
  file(GLOB SHFORMAT_SOURCES src/*.h src/debug/*.c src/sh/sh_fmt.c)

  list(FILTER SHFORMAT_SOURCES EXCLUDE REGEX "dump")
  set_add(ALL_SOURCES ${SHFORMAT_SOURCES})

  add_executable(shformat ${SHFORMAT_SOURCES})
  target_link_libraries(shformat PRIVATE libshell ${LIBOWFAT_LIBRARY} ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shformat PUBLIC SHFORMAT=1)

  strip_minsize(shformat)
  install(TARGETS shformat DESTINATION bin)
endif()

option(BUILD_SHPARSE2AST "Build shformat program" OFF)

if(BUILD_SHPARSE2AST)
  file(GLOB SHPARSE2AST_SOURCES src/*.h src/debug/*.c src/sh/sh_parse2ast.c)

  list(FILTER SHPARSE2AST_SOURCES EXCLUDE REGEX "dump")
  set_add(ALL_SOURCES ${SHPARSE2AST_SOURCES})

  add_executable(shparse2ast ${SHPARSE2AST_SOURCES})
  target_link_libraries(shparse2ast PRIVATE libshell ${LIBOWFAT_LIBRARY} ${LIBMATH} ${ELECTRICFENCE_LIBRARY})

  target_compile_definitions(shparse2ast PUBLIC SHPARSE2AST=1)

  strip_minsize(shparse2ast)
  install(TARGETS shparse2ast DESTINATION bin)
endif()

option(BUILD_SHUTIL_WASM "Build shutil, a WASM-only library module (format + AST dump, no CLI, no main loop)" OFF)

if(BUILD_SHUTIL_WASM)
  if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "BUILD_SHUTIL_WASM requires an Emscripten toolchain")
  endif()

  file(GLOB SHUTIL_SOURCES src/*.h src/debug/*.c src/sh/sh_util_wasm.c)
  list(FILTER SHUTIL_SOURCES EXCLUDE REGEX "dump")
  set_add(ALL_SOURCES ${SHUTIL_SOURCES})

  add_executable(shutil ${SHUTIL_SOURCES})
  target_link_libraries(shutil PRIVATE libshell ${LIBOWFAT_LIBRARY} ${LIBMATH})

  # SHPARSE2AST=1 gates the JSON debug-tree code in src/debug/*.c on; SHFORMAT=1 keeps parse_simpletok.c's PS2-prompt call site
  # (still dead here regardless, via parse_prompt_hook -- see fixes/219) consistent with the other parser-only tools.
  target_compile_definitions(shutil PUBLIC SHFORMAT=1 SHPARSE2AST=1)

  set_target_properties(shutil PROPERTIES SUFFIX ".js")
  target_link_options(
    shutil
    PRIVATE
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createShutil"
    "-sEXPORTED_FUNCTIONS=['_shutil_format','_shutil_parse_ast','_shutil_free','_malloc','_free']"
    "-sEXPORTED_RUNTIME_METHODS=['cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
    "-sINVOKE_RUN=0")

  strip_minsize(shutil)
endif()

set_source_files_properties(src/builtin/builtin_table.c PROPERTIES COMPILE_FLAGS "${BUILTIN_FLAGS}")

install(TARGETS shish DESTINATION bin)

target_cmake_format()
target_clang_format(${ALL_SOURCES})

file(GLOB TESTS tests/*.sh)
list(FILTER TESTS EXCLUDE REGEX "bourne-compat.sh|common.sh|run-tst.sh")

relative_paths(TESTS "${CMAKE_CURRENT_SOURCE_DIR}" ${TESTS})

option(DO_TESTS "Perform tests" ON)

get_target_property(SHISH_SUFFIX shish SUFFIX)
if(NOT SHISH_SUFFIX)
  set(SHISH_SUFFIX "")
endif()
set(SHISH_PATH "${CMAKE_CURRENT_BINARY_DIR}/shish${SHISH_SUFFIX}")

option(DO_CONFORMANCE_TESTS "Also run the tests/posix conformance suite" ON)
option(DO_YASH_TESTS "Also run the tests/yash conformance suite (off by default -- tests/yash/random-y.tst hangs and eats a 120s TIMEOUT on every run, see BUGS)" OFF)

if(DO_TESTS)
  include(CTest)

  foreach(TEST ${TESTS})
    basename(NAME "${TEST}")
    add_test(
      NAME "${NAME}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND "${SHISH_PATH}" "${TEST}")
  endforeach()

  # lib/arena unit test (C, links against the in-tree libowfat)
  add_executable(arena_test tests/arena_test.c)
  target_link_libraries(arena_test PRIVATE ${LIBOWFAT_LIBRARY})
  add_test(NAME arena_test COMMAND arena_test)

  # tests/posix/*.tst and tests/yash/*.tst: yash's own POSIX/self conformance suite 
  # yash's run-test.sh harness interprets each *.tst file's test_oE/ test_x/... cases). 
  # 
  # Each file is one CTest case, wrapped by tests/run-tst.sh (which drives the harness under bash -- it needs $LINENO
  # and alias-expansion-in-scripts, neither of which dash has, and bash needs -O expand_aliases explicitly for the latter)
  # and translates its per-case PASSED/FAILED/SKIPPED result file into a single pass/fail for the whole test.
  #
  # Re-enabled (2026-07-23): this was disabled because tests/posix/fnmatch-p.tst hung the testee at 100% CPU, 
  # which made a plain `ctest`/`make test` run that included this suite never return. Fixed by path_fnmatch()'s bracket-class
  # rewrite (fixes/67) -- confirmed the whole suite (239 files across both directories) now completes in about a minute with no
  # hangs. Most individual files still fail (POSIX compliance triage is ongoing, see TODO.md Goal 2) -- that's expected and is 
  # what CTest is here to report, not a reason to keep this off.
  #
  # tests/yash split into its own DO_YASH_TESTS option, off by default (2026-07-24): unlike the rest of the corpus, 
  # tests/yash/random-y.tst hangs and only terminates via its own 120s TIMEOUT (BUGS: yash-random-y-tst-hangs), so it alone dominates
  # a default `ctest` run's wall time. tests/posix has no such straggler and stays on by default under DO_CONFORMANCE_TESTS.
  set(CONFORMANCE_SUITES)

  if(DO_CONFORMANCE_TESTS)
    set_add(CONFORMANCE_SUITES posix)
  endif()

  if(DO_YASH_TESTS)
    set_add(CONFORMANCE_SUITES yash)
  endif()

  # tests/posix/*.tst files marked "%REQUIRETTY%" gate themselves on "../checkfg" (tests/checkfg.c -- restored from git history, 
  # fixes/213's follow-up: it existed as of commit caa1ea28 but had gone missing from the tree, silently turning every one of these 
  # 44 files into an unconditional skip via "command not found" rather than a real "no controlling terminal" check) and self-skip
  # without a real controlling terminal. DO_PTY_TESTS (off by default -- Linux/POSIX pty ioctls, and it changes timing/behavior for a
  # job-control-sensitive slice of the suite) wraps just those files in tests/pty-run (tests/pty-run.c), which gives the testee a real
  # pty-backed session/controlling terminal instead of letting them opt out. checkfg itself is always built when conformance tests are
  # on, on any POSIX platform, so the self-skip check is a real check rather than a missing-binary error either way.
  if(DO_CONFORMANCE_TESTS AND NOT WINDOWS_NATIVE)
    add_executable(checkfg tests/checkfg.c)

    # tests/posix/run-test.sh's *.tst files reference "../checkfg" from inside a per-run tests/posix/tmp.$$ work directory it creates
    # itself, so the binary has to live at tests/posix/checkfg in the source tree -- same place run-test.sh already leaves its own
    # tmp.NNNNN/.trs artifacts (BUGS: harness-leaves-tmp-dirs).
    set_target_properties(checkfg PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/posix")

    option(DO_PTY_TESTS "Run the tests/posix/*.tst files gated on a real controlling terminal (%REQUIRETTY%) under tests/pty-run instead of letting them self-skip" OFF)

    if(DO_PTY_TESTS)
      add_executable(pty-run tests/pty-run.c)
    endif()
  endif()

  foreach(SUITE ${CONFORMANCE_SUITES})
    file(GLOB CONFORMANCE_TESTS "tests/${SUITE}/*.tst")

    foreach(TEST ${CONFORMANCE_TESTS})
      basename(NAME "${TEST}")

      set(REQUIRETTY_MARK)

      if(SUITE STREQUAL "posix"
         AND DO_PTY_TESTS
         AND NOT WINDOWS_NATIVE)
        file(STRINGS "${TEST}" REQUIRETTY_MARK REGEX "%REQUIRETTY%")
      endif()

      if(REQUIRETTY_MARK)
        add_test(
          NAME "${SUITE}/${NAME}"
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
          COMMAND "$<TARGET_FILE:pty-run>" -t 60 -- sh "tests/run-tst.sh" "${SHISH_PATH}" "tests/${SUITE}" "${NAME}")
      else()
        add_test(
          NAME "${SUITE}/${NAME}"
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
          COMMAND sh "tests/run-tst.sh" "${SHISH_PATH}" "tests/${SUITE}" "${NAME}")
      endif()

      set_tests_properties("${SUITE}/${NAME}" PROPERTIES TIMEOUT 120)
    endforeach()
  endforeach()
endif()
