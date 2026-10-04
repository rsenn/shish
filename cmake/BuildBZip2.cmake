# build_bzip2(BINARY SUFFIX PIC)
#
# Downloads and builds bzip2 static library using CMake into ${BINARY}/deps-${SUFFIX}.
# Sets BZIP2_INCLUDE_DIR_${SUFFIX} and BZIP2_LIBRARY_FILE_${SUFFIX};
# the ExternalProject target is bzip2_${SUFFIX}.
macro(build_bzip2 BINARY SUFFIX PIC)
  include(ExternalProject)

  message("-- Building bzip2 from source (${SUFFIX}, PIC=${PIC})")

  set(BZIP2_PREFIX_${SUFFIX} "${BINARY}/deps-${SUFFIX}")
  set(BZIP2_INCLUDE_DIR_${SUFFIX} "${BZIP2_PREFIX_${SUFFIX}}/include")
  set(BZIP2_LIBRARY_FILE_${SUFFIX} "${BZIP2_PREFIX_${SUFFIX}}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}bz2${CMAKE_STATIC_LIBRARY_SUFFIX}")

  # Build helper command to generate CMakeLists.txt inside <SOURCE_DIR>
  set(GEN_CMAKE_SCRIPT "${CMAKE_BINARY_DIR}/gen_bzip2_cmake.cmake")
  file(WRITE "${GEN_CMAKE_SCRIPT}" "file(WRITE \"\${SOURCE_DIR}/CMakeLists.txt\"\n\"cmake_minimum_required(VERSION 3.10)
project(bzip2 C)

add_library(bz2 STATIC
  blocksort.c
  huffman.c
  crctable.c
  randtable.c
  compress.c
  decompress.c
  bzlib.c
)

target_compile_definitions(bz2 PRIVATE _FILE_OFFSET_BITS=64)
set_target_properties(bz2 PROPERTIES POSITION_INDEPENDENT_CODE ${PIC})

install(TARGETS bz2 DESTINATION lib)
install(FILES bzlib.h DESTINATION include)
\")")

  ExternalProject_Add(
    bzip2_${SUFFIX}
    URL https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz
    DOWNLOAD_DIR "${BINARY}/downloads-${SUFFIX}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE

    # Run the generator script during configure step
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=<SOURCE_DIR> -DPIC=${PIC} -P "${GEN_CMAKE_SCRIPT}"
    
    # Configure and build the generated CMake project
    COMMAND ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR>
              -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
              -DCMAKE_AR=${CMAKE_AR}
              -DCMAKE_RANLIB=${CMAKE_RANLIB}
              -DCMAKE_INSTALL_PREFIX=${BZIP2_PREFIX_${SUFFIX}}

    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
    BUILD_BYPRODUCTS "${BZIP2_LIBRARY_FILE_${SUFFIX}}"
  )
endmacro(build_bzip2)