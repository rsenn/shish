include(CheckCCompilerFlag)

#
# debug_module_options <MODULE-NAMES...>
#
macro(debug_module_options)
  if(${ARGC} GREATER_EQUAL 1)
    set(LIST ${ARGN})
  else()
    set(LIST ALLOC FD FDSTACK FDTABLE PARSE)
  endif()

  foreach(M ${LIST})
    string(TOLOWER NAME "${M}")
    option(DEBUG_${M} "Debug ${NAME}" OFF)
    if(DEBUG_${M})
      add_definitions(-DDEBUG_${M})
    endif()
  endforeach()
endmacro()

#
# debug_flag <NAME> <DESC>
#
macro(debug_flag NAME DESC)
  option(DEBUG_${NAME} "${DESC}" OFF)
  if(DEBUG_${NAME})
    add_definitions(-DDEBUG_${NAME}=1)
  endif()
endmacro()
