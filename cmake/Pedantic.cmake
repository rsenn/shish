# check_c_compiler_flag("-Wno-error=unused-const-variable" WARN_NO_UNUSED_FUNCTION) if(WARN_NO_UNUSED_FUNCTION)
# set(WERROR_FLAG "${WERROR_FLAG} -Wno-error=unused-const-variabe") endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(PEDANTIC_COMPILE_FLAGS
      -pedantic-errors
      -Wall
      -Wextra
      -pedantic
      -Wold-style-cast
      -Wundef
      -Wredundant-decls
      -Wwrite-strings
      -Wpointer-arith
      -Wcast-qual
      -Wformat=2
      -Wmissing-include-dirs
      -Wcast-align
      -Wnon-virtual-dtor
      -Wctor-dtor-privacy
      -Wdisabled-optimization
      -Winvalid-pch
      -Woverloaded-virtual
      -Wconversion
      -Wno-ctor-dtor-privacy
      -Wno-format-nonliteral
      -Wno-shadow)
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.6)
    set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wnoexcept -Wno-dangling-else -Wno-unused-local-typedefs)
  endif()
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
    set(PEDANTIC_COMPILE_FLAGS
        ${PEDANTIC_COMPILE_FLAGS}
        -Wdouble-promotion
        -Wtrampolines
        -Wzero-as-null-pointer-constant
        -Wuseless-cast
        -Wvector-operation-performance
        -Wsized-deallocation)
  endif()
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 6.0)
    set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2 -Wnull-dereference -Wduplicated-cond)
  endif()
  set(WERROR_FLAG "${WERROR_FLAG} -Werror")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -pedantic -Wconversion -Wno-sign-conversion)
  set(WERROR_FLAG "${WERROR_FLAG} -Werror")
endif()
if(MSVC)
  set(PEDANTIC_COMPILE_FLAGS /W3)
  set(WERROR_FLAG /WX)
endif()
if(WARN_WERROR)
  set(WARN_C_COMPILER_FLAGS "-Wall ${WERROR_FLAG}")
endif()
if(WARN_PEDANTIC)
  set(WARN_C_COMPILER_FLAGS "-Wall ${PEDANTIC_COMPILE_FLAGS}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_C_COMPILER_FLAGS}")
