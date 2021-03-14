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
