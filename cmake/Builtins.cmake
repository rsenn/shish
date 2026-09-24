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
  digest
  dirname
  find
  grep
  hostname
  link
  ln
  ls
  mkdir
  readlink
  realpath
  rm
  rmdir
  sleep
  tee
  timeout
  touch
  wc
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

endfunction()

# variable_watch(ENABLE_ALL_BUILTINS ON_ENABLE_ALL_BUILTINS)

# Which builtins get built -- one switch per builtin, one for all:
#
#   -DBUILTIN_<NAME>=ON|OFF     that builtin; permanent (it stays in the cache)
#   -DENABLE_ALL_BUILTINS=ON    every builtin that has no BUILTIN_<NAME> of
#                               its own; permanent, -DENABLE_ALL_BUILTINS=OFF
#                               undoes it
#   neither                     DEFAULT_BUILTINS (dump: also in debug builds)
#
# BUILTIN_<NAME> is AUTO until somebody sets it, which is what tells an
# answer from a default -- the computed result is kept in a plain
# variable of the same name and never written back to the cache, so a
# later configure (also the one cmake starts by itself when a CMake file
# changes) reaches the same decision.
option(ENABLE_ALL_BUILTINS "Build every builtin (BUILTIN_<NAME>=OFF still wins)" OFF)

# Build directories from before this scheme cached the *computed* answer
# in BUILTIN_<NAME>; those are not choices, so forget them once. The
# BUILD_BUILTIN_* entries are written by every configure, old or new.
if(NOT BUILTINS_MODEL AND DEFINED BUILD_BUILTIN_ALIAS)
  get_cmake_property(CACHED_VARIABLES CACHE_VARIABLES)
  foreach(VAR ${CACHED_VARIABLES})
    if(VAR MATCHES "^BUILTIN_")
      unset(${VAR} CACHE)
    endif(VAR MATCHES "^BUILTIN_")
  endforeach(VAR)
endif(NOT BUILTINS_MODEL AND DEFINED BUILD_BUILTIN_ALIAS)

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  # -DENABLE_<NAME>=ON|OFF is the older spelling of BUILTIN_<NAME>
  get_property(LEGACY_SET CACHE ENABLE_${NAME} PROPERTY VALUE SET)

  if(LEGACY_SET)
    get_property(LEGACY CACHE ENABLE_${NAME} PROPERTY VALUE)
    set(BUILTIN_${NAME}
        "${LEGACY}"
        CACHE STRING "Build the ${BUILTIN} builtin: ON, OFF or AUTO" FORCE)
    unset(ENABLE_${NAME} CACHE)
  endif(LEGACY_SET)

  # no FORCE: an existing (or -D given) value is kept
  set(BUILTIN_${NAME}
      "AUTO"
      CACHE STRING "Build the ${BUILTIN} builtin: ON, OFF or AUTO")
  set_property(CACHE BUILTIN_${NAME} PROPERTY STRINGS AUTO ON OFF)

  if(NOT "${BUILTIN_${NAME}}" STREQUAL "AUTO")
    set(WANT_BUILTIN ${BUILTIN_${NAME}})
  elseif(ENABLE_ALL_BUILTINS)
    set(WANT_BUILTIN ON)
  elseif(BUILD_DEBUG AND "${BUILTIN}" STREQUAL "dump")
    set(WANT_BUILTIN ON)
  else()
    isin(WANT_BUILTIN ${BUILTIN} ${DEFAULT_BUILTINS})
  endif()

  # the answer, for this configure only (shadows the cache entry)
  if(WANT_BUILTIN)
    set(BUILTIN_${NAME} ON)
  else(WANT_BUILTIN)
    set(BUILTIN_${NAME} OFF)
  endif(WANT_BUILTIN)
endforeach(BUILTIN ${ALL_BUILTINS})

set(BUILTINS_MODEL
    2
    CACHE INTERNAL "builtin switch scheme (see above)")

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

# a builtin's source lives in src/builtin/extra/ when it is one of the
# coreutils-style / third-party utilities, else in src/builtin/
function(builtin_source OUT NAME)
  if(EXISTS "${CMAKE_SOURCE_DIR}/src/builtin/extra/builtin_${NAME}.c")
    set(${OUT} "src/builtin/extra/builtin_${NAME}.c" PARENT_SCOPE)
  else()
    set(${OUT} "src/builtin/builtin_${NAME}.c" PARENT_SCOPE)
  endif()
endfunction(builtin_source)

foreach(BUILTIN ${BUILTINS_ENABLED})
  builtin_source(BUILTIN_FILE ${BUILTIN})
  list(APPEND SOURCES ${BUILTIN_FILE})
endforeach(BUILTIN ${BUILTINS_ENABLED})

# NAME has to come from ${DISABLED}: without it the loop reuses whatever
# NAME the previous foreach left behind, so a builtin that was enabled in
# an earlier configure of this build directory kept its cached
# BUILD_BUILTIN_<NAME>=1 and stayed in builtin_config.h.
foreach(DISABLED ${BUILTINS_DISABLED})
  string(TOUPPER "${DISABLED}" NAME)
  builtin_source(BUILTIN_FILE ${DISABLED})
  list(REMOVE_ITEM SOURCES "${BUILTIN_FILE}")
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

#dump(BUILTINS_ENABLED)
#dump(BUILTIN_FLAGS)

set(BUILTIN_CONFIG "")
foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 1")
    builtin_source(BUILTIN_FILE ${BUILTIN})
    list(APPEND BUILTIN_SOURCES "${BUILTIN_FILE}")
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


dump(BUILTINS_DISABLED)

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
