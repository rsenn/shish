include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Functions.cmake)

set_list(MINIMAL_BUILTINS alias break cd command eval exec exit export expr getopts hash history jobs kill local printf pwd read readonly return set shift source test times trap type umask unset wait)
set_list(EXTRA_BUILTINS awk basename cat chmod digest dirname find grep hostname link ln ls mkdir readlink realpath rm rmdir sed sleep tee timeout touch wc which mktemp uname xargs)
set_list(DEFAULT_BUILTINS ${MINIMAL_BUILTINS} help type echo fdtable true false)

set_list(ALL_BUILTINS ${MINIMAL_BUILTINS} ${DEFAULT_BUILTINS} ${EXTRA_BUILTINS} basename break cd dirname dump echo eval exec exit export expr false fdtable hash help history hostname ln printf pwd set shift source test times true type unset)
list(SORT ALL_BUILTINS)
list(REMOVE_DUPLICATES ALL_BUILTINS)

unset(BUILTINS_ENABLED)
unset(BUILTINS_DISABLED)

# Which builtins get built -- one switch per builtin, one for all:
#
# -DBUILTIN_<NAME>=ON|OFF     that builtin; permanent (it stays in the cache)
# -DENABLE_ALL_BUILTINS=ON    every builtin that has no BUILTIN_<NAME> of its
# own; permanent, -DENABLE_ALL_BUILTINS=OFF undoes it neither DEFAULT_BUILTINS
# (dump: also in debug builds)
#
# BUILTIN_<NAME> is AUTO until somebody sets it, which is what tells an answer
# from a default -- the computed result is kept in a plain variable of the same
# name and never written back to the cache, so a later configure (also the one
# cmake starts by itself when a CMake file changes) reaches the same decision.
option(ENABLE_ALL_BUILTINS "Build every builtin (BUILTIN_<NAME>=OFF still wins)" OFF)

# Build directories from before this scheme cached the *computed* answer in
# BUILTIN_<NAME>; those are not choices, so forget them once. The
# BUILD_BUILTIN_* entries are written by every configure, old or new.
if(NOT BUILTINS_MODEL AND DEFINED BUILD_BUILTIN_ALIAS)
  get_cmake_property(CACHED_VARIABLES CACHE_VARIABLES)
  foreach(VAR ${CACHED_VARIABLES})
    if(VAR MATCHES "^BUILTIN_")
      unset(${VAR} CACHE)
    endif()
  endforeach()
endif()

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  # no FORCE: an existing (or -D given) value is kept
  set(BUILTIN_${NAME} "AUTO" CACHE STRING "Build the ${BUILTIN} builtin: ON, OFF or AUTO")
  set_property(CACHE BUILTIN_${NAME} PROPERTY STRINGS AUTO ON OFF)

  if(NOT "${BUILTIN_${NAME}}" STREQUAL "AUTO")
    set(WANT_BUILTIN ${BUILTIN_${NAME}})
  elseif(ENABLE_ALL_BUILTINS)
    set(WANT_BUILTIN ON)
  elseif(BUILD_DEBUG AND "${BUILTIN}" STREQUAL "dump")
    set(WANT_BUILTIN ON)
  else()
    #isin_list(WANT_BUILTIN ${BUILTIN} ${DEFAULT_BUILTINS})
    isin_var(WANT_BUILTIN ${BUILTIN} DEFAULT_BUILTINS)
  endif()

  # the answer, for this configure only (shadows the cache entry)
  if(WANT_BUILTIN)
    set(BUILTIN_${NAME} ON)
  else()
    set(BUILTIN_${NAME} OFF)
  endif()
endforeach()

set(BUILTINS_MODEL 2 CACHE INTERNAL "builtin switch scheme (see above)")

foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)

  # a plain truth test: isin_{list,var}() answers TRUE/FALSE and an option() answers
  # ON/OFF, and "TRUE STREQUAL ON" is false
  if(BUILTIN_${NAME})
    append_unique(BUILTINS_ENABLED ${BUILTIN})
  else()
    append_unique(BUILTINS_DISABLED ${BUILTIN})
  endif()
endforeach()

# now that BUILTINS_ENABLED is populated, "is dump being built" is a real answer
# -- and dump's output goes through the debug buffer, so building it turns
# DEBUG_OUTPUT on by default (CMakeLists.txt).
isin_var(ENABLE_DUMP dump BUILTINS_ENABLED)

set(DEBUG_OUTPUT_DEFAULT OFF)

if(ENABLE_DUMP)
  set(DEBUG_OUTPUT_DEFAULT ON)
endif()

# a builtin's source lives in src/builtin/extra/ when it is one of the
# coreutils-style / third-party utilities, else in src/builtin/
function(builtin_source OUT NAME)
  if(EXISTS "${CMAKE_SOURCE_DIR}/src/builtin/extra/builtin_${NAME}.c")
    set(RESULT "src/builtin/extra/builtin_${NAME}.c")
  else()
    set(RESULT "src/builtin/builtin_${NAME}.c")
  endif()
  set("${OUT}" "${RESULT}" PARENT_SCOPE)
endfunction()

foreach(BUILTIN ${BUILTINS_ENABLED})
  builtin_source(BUILTIN_FILE ${BUILTIN})
  append_unique(SOURCES ${BUILTIN_FILE})
endforeach()

# NAME has to come from ${DISABLED}: without it the loop reuses whatever NAME
# the previous foreach left behind, so a builtin that was enabled in an earlier
# configure of this build directory kept its cached BUILD_BUILTIN_<NAME>=1 and
# stayed in builtin_config.h.
foreach(DISABLED ${BUILTINS_DISABLED})
  string(TOUPPER "${DISABLED}" NAME)
  builtin_source(BUILTIN_FILE ${DISABLED})
  list(REMOVE_ITEM SOURCES "${BUILTIN_FILE}")
  set(BUILD_BUILTIN_${NAME} "0" CACHE INTERNAL "Build the ${DISABLED} builtin")
endforeach()

foreach(ENABLED ${BUILTINS_ENABLED})
  string(TOUPPER "${ENABLED}" NAME)
  set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
  set(BUILD_BUILTIN_${NAME} 1 CACHE INTERNAL "Build the ${ENABLED} builtin")
endforeach()

# dump(BUILTINS_ENABLED)
# dump(BUILTIN_FLAGS)

set(BUILTIN_CONFIG "")
foreach(BUILTIN ${ALL_BUILTINS})
  string(TOUPPER ${BUILTIN} NAME)
  if(${BUILD_BUILTIN_${NAME}})
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 1")
    builtin_source(BUILTIN_FILE ${BUILTIN})
    append_unique(BUILTIN_SOURCES "${BUILTIN_FILE}")
  else()
    set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 0")
  endif()
endforeach()

file(WRITE "${CMAKE_BINARY_DIR}/src/builtin_config.h" "${BUILTIN_CONFIG}\n\n")

set_source_files_properties(src/builtin/builtin_table.c 
  PROPERTIES 
  COMPILE_DEFINITIONS HAVE_BUILTIN_CONFIG_H
)

list(SORT BUILTINS_ENABLED)
list(SORT BUILTINS_DISABLED)

string(REPLACE ";" " " BUILTINS_ENABLED "${BUILTINS_ENABLED}")
string(REPLACE ";" " " BUILTINS_DISABLED "${BUILTINS_DISABLED}")

get_columns(MAX_COLUMNS)

make_list(BUILTINS_ENABLED_LIST ${MAX_COLUMNS} ${BUILTINS_ENABLED})

message(STATUS "Enabled builtins: ${BUILTINS_ENABLED_LIST}")

if(BUILTINS_DISABLED)
  make_list(BUILTINS_DISABLED_LIST ${MAX_COLUMNS} ${BUILTINS_DISABLED})
  message(STATUS "Disabled builtins: ${BUILTINS_DISABLED}")
endif()
