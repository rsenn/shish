include(${CMAKE_CURRENT_LIST_DIR}/Functions.cmake)

# the source tree this file belongs to (not CMAKE_SOURCE_DIR: tests/cmake-builtins.sh includes it from a scratch project)
get_filename_component(BUILTINS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

#
# configure_shish_builtins
#
macro(configure_shish_builtins)
  # src/builtin/builtins.map is the one list of builtins: name, source, tier, extra sources
  file(STRINGS "${BUILTINS_SOURCE_DIR}/src/builtin/builtins.map" BUILTIN_MAP REGEX "^[^#]")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${BUILTINS_SOURCE_DIR}/src/builtin/builtins.map")

  set(ALL_BUILTINS "")
  set(MAP_MINIMAL "")
  set(MAP_DEFAULT "")
  set(MAP_EXTRA "")
  set(BUILTIN_MANAGED "")

  foreach(LINE ${BUILTIN_MAP})
    string(REGEX REPLACE "[ \t]+" ";" FIELDS "${LINE}")
    list(LENGTH FIELDS NFIELDS)
    if(NFIELDS LESS 4 OR NFIELDS GREATER 5)
      message(FATAL_ERROR "src/builtin/builtins.map: expected 'name file tier needs [keep]': ${LINE}")
    endif()
    list(GET FIELDS 0 MAP_NAME)
    list(GET FIELDS 1 MAP_FILE)
    list(GET FIELDS 2 MAP_TIER)
    list(GET FIELDS 3 MAP_NEEDS)
    string(TOUPPER ${MAP_NAME} MAP_UNAME)

    # every source of the line, as an absolute path (what file(GLOB) returns)
    set(MAP_FILES "")
    string(REPLACE "," ";" MAP_NEEDS "${MAP_NEEDS}")
    foreach(REL ${MAP_FILE} ${MAP_NEEDS})
      if("${REL}" MATCHES "/$")
        # a directory: every .c in it
        get_filename_component(ABS "${BUILTINS_SOURCE_DIR}/src/builtin/${REL}" ABSOLUTE)
        file(GLOB DIR_SOURCES "${ABS}/*.c")
        list(APPEND MAP_FILES ${DIR_SOURCES})
      elseif(NOT "${REL}" STREQUAL "-")
        get_filename_component(ABS "${BUILTINS_SOURCE_DIR}/src/builtin/${REL}" ABSOLUTE)
        list(APPEND MAP_FILES "${ABS}")
      endif()
    endforeach()
    set(BUILTIN_FILES_${MAP_UNAME} "${MAP_FILES}")
    if(NFIELDS EQUAL 5)
      set(BUILTIN_FILES_${MAP_UNAME} "") # "keep": compiled whatever the switch says
    else()
      list(APPEND BUILTIN_MANAGED ${MAP_FILES})
    endif()
    list(APPEND ALL_BUILTINS ${MAP_NAME})

    if("${MAP_TIER}" STREQUAL "m")
      list(APPEND MAP_MINIMAL ${MAP_NAME})
    elseif("${MAP_TIER}" STREQUAL "d")
      list(APPEND MAP_DEFAULT ${MAP_NAME})
    elseif("${MAP_TIER}" STREQUAL "x")
      list(APPEND MAP_EXTRA ${MAP_NAME})
    else()
      message(FATAL_ERROR "src/builtin/builtins.map: tier must be m, d or x: ${LINE}")
    endif()
  endforeach()

  list(REMOVE_DUPLICATES BUILTIN_MANAGED)

  set_init(MINIMAL_BUILTINS ${MAP_MINIMAL})
  set_init(EXTRA_BUILTINS ${MAP_EXTRA})
  set_init(DEFAULT_BUILTINS ${MINIMAL_BUILTINS} ${MAP_DEFAULT})
  list(SORT ALL_BUILTINS)

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
      set_add(BUILTINS_ENABLED ${BUILTIN})
    else()
      set_add(BUILTINS_DISABLED ${BUILTIN})
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

  # The sources of the enabled switches: each line's file plus its "needs". Every source the map
  # names is dropped from the src/*/*.c glob in CMakeLists.txt and comes back only from here.
  foreach(ENABLED ${BUILTINS_ENABLED})
    string(TOUPPER "${ENABLED}" NAME)
    set_add(BUILTIN_SOURCES ${BUILTIN_FILES_${NAME}})
    set(BUILTIN_FLAGS "${BUILTIN_FLAGS} -DBUILTIN_${NAME}=1")
    set(BUILD_BUILTIN_${NAME} "1" CACHE INTERNAL "Build the ${ENABLED} builtin")
  endforeach()

  # NAME has to come from ${DISABLED}: without it the loop reuses whatever NAME
  # the previous foreach left behind, so a builtin that was enabled in an earlier
  # configure of this build directory kept its cached BUILD_BUILTIN_<NAME>=1 and
  # stayed in builtin_config.h.
  foreach(DISABLED ${BUILTINS_DISABLED})
    string(TOUPPER "${DISABLED}" NAME)
    set(BUILD_BUILTIN_${NAME} "0" CACHE INTERNAL "Build the ${DISABLED} builtin")
  endforeach()

  set(BUILTIN_CONFIG "")
  foreach(BUILTIN ${ALL_BUILTINS})
    string(TOUPPER ${BUILTIN} NAME)
    if(${BUILD_BUILTIN_${NAME}})
      set(BUILTIN_CONFIG "${BUILTIN_CONFIG}\n#define BUILTIN_${NAME} 1")
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

endmacro()
