# include(CheckIncludeFile) include(CheckSymbolExists) include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

macro(add_cflags ADD)
  if(NOT ARGN)
    set(VAR_NAME CMAKE_C_FLAGS)
  else(NOT ARGN)
    set(VAR_NAME "${ARGN}")
  endif(NOT ARGN)
  set(C_FLAGS ${CMAKE_C_FLAGS} ${${VAR_NAME}})
  string(REGEX REPLACE " +" ";" C_FLAGS "${C_FLAGS}")
  string(REGEX REPLACE "^;+" "" C_FLAGS "${C_FLAGS}")
  list(REMOVE_DUPLICATES C_FLAGS)
  # message("VAR_NAME: ${VAR_NAME}") message("C_FLAGS: ${C_FLAGS}")
  list(FIND C_FLAGS "${ADD}" FOUND_AT)
  # message("FOUND_AT: ${FOUND_AT}") if("${C_FLAGS}" MATCHES "${ADD}")
  if(NOT "${FOUND_AT}" MATCHES "-1")

  else()
    set("${VAR_NAME}" "${${VAR_NAME}} ${ADD}")
  endif()
endmacro(add_cflags)

function(check_cflag FLAG VAR)
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_c_compiler_flag("${FLAG}" "${VAR}")
  set(CMAKE_REQUIRED_QUIET FALSE)
  if(NOT ARGN)
    set(VAR_NAME CMAKE_C_FLAGS)
  else(NOT ARGN)
    string(TOUPPER "CMAKE_C_FLAGS_${ARGN}" VAR_NAME)
  endif(NOT ARGN)
  # message("VAR_NAME: ${VAR_NAME}")
  if(${VAR})
    message(STATUS "Compiler flag ${FLAG} ... supported")
    add_cflags("${FLAG}" "${VAR_NAME}")
  else(${VAR})
    message(STATUS "Compiler flag ${FLAG} ... not supported")
  endif(${VAR})
endfunction(
  check_cflag
  FLAG
  VAR)

function(DUMP VAR)
  message("\n\nVariable dump of: " ${ARGV} "\n")
  foreach(VAR ${ARGV})
    string(REGEX REPLACE ";" "\n" VALUE "${${VAR}}")
    message("\t${VAR} = ${VALUE}")
  endforeach(VAR ${ARGV})
  message("\n")
endfunction(DUMP VAR)

macro(check_inline)
  foreach(KEYWORD "__inline__" "__inline" "inline")
    if(NOT INLINE_KEYWORD)
      set(CMAKE_REQUIRED_DEFINITIONS "-DTESTKEYWORD=${KEYWORD}")
      check_c_source_compiles(
        "typedef int foo_t;
  static TESTKEYWORD foo_t static_foo(){return 0;}
  foo_t foo(){return 0;}
  int main(int argc, char *argv[]){return 0;}"
        HAVE_${KEYWORD})
      if(HAVE_${KEYWORD})
        set(INLINE_KEYWORD "${KEYWORD}")
      endif(HAVE_${KEYWORD})
    endif(NOT INLINE_KEYWORD)
  endforeach(KEYWORD)
  message(STATUS "inline keyword: ${INLINE_KEYWORD}")
endmacro(check_inline)

function(isin)
  set(ARGUMENTS "${ARGV}")
  list(GET ARGUMENTS 0 RETVAR)
  list(GET ARGUMENTS 1 ITEM)
  list(REMOVE_AT ARGUMENTS 0 1)
  # message("ARGUMENTS: ${ARGUMENTS}")
  list(FIND ARGUMENTS "${ITEM}" FOUND)

  if(${FOUND} EQUAL -1)
    set(RET FALSE)
  else()
    set(RET TRUE)
  endif()
  set("${RETVAR}"
      ${RET}
      PARENT_SCOPE)
  # return(${RET})
endfunction(isin)

macro(show_result RESULT_VAR)
  if(${${RESULT_VAR}})
    set(RESULT_VALUE yes)
  else(${${RESULT_VAR}})
    set(RESULT_VALUE no)
  endif(${${RESULT_VAR}})
endmacro(show_result RESULT_VAR)

macro(check_compile RESULT_VAR SOURCE)
  set(RESULT "${${RESULT_VAR}}")
  # message("${RESULT_VAR} = ${RESULT}" )
  if(RESULT STREQUAL "")
    string(
      RANDOM
      LENGTH 6
      ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" C_NAME)
    string(REPLACE SUPPORT_ "" NAME "${RESULT_VAR}")
    string(REPLACE _ - NAME "${NAME}")
    string(TOLOWER "${NAME}" C_NAME)
    set(C_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/try-${C_NAME}.c")
    string(REPLACE "\\" "\\\\" SOURCE "${SOURCE}")
    file(WRITE "${C_SOURCE}" "${SOURCE}")
    message(STATUS "Trying to compile try-${C_NAME}.c ... ")
    try_compile(
      COMPILE_RESULT "${CMAKE_CURRENT_BINARY_DIR}"
      "${C_SOURCE}"
      OUTPUT_VARIABLE "OUTPUT"
      LINK_LIBRARIES "${ARGN}")
    file(REMOVE "${C_SOURCE}")

    if(COMPILE_RESULT)
      message(STATUS "ok")
      # add_definitions(-D${RESULT_VAR})
    else(COMPILE_RESULT)
      set(COMPILE_LOG "${CMAKE_CURRENT_BINARY_DIR}/compile-${C_NAME}.log")
      message(STATUS "fail: ${COMPILE_LOG}")
      file(WRITE "${COMPILE_LOG}" "${OUTPUT}")
      string(REPLACE "\n" ";" OUTPUT "${OUTPUT}")
      list(FILTER OUTPUT INCLUDE REGEX "error")
    endif(COMPILE_RESULT)

    set("${RESULT_VAR}"
        "${COMPILE_RESULT}"
        CACHE BOOL "Support ${NAME}")
  endif(RESULT STREQUAL "")
  show_result(${RESULT_VAR})
endmacro()

function(RELATIVE_PATH OUT_VAR RELATIVE_TO)
  set(LIST "")

  foreach(ARG ${ARGN})
    file(RELATIVE_PATH ARG "${RELATIVE_TO}" "${ARG}")
    list(APPEND LIST "${ARG}")
  endforeach(ARG ${ARGN})

  set("${OUT_VAR}"
      "${LIST}"
      PARENT_SCOPE)
endfunction(
  RELATIVE_PATH
  RELATIVE_TO
  OUT_VAR)

function(BASENAME OUTPUT_VAR STR)
  string(REGEX REPLACE ".*/" "" TMP_STR "${STR}")
  if(ARGN)
    string(REGEX REPLACE "\\${ARGN}\$" "" TMP_STR "${TMP_STR}")
  endif(ARGN)

  set("${OUTPUT_VAR}"
      "${TMP_STR}"
      PARENT_SCOPE)
endfunction(
  BASENAME
  OUTPUT_VAR
  FILE)

macro(CHECK_FUNCTION_DEF FUNC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
    string(TOUPPER "HAVE_${FUNC}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)
  check_function_exists("${FUNC}" "${RESULT_VAR}")
  if(${${RESULT_VAR}})
    set("${RESULT_VAR}"
        TRUE
        CACHE BOOL "Define this if you have the '${FUNC}' function")
    if(NOT "${PREPROC_DEF}" STREQUAL "")
      add_definitions(-D${PREPROC_DEF})
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})
endmacro(CHECK_FUNCTION_DEF FUNC)

macro(CHECK_FUNCTIONS)
  foreach(FUNC ${ARGN})
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
    check_function_def("${FUNC}" "${RESULT_VAR}")
  endforeach(FUNC ${ARGN})
endmacro(CHECK_FUNCTIONS)

macro(CHECK_FUNCTIONS_DEF)
  foreach(FUNC ${ARGN})
    check_function_def("${FUNC}")
  endforeach(FUNC ${ARGN})
endmacro(CHECK_FUNCTIONS_DEF)

macro(DEBUG_FLAG NAME DESC)
  option(DEBUG_${NAME} "${DESC}" OFF)

  if(DEBUG_${NAME})
    add_definitions(-DDEBUG_${NAME}=1)
  endif()
endmacro(
  DEBUG_FLAG
  NAME
  DESC)
