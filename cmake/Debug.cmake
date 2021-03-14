include(CheckCCompilerFlag)

check_c_compiler_flag("-O0" OPT_C_OPT_NONE)
if(OPT_C_OPT_NONE)
  if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
    set(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG} -O0"
        CACHE STRING "C compiler options" FORCE)
  endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
endif(OPT_C_OPT_NONE)

check_c_compiler_flag("-ggdb" OPT_C_G_GDB)
if(OPT_C_G_GDB)
  if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
    set(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG} -ggdb"
        CACHE STRING "C compiler options" FORCE)
  endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
endif(OPT_C_G_GDB)

foreach(M ALLOC FD FDSTACK FDTABLE PARSE)
  string(TOLOWER NAME "${M}")
  option(DEBUG_${M} "Debug ${NAME}" OFF)
  if(DEBUG_${M})
    add_definitions(-DDEBUG_${M})
  endif(DEBUG_${M})
endforeach(M ALLOC FD FDSTACK FDTABLE PARSE)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(ENABLE_DUMP TRUE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
