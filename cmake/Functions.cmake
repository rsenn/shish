include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

#
# add_cflags <ADD> [VAR_NAME]
#
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
  list(FIND C_FLAGS "${ADD}" FOUND_AT)
  
  if(NOT "${FOUND_AT}" MATCHES "-1")
  else()
    set("${VAR_NAME}" "${${VAR_NAME}} ${ADD}")
  endif()
endmacro(add_cflags)

#
# check_cflag <FLAG> <VAR> [VAR_NAME]
#
function(check_cflag FLAG VAR)
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_c_compiler_flag("${FLAG}" "${VAR}")
  set(CMAKE_REQUIRED_QUIET FALSE)
  if(NOT ARGN)
    set(VAR_NAME CMAKE_C_FLAGS)
  else(NOT ARGN)
    string(TOUPPER "CMAKE_C_FLAGS_${ARGN}" VAR_NAME)
  endif(NOT ARGN)
  if(${VAR})
    message(STATUS "Compiler flag ${FLAG} ... supported")
    add_cflags("${FLAG}" "${VAR_NAME}")
  else(${VAR})
    message(STATUS "Compiler flag ${FLAG} ... not supported")
  endif(${VAR})
endfunction(check_cflag FLAG VAR)

# Append FLAG to CMAKE_EXE_LINKER_FLAGS if a test executable links with it. A macro, not a function, so the result reaches the calling scope.
#
# check_ldflags <FLAG> <VAR>
#
# FLAG     the linker flag, driver-style ("-Wl,--gc-sections") VAR      cache variable the probe result is stored in
# -----------------------------------------------------------------------
macro(check_ldflag FLAG VAR)
  set(CHECK_LDFLAG_SAVED "${CMAKE_EXE_LINKER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_c_source_compiles("int main(void) { return 0; }" ${VAR})
  set(CMAKE_REQUIRED_QUIET FALSE)
  set(CMAKE_EXE_LINKER_FLAGS "${CHECK_LDFLAG_SAVED}")
  if(${VAR})
    message(STATUS "Linker flag ${FLAG} ... supported")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
  else(${VAR})
    message(STATUS "Linker flag ${FLAG} ... not supported")
  endif(${VAR})
endmacro(check_ldflag)

#
# dump [VAR-NAMES...]
#
function(dump VAR)
  message("\n\nVariable dump of: " ${ARGV} "\n")
  foreach(VAR ${ARGV})
    string(REGEX REPLACE ";" "\n" VALUE "${${VAR}}")
    message("\t${VAR} = ${VALUE}")
  endforeach(VAR ${ARGV})
  message("\n")
endfunction(dump VAR)

#
# check_inline [OUTPUT-VAR]
#
function(check_inline)
  if(ARGN)
    set(OUTPUT_VAR ${ARGN})
  else(ARGN)
    set(OUTPUT_VAR INLINE_KEYWORD)
  endif(ARGN)

  foreach(KEYWORD "__inline__" "__inline" "inline")
    if(NOT INLINE_KEYWORD)
      set(CMAKE_REQUIRED_DEFINITIONS "-DTESTKEYWORD=${KEYWORD}")
      check_c_source_compiles("typedef int foo_t;\nstatic TESTKEYWORD foo_t static_foo(){return 0;}\nfoo_t foo(){return 0;}\nint main(int argc, char *argv[]){return 0;}\n" HAVE_${KEYWORD})
      if(HAVE_${KEYWORD})
        set("${OUTPUT_VAR}" "${KEYWORD}" PARENT_SCOPE)
      endif(HAVE_${KEYWORD})
    endif(NOT INLINE_KEYWORD)
  endforeach(KEYWORD)

  message(STATUS "inline keyword: ${${OUTPUT_VAR}}")
endfunction(check_inline)

#
# isin <RESULT_VAR> <ITEM> [LIST...]
#
function(isin)
  set(ARGUMENTS "${ARGV}")
  list(GET ARGUMENTS 0 RESULT_VAR)
  list(GET ARGUMENTS 1 ITEM)
  list(REMOVE_AT ARGUMENTS 0 1)
  # message("ARGUMENTS: ${ARGUMENTS}")
  list(FIND ARGUMENTS "${ITEM}" FOUND)

  if(${FOUND} EQUAL -1)
    set(RET FALSE)
  else()
    set(RET TRUE)
  endif()
  set("${RESULT_VAR}" ${RET} PARENT_SCOPE)
  # return(${RET})
endfunction(isin)

#
# show_result <RESULT_VAR>
#
macro(show_result RESULT_VAR)
  if(${${RESULT_VAR}})
    set(RESULT_VALUE yes)
  else(${${RESULT_VAR}})
    set(RESULT_VALUE no)
  endif(${${RESULT_VAR}})
endmacro(show_result RESULT_VAR)

#
# check_compile <RESULT_VAR> <SOURCE>
#
macro(check_compile RESULT_VAR SOURCE)
  set(RESULT "${${RESULT_VAR}}")

  if(RESULT STREQUAL "")
    string(RANDOM LENGTH 6 ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" C_NAME)
    string(REPLACE SUPPORT_ "" NAME "${RESULT_VAR}")
    string(REPLACE _ - NAME "${NAME}")
    string(TOLOWER "${NAME}" C_NAME)
    set(C_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/try-${C_NAME}.c")
    string(REPLACE "\\" "\\\\" SOURCE "${SOURCE}")
    file(WRITE "${C_SOURCE}" "${SOURCE}")
    message(STATUS "Trying to compile try-${C_NAME}.c ... ")
    try_compile(COMPILE_RESULT "${CMAKE_CURRENT_BINARY_DIR}" "${C_SOURCE}" OUTPUT_VARIABLE "OUTPUT" LINK_LIBRARIES "${ARGN}")
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

    set("${RESULT_VAR}" "${COMPILE_RESULT}" CACHE BOOL "Support ${NAME}")
  endif(RESULT STREQUAL "")
  show_result(${RESULT_VAR})
endmacro()

#
# check_run <RESULT_VAR> <SOURCE>
#
macro(check_run RESULT_VAR SOURCE)
  set(RESULT "${${RESULT_VAR}}")

  if(RESULT STREQUAL "")
    string(RANDOM LENGTH 6 ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" C_NAME)
    string(REPLACE SUPPORT_ "" NAME "${RESULT_VAR}")
    string(REPLACE _ - NAME "${NAME}")
    string(TOLOWER "${NAME}" C_NAME)
    set(C_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/try-${C_NAME}.c")
    string(REPLACE "\\" "\\\\" SOURCE "${SOURCE}")
    file(WRITE "${C_SOURCE}" "${SOURCE}")
    message(STATUS "Trying to compile try-${C_NAME}.c ... ")
    try_run(RUN_RESULT COMPILE_RESULT "${CMAKE_CURRENT_BINARY_DIR}" "${C_SOURCE}" COMPILE_OUTPUT_VARIABLE "OUTPUT" LINK_LIBRARIES "${ARGN}")
    file(REMOVE "${C_SOURCE}")

    if(COMPILE_RESULT AND RUN_RESULT)
      message(STATUS "ok")
      # add_definitions(-D${RESULT_VAR})
    else(COMPILE_RESULT AND RUN_RESULT)
      set(COMPILE_LOG "${CMAKE_CURRENT_BINARY_DIR}/compile-${C_NAME}.log")
      message(STATUS "fail: ${COMPILE_LOG}")
      file(WRITE "${COMPILE_LOG}" "${OUTPUT}")
      string(REPLACE "\n" ";" OUTPUT "${OUTPUT}")
      list(FILTER OUTPUT INCLUDE REGEX "error")
    endif(COMPILE_RESULT AND RUN_RESULT)

    set("${RESULT_VAR}" "${COMPILE_RESULT}" CACHE BOOL "Support ${NAME}")
  endif(RESULT STREQUAL "")
  show_result(${RESULT_VAR})
endmacro()

#
# relative_path <OUTPUT_VAR> <RELATIVE_TO> [ARGUMENTS...]
#
function(relative_path OUTPUT_VAR RELATIVE_TO)
  set(LIST "")

  foreach(ARG ${ARGN})
    file(RELATIVE_PATH ARG "${RELATIVE_TO}" "${ARG}")
    list(APPEND LIST "${ARG}")
  endforeach(ARG ${ARGN})

  set("${OUTPUT_VAR}" "${LIST}" PARENT_SCOPE)
endfunction(relative_path RELATIVE_TO OUTPUT_VAR)

#
# basename <OUTPUT_VAR> <STR> [EXT_NAME]
#
function(basename OUTPUT_VAR STR)
  string(REGEX REPLACE ".*/" "" TMP_STR "${STR}")
  if(ARGN)
    string(REGEX REPLACE "\\${ARGN}\$" "" TMP_STR "${TMP_STR}")
  endif(ARGN)
  set("${OUTPUT_VAR}" "${TMP_STR}" PARENT_SCOPE)
endfunction(basename OUTPUT_VAR FILE)

#
# var2define <NAME> [DEFINED_VALUE] [VAR_NAME]
#
function(var2define NAME)
  if("${ARGC}" GREATER 2)
    list(GET ARGN 1 VAR_NAME)
  else("${ARGC}" GREATER 2)
    set(VAR_NAME "${NAME}")
  endif("${ARGC}" GREATER 2)

  set(VALUE "${${VAR_NAME}}")

  if("${ARGC}" LESS_EQUAL 1)
    if("${VALUE}")
      add_definitions(-D${NAME}=1)
    else("${VALUE}")
      add_definitions(-D${NAME}=0)
    endif("${VALUE}")
  else("${ARGC}" LESS_EQUAL 1)
    if("${VALUE}")
      list(GET ARGN 0 DEFINED_VALUE)
      add_definitions(-D${NAME}=${DEFINED_VALUE})
    endif("${VALUE}")
  endif("${ARGC}" LESS_EQUAL 1)
endfunction(var2define NAME)

#
# check_include_def <INCLUDE> [RESULT-VAR] [PREPROC_DEF]
#
macro(check_include_def INC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    clean_name("${INC}" INC_D)
    string(TOUPPER "HAVE_${INC_D}" RESULT_VAR)
    string(TOUPPER "HAVE_${INC_D}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)

  check_include_file("${INC}" "${RESULT_VAR}")

  if(${${RESULT_VAR}})
    set("${RESULT_VAR}" TRUE CACHE INTERNAL "Define this if you have the '${INC}' header file")

    if(NOT "${PREPROC_DEF}" STREQUAL "")
      var2define("${PREPROC_DEF}" 1)
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})

  list(APPEND CHECKED_INCLUDES "${INC}")
endmacro(check_include_def INC)

#
# check_includes <INCLUDE-FILES...>
#
macro(check_includes)
  foreach(INC ${ARGN})
    clean_name("HAVE_${INC}" RESULT_VAR)
    check_include_def("${INC}" "${RESULT_VAR}")
  endforeach(INC ${ARGN})
endmacro(check_includes)

#
# check_includes_def <INCLUDE-FILES...>
#
macro(check_includes_def)
  foreach(INC ${ARGN})
    check_include_def("${INC}")
  endforeach(INC ${ARGN})
endmacro(check_includes_def)

#
# clean_name <STRING> <OUTPUT-VAR>
#
function(clean_name STR OUTPUT_VAR)
  string(TOUPPER "${STR}" STR)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" STR "${STR}")
  set("${OUTPUT_VAR}" "${STR}" PARENT_SCOPE)
endfunction(clean_name STR OUTPUT_VAR)

#
# check_include_def <INCLUDE> [RESULT-VAR] [PREPROC_DEF]
#
macro(check_include_def INC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    clean_name("${INC}" INC_D)
    string(TOUPPER "HAVE_${INC_D}" RESULT_VAR)
    string(TOUPPER "HAVE_${INC_D}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)

  check_include_file("${INC}" "${RESULT_VAR}")

  if(${${RESULT_VAR}})
    set("${RESULT_VAR}" TRUE CACHE INTERNAL "Define this if you have the '${INC}' header file")

    if(NOT "${PREPROC_DEF}" STREQUAL "")
      var2define("${PREPROC_DEF}" 1)
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})

  list(APPEND CHECKED_INCLUDES "${INC}")
endmacro(check_include_def INC)

#
# check_includes <INCLUDE-FILES...>
#
macro(check_includes)
  foreach(INC ${ARGN})
    clean_name("HAVE_${INC}" RESULT_VAR)
    check_include_def("${INC}" "${RESULT_VAR}")
  endforeach(INC ${ARGN})
endmacro(check_includes)

#
# check_includes_def <INCLUDE-FILES...>
#
macro(check_includes_def)
  foreach(INC ${ARGN})
    check_include_def("${INC}")
  endforeach(INC ${ARGN})
endmacro(check_includes_def)

#
# check_function_def <FUNC> [RESULT_VAR] [PREPROC_DEF]
#
macro(check_function_def FUNC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
    string(TOUPPER "HAVE_${FUNC}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)
  check_function_exists("${FUNC}" "${RESULT_VAR}")
  if(${${RESULT_VAR}})
    set("${RESULT_VAR}" TRUE CACHE BOOL "Define this if you have the '${FUNC}' function")
    if(NOT "${PREPROC_DEF}" STREQUAL "")
      add_definitions(-D${PREPROC_DEF})
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})
endmacro(check_function_def FUNC)

#
# check_functions [FUNCTION-NAMES...]
#
macro(check_functions)
  foreach(FUNC ${ARGN})
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
    check_function_def("${FUNC}" "${RESULT_VAR}")
  endforeach(FUNC ${ARGN})
endmacro(check_functions)

#
# check_functions_def [FUNCTION-NAMES...]
#
macro(check_functions_def)
  foreach(FUNC ${ARGN})
    check_function_def("${FUNC}")
  endforeach(FUNC ${ARGN})
endmacro(check_functions_def)

#
# debug_flag <NAME> <DESC>
#
macro(debug_flag NAME DESC)
  option(DEBUG_${NAME} "${DESC}" OFF)

  if(DEBUG_${NAME})
    add_definitions(-DDEBUG_${NAME}=1)
  endif()
endmacro(debug_flag NAME DESC)

#
# check_function_and_include <FUNCTION> <INCLUDE>
#
macro(check_function_and_include FUNC INC)
  clean_name("HAVE_${INC}" INC_RESULT)
  clean_name("HAVE_${FUNC}" FUNC_RESULT)

  check_include_def("${INC}" "${INC_RESULT}" "${INC_RESULT}")

  if(${${INC_RESULT}})
    check_function_def("${FUNC}" "${FUNC_RESULT}" "${FUNC_RESULT}")
  endif(${${INC_RESULT}})
endmacro(check_function_and_include FUNC INC)

# Get number of columns the terminal supports
#
# get_columns <OUTPUT_VAR> [DEFAULT]
#
function(get_columns OUTPUT_VAR)
  if(ARGN)
    set(DEFAULT_VALUE ${ARGN})
  else(ARGN)
    set(DEFAULT_VALUE 80)
  endif(ARGN)
  set(VALUE "$ENV{COLUMNS}")
  if("${VALUE}" OR NOT "${VALUE}" STREQUAL "")
    message("Got COLUMNS (${VALUE}) from environment")
  else("${VALUE}" OR NOT "${VALUE}" STREQUAL "")
    execute_process(COMMAND tput cols OUTPUT_VARIABLE TPUT_COLS ERROR_QUIET ERROR_VARIABLE TPUT_ERROR)
    set(TPUT_ERROR TRUE)
    if(NOT TPUT_ERROR AND TPUT_COLS)
      set(VALUE ${TPUT_COLS})
    else(NOT TPUT_ERROR AND TPUT_COLS)
      set(SOURCE_NAME "get-tty-size.c")
      set(SOURCE_CODE "#include <unistd.h>\n#include <fcntl.h>\n#include <termios.h>\n#include <sys/ioctl.h>\n#include <stdio.h>\n\nint\nmain() {\n\tstruct winsize sz;\n\tint fd = isatty(0) ? dup(0) : open(\"/dev/tty\", O_RDWR);\n\n\tif(!isatty(fd)) {\n\t\tfputs(\"not a tty\\n\", stderr);\n\t\tfflush(stderr);\n\t\treturn 1;\n\t}\n\n\tif(ioctl(fd, TIOCGWINSZ, &sz) == -1) {\n\t\tperror(\"ioctl\");\n\t\treturn 1;\n\t}\n\n\tclose(fd);\n\n\tprintf(\"%u\\n\", sz.ws_col);\n\treturn 0;\n}\n")
      try_run(RUN_RESULT COMPILE_RESULT 
        SOURCE_FROM_CONTENT "${SOURCE_NAME}" "${SOURCE_CODE}"
        RUN_OUTPUT_STDOUT_VARIABLE RUN_OUTPUT
        COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
        NO_CACHE
      )
      if(NOT COMPILE_RESULT)
        message(STATUS "${SOURCE_NAME} failed to compile:\n${COMPILE_OUTPUT}")
      else(NOT COMPILE_RESULT)
        if(RUN_RESULT STREQUAL 0)
          #message(STATUS "${SOURCE_NAME} succeeded to run: ${RUN_OUTPUT}")
          set(VALUE "${RUN_OUTPUT}")
        else(RUN_RESULT STREQUAL 0)
          message(STATUS "${SOURCE_NAME} failed to run:\n${RUN_OUTPUT}")
        endif(RUN_RESULT STREQUAL 0)   
      endif(NOT COMPILE_RESULT)
    endif(NOT TPUT_ERROR AND TPUT_COLS)
  endif("${VALUE}" OR NOT "${VALUE}" STREQUAL "")
  if("${VALUE}" STREQUAL "")
    set(VALUE "${DEFAULT_VALUE}")
    message("Defaulting ${OUTPUT_VAR} to ${DEFAULT_VALUE}")
  endif("${VALUE}" STREQUAL "")
  set("${OUTPUT_VAR}" "${VALUE}" PARENT_SCOPE)
endfunction(get_columns OUTPUT_VAR)

#
# make_list <OUTPUT_VAR> <MAX_LINE_LEN>
#
function(make_list OUTPUT_VAR MAX_LINE_LEN)
  set(${OUTPUT_VAR} "" PARENT_SCOPE)
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

  set("${OUTPUT_VAR}" "${OUTPUT}" PARENT_SCOPE)
endfunction(make_list OUTPUT_VAR)

#
# check_source_definitions [TYPES...]
#
macro(check_source_definitions)
  set(SOURCE_DEFINITIONS)
  if(ARGN)
    set(SOURCE_TYPES ${ARGN})
  else(ARGN)
    set(SOURCE_TYPES ATFILE GNU LARGEFILE LARGE_FILE LARGEFILE64 POSIX POSIX_C XOPEN XOPEN_EXTENDED)
  endif(ARGN)

  foreach(S ${SOURCE_TYPES})
    list(APPEND SOURCE_DEFINITIONS -D_${S}_SOURCE)
  endforeach(S ${SOURCE_TYPES})

  add_definitions(${SOURCE_DEFINITIONS})
endmacro(check_source_definitions)
