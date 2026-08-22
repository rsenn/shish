list(
  APPEND
  MINIMAL_BUILTINS
  alias
  break
  cd
  command
  eval
  exec
  exit
  export
  expr
  getopts
  hash
  history
  jobs
  kill
  local
  printf
  pwd
  read
  readonly
  return
  set
  shift
  source
  test
  times
  trap
  type
  umask
  unset
  wait)
list(
  APPEND
  EXTRA_BUILTINS
  basename
  cat
  chmod
  dirname
  hostname
  ln
  mkdir
  rm
  rmdir
  which
  mktemp
  uname)
list(
  APPEND
  DEFAULT_BUILTINS
  ${MINIMAL_BUILTINS}
  help
  type
  echo
  fdtable
  true
  false)

set(ALL_BUILTINS "")
list(
  APPEND
  ALL_BUILTINS
  ${MINIMAL_BUILTINS}
  ${DEFAULT_BUILTINS}
  ${EXTRA_BUILTINS}
  basename
  break
  cd
  dirname
  dump
  echo
  eval
  exec
  exit
  export
  expr
  false
  fdtable
  hash
  help
  history
  hostname
  ln
  printf
  pwd
  set
  shift
  source
  test
  times
  true
  type
  unset)
list(SORT ALL_BUILTINS)
list(REMOVE_DUPLICATES ALL_BUILTINS)

set(BUILTINS_ENABLED "")
set(BUILTINS_DISABLED "")

# option(ENABLE_ALL_BUILTINS "Enable all builtins" OFF)

function(ON_ENABLE_ALL_BUILTINS VAR ACCESS VALUE CURRENT_LIST_FILE STACK)

  if(NOT "${ACCESS}" STREQUAL "READ_ACCESS")
    message("VAR = ${VAR}")
    message("ACCESS = ${ACCESS}")
    message("VALUE = ${VALUE}")
    message("CURRENT_LIST_FILE = ${CURRENT_LIST_FILE}")
    message("STACK = ${STACK}")
  endif(NOT "${ACCESS}" STREQUAL "READ_ACCESS")

endfunction(
  ON_ENABLE_ALL_BUILTINS
  VAR
  ACCESS
  VALUE
  CURRENT_LIST_FILE
  STACK)

# variable_watch(ENABLE_ALL_BUILTINS ON_ENABLE_ALL_BUILTINS)

# ENABLE_DUMP doubles as "the user asked for dump" and, further down,
# as "dump is being built", so record the request before the loop below
# starts writing to it.
if(BUILD_DEBUG OR ENABLE_DUMP)
  set(WANT_DUMP ON)
endif(BUILD_DEBUG OR ENABLE_DUMP)

# -DENABLE_ALL_BUILTINS=ON means all of them, so start from a clean
# slate: drop every cached BUILTIN_<NAME>, including stale ones for
# builtins no longer in ALL_BUILTINS, before the loop writes the new
# answers.
if(ENABLE_ALL_BUILTINS)
  get_cmake_property(CACHED_VARIABLES CACHE_VARIABLES)
  foreach(VAR ${CACHED_VARIABLES})
    if(VAR MATCHES "^BUILTIN_")
      unset(${VAR} CACHE)
    endif(VAR MATCHES "^BUILTIN_")
  endforeach(VAR)
endif(ENABLE_ALL_BUILTINS)

# Decide each builtin once, most specific answer first:
#
#   -DENABLE_ALL_BUILTINS=ON   every builtin, no exceptions
#   -DENABLE_<NAME>=ON/OFF     that one builtin
#   otherwise                  DEFAULT_BUILTINS
#
# The test is "DEFINED ENABLE_${NAME}", not "DEFINED ${ENABLE_${NAME}}"
# -- the latter asks whether the *value* (ON) names a variable, which it
# does not, so every -DENABLE_<NAME> was silently discarded here.
foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  if(ENABLE_ALL_BUILTINS)
    set(WANT_BUILTIN ON)
  elseif(DEFINED ENABLE_${NAME})
    set(WANT_BUILTIN ${ENABLE_${NAME}})
  else()
    isin(WANT_BUILTIN ${BUILTIN} ${DEFAULT_BUILTINS})
  endif()

  # BUILTIN_<NAME> is the cached answer; ENABLE_<NAME> stays uncached, so
  # it means "asked for on this command line" and nothing else. Caching
  # it would make a one-off -DENABLE_DUMP=ON stick to the build
  # directory, and with it the DEBUG_OUTPUT default dump drags along.
  #
  # FORCE: option()/set() without it keep whatever an earlier configure
  # of the same build directory cached, so -DENABLE_<NAME> would only
  # ever take effect in a fresh one.
  set(BUILTIN_${NAME}
      ${WANT_BUILTIN}
      CACHE BOOL "Enable ${BUILTIN} builtin" FORCE)
endforeach(BUILTIN ${ALL_BUILTINS})

# BUILTIN_<NAME> is the state; ENABLE_* is only ever a request made on
# one command line. -D puts them in the cache, so drop them again --
# otherwise a single -DENABLE_PRINTF=OFF sticks to the build directory
# and no later configure can undo it.
unset(ENABLE_ALL_BUILTINS)
unset(ENABLE_ALL_BUILTINS CACHE)

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  unset(ENABLE_${NAME} CACHE)
endforeach(BUILTIN ${ALL_BUILTINS})

# "dump" only exists to inspect shell state while debugging, so a debug
# build gets it whether or not it was named in the builtin list.
# ENABLE_DUMP is also set by cmake/Debug.cmake for CMAKE_BUILD_TYPE=Debug,
# and can be passed directly (-DENABLE_DUMP=ON). Has to run before the
# loop below, which is what reads BUILTIN_* to decide what gets built.
if(WANT_DUMP)
  set(BUILTIN_DUMP
      ON
      CACHE BOOL "Enable dump builtin" FORCE)
endif(WANT_DUMP)

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  # a plain truth test: isin() answers TRUE/FALSE and an option()
  # answers ON/OFF, and "TRUE STREQUAL ON" is false
  if(BUILTIN_${NAME})
    list(APPEND BUILTINS_ENABLED ${BUILTIN})
  else(BUILTIN_${NAME})
    list(APPEND BUILTINS_DISABLED ${BUILTIN})
  endif(BUILTIN_${NAME})
endforeach(BUILTIN ${ALL_BUILTINS})

# now that BUILTINS_ENABLED is populated, "is dump being built" is a
# real answer -- and dump's output goes through the debug buffer, so
# building it turns DEBUG_OUTPUT on by default (CMakeLists.txt).
isin(ENABLE_DUMP dump ${BUILTINS_ENABLED})

set(DEBUG_OUTPUT_DEFAULT OFF)

if(ENABLE_DUMP)
  set(DEBUG_OUTPUT_DEFAULT ON)
endif(ENABLE_DUMP)

foreach(BUILTIN ${BUILTINS_ENABLED})
  list(APPEND SOURCES src/builtin/builtin_${BUILTIN}.c)
endforeach(BUILTIN ${BUILTINS_ENABLED})

# NAME has to come from ${DISABLED}: without it the loop reuses whatever
# NAME the previous foreach left behind, so a builtin that was enabled in
# an earlier configure of this build directory kept its cached
# BUILD_BUILTIN_<NAME>=1 and stayed in builtin_config.h.
foreach(DISABLED ${BUILTINS_DISABLED})
  string(TOUPPER "${DISABLED}" NAME)
  list(REMOVE_ITEM SOURCES "src/builtin/builtin_${DISABLED}.c")
  set(BUILD_BUILTIN_${NAME}
      "0"
      CACHE INTERNAL "Build the ${DISABLED} builtin")
endforeach(DISABLED ${BUILTINS_DISABLED})

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
  set(BUILD_BUILTIN_${NAME}
      1
      CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(ENABLED ${BUILTINS_ENABLED})

dump(BUILTINS_ENABLED)
dump(BUILTIN_FLAGS)

set(BUILTIN_CONFIG "")
foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 1")
    list(APPEND BUILTIN_SOURCES "src/builtin/builtin_${BUILTIN}.c")
  else(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 0")
  endif(${BUILD_BUILTIN_${NAME}})
endforeach(BUILTIN ${ALL_BUILTINS})

file(WRITE "${CMAKE_BINARY_DIR}/src/builtin_config.h" "${BUILTIN_CONFIG}\n\n")

set_source_files_properties(
  src/builtin/builtin_table.c PROPERTIES COMPILE_DEFINITIONS
                                         HAVE_BUILTIN_CONFIG_H=1)

list(SORT BUILTINS_ENABLED)
list(SORT BUILTINS_DISABLED)

string(REPLACE ";" " " BUILTINS_ENABLED "${BUILTINS_ENABLED}")
string(REPLACE ";" " " BUILTINS_DISABLED "${BUILTINS_DISABLED}")

function(make_list OUTPUT_VAR MAX_LINE_LEN)
  set(${OUTPUT_VAR}
      ""
      PARENT_SCOPE)
  string(REPLACE " " ";" ARGS "${ARGN}")
  set(OUTPUT "${${OUTPUT_VAR}}")
  set(LINE "")
  foreach(ITEM ${ARGS})
    string(LENGTH "${LINE} ${ITEM}" LEN)
    if(LEN GREATER MAX_LINE_LEN)
      set(OUTPUT "${OUTPUT}\n--  ${LINE}")
      set(LINE " ${ITEM}")
      string(LENGTH "${LINE}" LEN)
    else(LEN GREATER MAX_LINE_LEN)
      set(LINE "${LINE} ${ITEM}")
    endif(LEN GREATER MAX_LINE_LEN)
  endforeach(ITEM ${ARGN})
  if(LINE)
    set(OUTPUT "${OUTPUT}\n--  ${LINE}")
  endif(LINE)
  set("${OUTPUT_VAR}"
      "${OUTPUT}"
      PARENT_SCOPE)
endfunction(make_list OUTPUT_VAR)

make_list(BUILTINS_ENABLED_LIST 80 ${BUILTINS_ENABLED})

message(STATUS "Enabled builtins: ${BUILTINS_ENABLED_LIST}")
if(BUILTINS_DISABLED)
  make_list(BUILTINS_DISABLED_LIST 80 ${BUILTINS_DISABLED})
  message(STATUS "Disabled builtins: ${BUILTINS_DISABLED}")
endif(BUILTINS_DISABLED)
