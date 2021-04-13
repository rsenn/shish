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
  getopts
  hash
  history
  local
  pwd
  read
  readonly
  return
  set
  shift
  source
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
  expr
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
  pwd
  set
  shift
  source
  test
  true
  type
  unset)
list(SORT ALL_BUILTINS)
list(REMOVE_DUPLICATES ALL_BUILTINS)

set(BUILTINS_ENABLED "")
set(BUILTINS_DISABLED "")

option(ENABLE_ALL_BUILTINS "Enable all builtins" OFF)

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

variable_watch(ENABLE_ALL_BUILTINS ON_ENABLE_ALL_BUILTINS)

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(NOT DEFINED ${ENABLE_${NAME}})
    isin("ENABLE_${NAME}" ${BUILTIN} ${DEFAULT_BUILTINS})
  endif(NOT DEFINED ${ENABLE_${NAME}})

  if(ENABLE_ALL_BUILTINS)
    option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" ON)
    set(BUILTIN_${NAME}
        TRUE
        CACHE BOOL "Enable ${BUILTIN} builtin" FORCE)
  else(ENABLE_ALL_BUILTINS)

    if(DEFINED ENABLE_${NAME})
      option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" "${ENABLE_${NAME}}")
    else(DEFINED ENABLE_${NAME})
      option(BUILTIN_${NAME} "Enable ${BUILTIN} builtin" OFF)
    endif(DEFINED ENABLE_${NAME})
  endif(ENABLE_ALL_BUILTINS)
endforeach(BUILTIN ${ALL_BUILTINS})

if(ENABLE_ALL_BUILTINS)
  message(STATUS "Enable all builtins")
  foreach(BUILTIN ${ALL_BUILTINS})
    string(TOUPPER ${BUILTIN} NAME)
    set(ENABLE_${NAME} ON)
    set(BUILTIN_${NAME} ON)
  endforeach(BUILTIN ${ALL_BUILTINS})
endif(ENABLE_ALL_BUILTINS)

if(BUILD_DEBUG)
  set(ENABLE_DUMP TRUE)
  # list(APPEND BUILTINS_ENABLED dump)
endif(BUILD_DEBUG)

isin(ENABLE_DUMP dump ${BUILTINS_ENABLED})

set(DEBUG_OUTPUT_DEFAULT OFF)

if(ENABLE_DUMP)
  set(DEBUG_OUTPUT_DEFAULT ON)
endif(ENABLE_DUMP)

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
    list(APPEND BUILTINS_ENABLED ${BUILTIN})
  else(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
    list(APPEND BUILTINS_DISABLED ${BUILTIN})
  endif(${BUILTIN_${NAME}} STREQUAL ON OR ENABLE_ALL_BUILTINS)
endforeach(BUILTIN ${ALL_BUILTINS})

foreach(BUILTIN ${BUILTINS_ENABLED})
  list(APPEND SOURCES src/builtin/builtin_${BUILTIN}.c)
endforeach(BUILTIN ${BUILTINS_ENABLED})

foreach(DISABLED ${BUILTINS_DISABLED})
  set(SRC "src/builtin/builtin_${DISABLED}.c")
  list(LENGTH SOURCES N)
  list(REMOVE_ITEM SOURCES "${SRC}")
  list(LENGTH SOURCES N2)
  unset(BUILTIN_${NAME})
  set(BUILD_BUILTIN_${NAME}
      "0"
      CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(DISABLED ${BUILTINS_DISABLED})

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
  set(BUILD_BUILTIN_${NAME}
      1
      CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(ENABLED ${BUILTINS_ENABLED})

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
