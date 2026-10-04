# build_libb2(BINARY SUFFIX PIC)
#
# Downloads and builds libb2 static library using CMake into ${BINARY}/deps-${SUFFIX}.
# Sets LIBB2_INCLUDE_DIR_${SUFFIX} and LIBB2_LIBRARY_FILE_${SUFFIX};
# the ExternalProject target is libb2_${SUFFIX}.
macro(build_libb2 BINARY SUFFIX PIC)
  include(ExternalProject)

  message("-- Building libb2 from source (${SUFFIX}, PIC=${PIC})")

  set(LIBB2_PREFIX_${SUFFIX} "${BINARY}/deps-${SUFFIX}")
  set(LIBB2_INCLUDE_DIR_${SUFFIX} "${LIBB2_PREFIX_${SUFFIX}}/include")
  set(LIBB2_LIBRARY_FILE_${SUFFIX} "${LIBB2_PREFIX_${SUFFIX}}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}b2${CMAKE_STATIC_LIBRARY_SUFFIX}")

  ExternalProject_Add(
    libb2_${SUFFIX}
    URL https://github.com/BLAKE2/libb2/releases/download/v0.98.1/libb2-0.98.1.tar.gz
    DOWNLOAD_DIR "${BINARY}/downloads-${SUFFIX}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE

    # Generate a CMakeLists.txt directly in the source directory during configuration
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "
      cmake_minimum_required(VERSION 3.10)
      project(libb2 C)

      # Create config.h expected by libb2 source files
      include(CheckFunctionExists)
      include(TestBigEndian)

      check_function_exists(explicit_bzero HAVE_EXPLICIT_BZERO)
      check_function_exists(explicit_memset HAVE_EXPLICIT_MEMSET)
      check_function_exists(memset_s HAVE_MEMSET_S)
      
      test_big_endian(IS_BIG_ENDIAN)
      if(NOT IS_BIG_ENDIAN)
        set(NATIVE_LITTLE_ENDIAN 1)
      endif()

      configure_file(
        \${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
        \${CMAKE_CURRENT_BINARY_DIR}/config.h
      )

      add_library(b2 STATIC
        src/blake2b-ref.c
        src/blake2s-ref.c
        src/blake2bp-ref.c
        src/blake2sp-ref.c
      )

      target_include_directories(b2 PRIVATE
        \${CMAKE_CURRENT_BINARY_DIR}
        src
      )

      set_target_properties(b2 PROPERTIES POSITION_INDEPENDENT_CODE ${PIC})

      install(TARGETS b2 DESTINATION lib)
      install(FILES src/blake2.h DESTINATION include)
    " > <SOURCE_DIR>/CMakeLists.txt

    # Configure, build, and install the generated CMake target
    COMMAND ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR>
              -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
              -DCMAKE_AR=${CMAKE_AR}
              -DCMAKE_RANLIB=${CMAKE_RANLIB}
              -DCMAKE_INSTALL_PREFIX=${LIBB2_PREFIX_${SUFFIX}}

    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release

    BUILD_BYPRODUCTS "${LIBB2_LIBRARY_FILE_${SUFFIX}}"
  )
endmacro(build_libb2)