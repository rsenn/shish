list(
  APPEND
  MINIMAL_BUILTINS
  break
  cd
  eval
  exec
  exit
  export
  hash
  history
  pwd
  read
  readonly
  set
  shift
  source
  unset)
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
  uname
  # ls   mv printf   sort tr  uniq
)
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

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(NOT DEFINED ${ENABLE_${NAME}})
    isin("ENABLE_${NAME}" ${BUILTIN} ${DEFAULT_BUILTINS})
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
  # message(STATUS "Builtin ${BUILTIN} - ${ENABLE_${NAME}}")
endforeach(BUILTIN ${ALL_BUILTINS})

if(ENABLE_ALL_BUILTINS)
  message("Enable ALL builtins")
  foreach(BUILTIN ${ALL_BUILTINS})
    string(TOUPPER ${BUILTIN} NAME)
    message("Enable ${BUILTIN}")
    set(ENABLE_${NAME} ON)
    set(BUILTIN_${NAME} ON)
  endforeach(BUILTIN ${ALL_BUILTINS})
endif(ENABLE_ALL_BUILTINS)

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
  # list(FILTER SOURCES EXCLUDE REGEX  "${SRC}")
  list(LENGTH SOURCES N2)
  # if(${N} EQUAL ${N2}) message("ERROR ${SRC}  -  ${N} == ${N2}") endif(${N}
  # EQUAL ${N2})
  unset(BUILTIN_${NAME})
  set(BUILD_BUILTIN_${NAME}
      "0"
      CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach(DISABLED ${BUILTINS_DISABLED})

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  # add_definitions("-DBUILTIN_${NAME}=1")
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
