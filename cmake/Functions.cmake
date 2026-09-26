include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

set(ANSI_ESCAPE "")



#
# set_list <OUTPUT-VAR> [ITEMS...]
#
function(set_list OUTPUT_VAR)
  unset(RESULT)
  append_unique(RESULT ${ARGN})
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# append_unique <OUTPUT-VAR> [ITEMS...]
#
function(append_unique OUTPUT_VAR)
  set(RESULT "${${OUTPUT_VAR}}")
  foreach(ITEM ${ARGN})
    if(NOT ITEM IN_LIST RESULT)
      list(APPEND RESULT "${ITEM}")
    endif()
  endforeach()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# escape_string <OUTPUT-VAR> <STRING>
#
function(escape_string OUTPUT_VAR STRING)
    string(REPLACE "\\" "\\\\" RESULT "${STRING}")

  string(REPLACE "\n" "\\n" RESULT "${RESULT}")
  string(REPLACE "\r" "\\r" RESULT "${RESULT}")
  string(REPLACE "\t" "\\t" RESULT "${RESULT}")
  string(REPLACE "${ANSI_ESCAPE}" "\\e" RESULT "${RESULT}")
  string(REPLACE "" "\\v" RESULT "${RESULT}")
  string(REPLACE "" "\\f" RESULT "${RESULT}")

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif(OUTPUT_VAR)
endfunction()

#
# unescape_string <OUTPUT-VAR> <STRING>
#
function(unescape_string OUTPUT_VAR STRING)
  string(REPLACE "\\n" "\n" RESULT "${STRING}")
  string(REPLACE "\\r" "\r" RESULT "${RESULT}")
  string(REPLACE "\\t" "\t" RESULT "${RESULT}")
  string(REPLACE "\\x1b" "${ANSI_ESCAPE}" RESULT "${RESULT}")
  string(REPLACE "\\033" "${ANSI_ESCAPE}" RESULT "${RESULT}")
  string(REPLACE "\\e" "${ANSI_ESCAPE}"  RESULT "${RESULT}")
  string(REPLACE "\\v" ""  RESULT "${RESULT}")
  string(REPLACE "\\f" ""   RESULT "${RESULT}")

  string(REPLACE "\\\\" "\\" RESULT "${RESULT}")

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif(OUTPUT_VAR)
endfunction()

# ITEMS contains multiple items
#
# assign item #0 to VAR-NAME 0
# assign item #1 to VAR-NAME 1
# ....
#
# assign_items <ITEMS> [VAR-NAMES...]
#
macro(assign_items ITEMS)
  set(__L "${ITEMS}")
  set(__I 0)
  foreach(__A ${ARGN})
    list(GET __L "${__I}" "${__A}")
    math(EXPR __I "${__I} + 1")
 endforeach()
endmacro()

# LIST is the name of a list
#
# assign item #0 to VAR-NAME 0
# assign item #1 to VAR-NAME 1
# ....
#
# assign_list <LIST> [VAR-NAMES...]
#
macro(assign_list LIST)
  assign_items("${${LIST}}" ${ARGN})
endmacro()


#
# eat_line <VAR>
#
function(eat_line VAR_NAME OUTPUT_VAR)
  set(STR "${${VAR_NAME}}")
  string(FIND "${STR}" "\n" NL_POS)
  string(LENGTH "${STR}" LEN)
  if(${NL_POS} EQUAL -1)
    set(NL_POS "${LEN}")
    set(NEXT_POS "${NL_POS}")
  else()
    math(EXPR NEXT_POS "${NL_POS} + 1")
  endif()
  string(SUBSTRING "${STR}" "0" "${NL_POS}" LINE)
  string(SUBSTRING "${STR}"  "${NEXT_POS}"  -1 REST)
  set("${OUTPUT_VAR}" "${LINE}" PARENT_SCOPE)
  set("${VAR_NAME}" "${REST}" PARENT_SCOPE)
endfunction()

#
# add_prefix <PREFIX> [ARGS...]
#
macro(add_prefix OUTPUT_VAR PREFIX)
  unset("${OUTPUT_VAR}" PARENT_SCOPE)
  foreach(ARG ${ARGN})
    list(APPEND "${OUTPUT_VAR}" "${PREFIX}${ARG}")
  endforeach()
endmacro()

#
# assign_named_prefix <PREFIX> [ARGS...]
#
macro(assign_named_prefix PREFIX)
  set(ARGUMENTS "${ARGN}")

  while(NOT "${ARGUMENTS}" STREQUAL "")
    eat_line(ARGUMENTS NAME)
    eat_line(ARGUMENTS VALUE)

   set("${PREFIX}${NAME}" "${VALUE}" CACHE STRING "Color value")
  endwhile()
endmacro()


#
# assign_named_items [ARGS...]
#
macro(assign_named_items)
 unset(__N)
  foreach(__A ${ARGN})
     if(NOT DEFINED __N)
      set(__N "${__A}")
     else()
      set(__V "${__A}")
      endif()
     if(DEFINED __V)
         set("${__N}" "${__V}")
       unset(__N)
       unset(__V)
     endif()
  endforeach()
endmacro()

#
# assign_named_var <VAR_NAME>
#
macro(assign_named_var VAR_NAME)
  assign_named_items(${${VAR_NAME}})
endmacro()

#
# isin_var <OUTPUT-VAR> <ITEM> <VAR-NAME>
#
function(isin_var OUTPUT_VAR ITEM VAR_NAME)
  if(ITEM IN_LIST "${VAR_NAME}")
    set(RESULT TRUE)
  else()
    set(RESULT FALSE)
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# isin_list <OUTPUT-VAR> <ITEM> [LIST...]
#
function(isin_list OUTPUT_VAR ITEM)
  set(LIST "${ARGN}")
  isin_var("${OUTPUT_VAR}" "${ITEM}" LIST)
endfunction()

#
# show_result <RESULT_VAR>
#
macro(show_result RESULT_VAR)
  if(RESULT_VAR)
    if(${${RESULT_VAR}})
      set(__R yes)
    else()
      set(__R no)
    endif()
  endif()
  message("${RESULT_VAR} = ${__R}")
endmacro()

#
# absolute_paths <OUTPUT-VAR> [PATHS...]
#
function(absolute_paths OUTPUT_VAR)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(ABSOLUTE_PATH ARG OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# absolute_paths_in <OUTPUT-VAR> <BASE-DIRECTORY> [PATHS...]
#
function(absolute_paths_in OUTPUT_VAR BASE_DIRECTORY)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(ABSOLUTE_PATH ARG BASE_DIRECTORY "${BASE_DIRECTORY}" OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# relative_paths <OUTPUT-VAR> <BASE-DIRECTORY> [PATHS...]
#
function(relative_paths OUTPUT_VAR BASE_DIRECTORY)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(RELATIVE_PATH ARG BASE_DIRECTORY "${BASE_DIRECTORY}" OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# get_cwd <OUTPUT-VAR>
#
function(get_cwd OUTPUT_VAR)
  set(ARG ".")
  cmake_path(ABSOLUTE_PATH ARG OUTPUT_VARIABLE RESULT)
  string(REGEX REPLACE "[/\\\\]\\.$" "" RESULT "${RESULT}")
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# is_relative <OUTPUT-VAR> <PATH>
#
function(is_relative OUTPUT_VAR PATH)
  if(OUTPUT_VAR)
    cmake_path(IS_RELATIVE PATH "${OUTPUT_VAR}")
  endif()
endfunction()

#
# is_absolute <OUTPUT-VAR> <PATH>
#
function(is_absolute OUTPUT_VAR PATH)
  if(OUTPUT_VAR)
    cmake_path(IS_ABSOLUTE PATH "${OUTPUT_VAR}")
  endif()
endfunction()


#
# concat <OUTPUT-VAR> <SEPARATOR> [ARGUMENTS...]
#
function(concat OUTPUT_VAR SEPARATOR)
  set(RESULT "")
  foreach(ARG ${ARGN})
    if(NOT "${RESULT}" STREQUAL "")
      set(RESULT "${RESULT}${SEPARATOR}${ARG}")
    else()
      set(RESULT "${ARG}")
    endif()
  endforeach()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# basename <OUTPUT-VAR> <STR> [EXT_NAME]
#
function(basename OUTPUT_VAR STR)
  string(REGEX REPLACE ".*/" "" RESULT "${STR}")
  if(ARGN)
    string(REGEX REPLACE "\\${ARGN}\$" "" RESULT "${RESULT}")
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# var2define <NAME> [DEFINED_VALUE] [VAR_NAME]
#
function(var2define NAME)
  if(${ARGC} GREATER_EQUAL 3)
    list(GET ARGV 2 VAR_NAME)
  else()
    set(VAR_NAME "${NAME}")
  endif()
  set(VALUE "${${VAR_NAME}}")
  if(${ARGC} LESS_EQUAL 1)
    if(VALUE)
      add_definitions(-D${NAME}=1)
    else()
      add_definitions(-D${NAME}=0)
    endif()
  else()
    if(VALUE)
      list(GET ARGV 1 DEFINED_VALUE)
      add_definitions(-D${NAME}=${DEFINED_VALUE})
    endif()
  endif()
endfunction()


# Get number of columns the terminal supports
#
# get_columns <OUTPUT-VAR> [DEFAULT]
#
function(get_columns OUTPUT_VAR)
  if(${ARGC} GREATER_EQUAL 2)
    list(GET ARGV 1 DEFAULT_VALUE)
  else()
    set(DEFAULT_VALUE 80)
  endif()
  set(VALUE "$ENV{COLUMNS}")
  if("${VALUE}" OR NOT "${VALUE}" STREQUAL "")
    # message("Got COLUMNS (${VALUE}) from environment")
  else()
    execute_process(
      COMMAND tput cols
      OUTPUT_VARIABLE TPUT_COLS
      ERROR_QUIET
      ERROR_VARIABLE TPUT_ERROR)
    set(TPUT_ERROR TRUE)
    if(NOT TPUT_ERROR AND TPUT_COLS)
      set(VALUE ${TPUT_COLS})
    else()
      set(SOURCE_NAME ttysize.c)
      set(SOURCE_CODE
          "#include <unistd.h>\n#include <fcntl.h>\n#include <termios.h>\n#include <sys/ioctl.h>\n#include <stdio.h>\n\nint\nmain() {\n\tstruct winsize sz;\n\tint fd = isatty(0) ? dup(0) : open(\"/dev/tty\", O_RDWR);\n\n\tif(!isatty(fd)) {\n\t\tfputs(\"not a tty\\n\", stderr);\n\t\tfflush(stderr);\n\t\treturn 1;\n\t}\n\n\tif(ioctl(fd, TIOCGWINSZ, &sz) == -1) {\n\t\tperror(\"ioctl\");\n\t\treturn 1;\n\t}\n\n\tclose(fd);\n\n\tprintf(\"%u\\n\", sz.ws_col);\n\treturn 0;\n}\n"
      )
           message(CHECK_START "Trying to compile ${SOURCE_NAME}")
   try_run(
        RUN_RESULT
        COMPILE_RESULT
        SOURCE_FROM_CONTENT
        "${SOURCE_NAME}"
        "${SOURCE_CODE}"
        RUN_OUTPUT_STDOUT_VARIABLE
        RUN_OUTPUT
        COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT NO_CACHE)
      if(NOT COMPILE_RESULT)
        message(CHECK_FAIL "failed to compile:\n${COMPILE_OUTPUT}")
      else()
        if(RUN_RESULT STREQUAL 0)
          set(VALUE "${RUN_OUTPUT}")
        else()
          message(CHECK_FAIL "failed to run:\n${RUN_OUTPUT}")
        endif()
      endif()
    endif()
  endif()
  if("${VALUE}" STREQUAL "")
    set(VALUE "${DEFAULT_VALUE}")
    # message("Defaulting ${OUTPUT_VAR} to ${DEFAULT_VALUE}")
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${VALUE}" PARENT_SCOPE)
  endif()
endfunction()

set(LIST_INDENT "    ")
set(LIST_SEP " ")



#
# dump [VAR-NAMES...]
#
function(named_dump MSG)
  assign_named_var(DUMP_FORMAT)
  set(INDEX 0)
  math(EXPR LAST "${ARGC} - 2")
    #set(RESULT "${MSG}${COLOR_LIGHTCYAN}${START}")
  set(RESULT "${MSG}${START}")
  foreach(NAME ${ARGN})
    set(VALUE "${${NAME}}")
    #escape_string(VALUE "${VALUE}")
    set(LINE "${INDENT}${NAME}${PROP}${QUOTE}${VALUE}${QUOTE}")
    #set(LINE "${INDENT}${COLOR_YELLOW}${NAME}${COLOR_LIGHTCYAN}${PROP}${COLOR_LIGHTMAGENTA}${QUOTE}${VALUE}${QUOTE}${COLOR_LIGHTCYAN}")
    if(INDEX LESS LAST)
      set(LINE "${LINE}${COMMA}")
    endif()
    set(RESULT "${RESULT}${NEWLINE}${LINE}")
    math(EXPR INDEX "${INDEX} + 1")
  endforeach()
  if(NOT END STREQUAL "")
    set(RESULT "${RESULT}${NEWLINE}${END}")
  endif()
  #set(RESULT "${RESULT}${COLOR_NONE}")
  message("${RESULT}")
endfunction()

set(DUMP_FORMAT 
  INDENT "  "
  START " {" #" \\\\033[36m{\\\\033[0m"
  END "}"
  PROP ": "
  QUOTE "'"
  COMMA ","
  NEWLINE "\\n"
)

#
# dump [VAR-NAMES...]
#
function(dump)
  named_dump("Variable dump" ${ARGN})
endfunction()

#
# dump_list [VAR-NAMES...]
#
function(dump_list VARIABLE_NAME)
  assign_named_var(DUMP_FORMAT)
  message("List dump of ${VARIABLE_NAME}:")
  list(LENGTH "${VARIABLE_NAME}" NUM_ITEMS)
  set(INDEX 0)
  while(${INDEX} LESS ${NUM_ITEMS})
    list(GET "${VARIABLE_NAME}" "${INDEX}" ITEM)
    message("${INDENT}${INDEX}: ${ITEM}")
    math(EXPR INDEX "${INDEX} + 1")
  endwhile()
endfunction()

#
# make_list <OUTPUT-VAR> <MAX_LINE_LEN>
#
function(make_list OUTPUT_VAR MAX_LINE_LEN)
  set(RESULT "")
  set(LINE "")
  string(REPLACE " " ";" ARGS "${ARGN}")
  foreach(ITEM ${ARGS})
    string(LENGTH "${LINE}${LIST_SEP}${ITEM}" LEN)
    math(EXPR EFFECTIVE_LEN "${LEN} + 4")
    if(EFFECTIVE_LEN GREATER MAX_LINE_LEN)
      set(RESULT "${RESULT}\n--${LIST_INDENT}${LINE}")
      set(LINE " ${ITEM}")
      string(LENGTH "${LINE}" LEN)
    else()
      set(LINE "${LINE}${LIST_SEP}${ITEM}")
    endif()
  endforeach()
  if(LINE)
    set(RESULT "${RESULT}\n--${LIST_INDENT}${LINE}")
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()



#
# message_unescaped [STRINGS...]
#
function(message_unescaped)
  set(S "")
  foreach(ARG ${ARGN})
    set(S "${S}${ARG}")
  endforeach()
  unescape_string(S "${S}")
  message("${S}")
endfunction()

#
# message_func <FUNCTION-NAME> [STRINGS...]
#
function(message_func FUNC)
  set(S "${COLOR_LIGHTRED}${FUNC}${COLOR_NONE}")
  foreach(ARG ${ARGN})
    set(S "${S} ${ARG}")
  endforeach()
  unescape_string(S "${S}")
  message("${S}")
endfunction()


set(SEMI "╎")
set(COLORS
  NONE "${ANSI_ESCAPE}[0m"
  BLACK "${ANSI_ESCAPE}[0${SEMI}30m"
  RED "${ANSI_ESCAPE}[0${SEMI}31m"
  GREEN "${ANSI_ESCAPE}[0${SEMI}32m"
  BROWN "${ANSI_ESCAPE}[0${SEMI}33m"
  BLUE "${ANSI_ESCAPE}[0${SEMI}34m"
  MAGENTA "${ANSI_ESCAPE}[0${SEMI}35m"
  CYAN "${ANSI_ESCAPE}[0${SEMI}36m"
  LIGHTGRAY "${ANSI_ESCAPE}[0${SEMI}37m"
  DARKGRAY "${ANSI_ESCAPE}[1${SEMI}30m"
  LIGHTRED "${ANSI_ESCAPE}[1${SEMI}31m"
  LIGHTGREEN "${ANSI_ESCAPE}[1${SEMI}32m"
  YELLOW "${ANSI_ESCAPE}[1${SEMI}33m"
  LIGHTBLUE "${ANSI_ESCAPE}[1${SEMI}34m"
  LIGHTMAGENTA "${ANSI_ESCAPE}[1${SEMI}35m"
  LIGHTCYAN "${ANSI_ESCAPE}[1${SEMI}36m"
  WHITE "${ANSI_ESCAPE}[1${SEMI}37m" 
)
string(REPLACE ";" "\n" CMAP "${COLORS}")
string(REPLACE "${SEMI}" ";" CMAP "${CMAP}")

assign_named_prefix(COLOR_ ${CMAP})

#add_prefix(COLOR_NAMES "COLOR_" NONE BLACK RED GREEN BROWN BLUE MAGENTA CYAN LIGHTGRAY DARKGRAY LIGHTRED LIGHTGREEN YELLOW LIGHTBLUE LIGHTMAGENTA LIGHTCYAN WHITE)
#dump(${COLOR_NAMES})

#message_unescaped("COLOR_LIGHTRED: ${COLOR_LIGHTRED}blah${COLOR_NONE}")
#message_unescaped("CSTR: ${COLOR_LIGHTRED}blah${COLOR_NONE}")

#
# add_cflags <ADD> [OUTPUT_VAR]
#
function(add_cflags ADD)
  if(${ARGC} LESS 2)
    set(OUTPUT_VAR CMAKE_C_FLAGS)
  else()
    set(OUTPUT_VAR "${ARGV1}")
  endif()
  
  message_func("add_cflags" ${ADD} ${OUTPUT_VAR})

  set(RESULT "${${OUTPUT_VAR}}")
  string(REGEX REPLACE " +" ";" FLAGS "${RESULT}")
  string(REGEX REPLACE "^;+" "" FLAGS "${FLAGS}")
  list(REMOVE_DUPLICATES FLAGS) 
  if(NOT ADD IN_LIST FLAGS)
    list(APPEND RESULT ${ADD})
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# debug_flag <NAME> <DESC>
#
macro(debug_flag NAME DESC)
  option(DEBUG_${NAME} "${DESC}" OFF)
  if(DEBUG_${NAME})
    add_definitions(-DDEBUG_${NAME}=1)
  endif()
endmacro()


macro(check_pw_functions)
  check_include_file(pwd.h HAVE_PWD_H)
  check_include_file(grp.h HAVE_GRP_H)
  check_function_exists(getpwuid_r HAVE_GETPWUID_R)
  check_function_exists(getpwuid HAVE_GETPWUID)
  check_function_exists(getgrgid_r HAVE_GETGRGID_R)
  check_function_exists(getgrgid HAVE_GETGRGID)
endmacro()

macro(check_sig_functions)
  check_function_exists(sigprocmask HAVE_SIGPROCMASK)

  if(NOT HAVE_SIGPROCMASK)
    check_function_exists(sigblock HAVE_SIGBLOCK)
    check_function_exists(sigsetmask HAVE_SIGSETMASK)
    check_function_exists(sigpause HAVE_SIGPAUSE)
  endif()

  check_function_exists(sigaction HAVE_SIGACTION)
  check_function_exists(setpgid HAVE_SETPGID)
endmacro()

# Strip a MinSizeRel executable harder than an install-strip would: -s drops the
# symbol table, -R drops whole sections nothing reads back.
#
# .eh_frame/.eh_frame_hdr  unwind data -- 15% of an untuned binary
# .comment/.note* toolchain provenance
#
# strip ignores a -R for a section that is not there, so one list serves every
# target format.
#
# strip_minsize TARGET
#
macro(strip_minsize TARGET)
  if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND MINSIZE_STRIP AND CMAKE_STRIP AND NOT EMSCRIPTEN)
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND
        ${CMAKE_STRIP} -s -R .comment -R .note -R .note.ABI-tag -R .note.gnu.build-id -R .note.gnu.property -R .eh_frame -R .eh_frame_hdr $<TARGET_FILE:${TARGET}>
      COMMENT "Stripping ${TARGET}")
  endif()
endmacro()

#
# getenv <OUTPUT-VAR> <ENVIRONMENT-VAR>
#
function(getenv OUTPUT_VAR ENVIRON_VAR)
  if(OUTPUT_VAR)
    if(DEFINED ENV{${ENVIRON_VAR}})
      set("${OUTPUT_VAR}" "$ENV{${ENVIRON_VAR}}" PARENT_SCOPE)
    else()
      unset("${OUTPUT_VAR}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

#
# getenv_default <OUTPUT-VAR> <ENVIRONMENT-VAR> [DEFAULT-VALUE]
#
function(getenv_default OUTPUT_VAR ENVIRON_VAR)
  if(DEFINED ENV{${ENVIRON_VAR}})
    set(RESULT "$ENV{${ENVIRON_VAR}}")
  else()
    set(RESULT "${ARGN}")
  endif()
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()
