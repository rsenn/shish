# configures libarchive builtin (uncompress, extract, ...): 
# wraps libarchive/libarchive, cloned into third_party/ on first configure. 
# Included when BUILTIN_LIBARCHIVE is ON (-DBUILTIN_LIBARCHIVE=ON, or -DENABLE_ALL_BUILTINS=ON).
#
# configure_libarchive_builtin sets
#
#   LIBARCHIVE_LIBRARIES     what shish links (libarchive and, when built here, its codecs)
#   LIBARCHIVE_INCLUDE_DIRS  where archive.h is
#
# A system libarchive is used when cmake finds one. Otherwise libarchive is
# built from source (cmake/BuildLibArchive.cmake) and so is every codec
# (zlib, bzip2, lzma, zstd, lz4, lzo2, libb2) that cmake does not find, so a
# musl, dietlibc or emscripten build needs nothing installed.
#
#   -DLIBARCHIVE_BUNDLED=ON    never look for a system libarchive
#
include(cmake/BuildLibArchive.cmake)

# the part that fetches third_party/libarchive and adds it as a subdirectory;
# kept for tcc, whose build needs the target_compile_options below
macro(libarchive_intree)
  libarchive_fetch()

  # Disable unwanted libarchive standalone tools/tests to keep the build light
  set(ENABLE_TEST FALSE CACHE BOOL "" FORCE)
  set(ENABLE_TAR FALSE CACHE BOOL "" FORCE)
  set(ENABLE_CPIO FALSE CACHE BOOL "" FORCE)
  set(ENABLE_CAT FALSE CACHE BOOL "" FORCE)
  set(ENABLE_UNZIP FALSE CACHE BOOL "" FORCE)

  # Include libarchive via subdirectory since it is a full CMake project
  set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
  set(BUILD_SHARED_LIBS OFF)
  # tcc defines __TINYC__ but not __GNUC__; see cmake/tcc-gnuc.h. Scoped to libarchive.
  include(CheckCSourceCompiles)
  check_c_source_compiles("#ifndef __TINYC__\n#error not tcc\n#endif\nint main(void){return 0;}" COMPILER_IS_TCC)

  # tcc rejects the bare `#pragma pack(push)` in the zip reader's LZMA code
  if(COMPILER_IS_TCC)
    set(ENABLE_LZMA FALSE CACHE BOOL "" FORCE)
  endif()

  add_subdirectory("${LIBARCHIVE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/third_party/libarchive" EXCLUDE_FROM_ALL)

  if(COMPILER_IS_TCC)
    target_compile_options(archive_static PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tcc-gnuc.h")
  endif()

  # Expose headers for your shell builtin implementation files
  include_directories("${LIBARCHIVE_DIR}/libarchive")

  set(LIBARCHIVE_LIBRARIES archive_static)
  set(LIBARCHIVE_INCLUDE_DIRS "${LIBARCHIVE_DIR}/libarchive")
endmacro(libarchive_intree)

# fetch third_party/libarchive when it is not there yet
macro(libarchive_fetch)
  set(LIBARCHIVE_REPO "https://github.com/libarchive/libarchive")
  set(LIBARCHIVE_REV "v3.7.7")
  set(LIBARCHIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libarchive")

  if(NOT EXISTS "${LIBARCHIVE_DIR}/CMakeLists.txt")
    find_program(GIT_EXECUTABLE git)

    if(NOT GIT_EXECUTABLE)
      message(FATAL_ERROR "libarchive builtin: git is needed to fetch ${LIBARCHIVE_REPO}")
    endif()

    message(STATUS "Fetching ${LIBARCHIVE_REPO} @ ${LIBARCHIVE_REV}")
    file(MAKE_DIRECTORY "${LIBARCHIVE_DIR}")

    foreach(
      GIT_ARGS
      "init|--quiet" "remote|add|origin|${LIBARCHIVE_REPO}"
      "fetch|--quiet|--depth|1|origin|${LIBARCHIVE_REV}"
      "checkout|--quiet|FETCH_HEAD")
      string(REPLACE "|" ";" GIT_ARGS "${GIT_ARGS}")
      execute_process(COMMAND "${GIT_EXECUTABLE}" ${GIT_ARGS} WORKING_DIRECTORY "${LIBARCHIVE_DIR}" RESULT_VARIABLE GIT_RESULT ERROR_VARIABLE GIT_ERROR)

      if(NOT GIT_RESULT EQUAL 0)
        file(REMOVE_RECURSE "${LIBARCHIVE_DIR}")
        message(FATAL_ERROR "libarchive builtin: git ${GIT_ARGS} failed: ${GIT_ERROR}")
      endif()
    endforeach()
  endif()
endmacro(libarchive_fetch)

macro(configure_libarchive_builtin)
  option(LIBARCHIVE_BUNDLED "Build libarchive from source even if the system has one" OFF)

  set(LIBARCHIVE_SYSTEM FALSE)

  # a cross toolchain must not pick up the host's libarchive (glibc headers
  # and libraries do not link into a musl, dietlibc or emscripten binary)
  if(NOT LIBARCHIVE_BUNDLED AND NOT CMAKE_CROSSCOMPILING AND NOT CMAKE_C_COMPILER MATCHES "musl|diet|emcc|wasi")
    find_package(LibArchive QUIET)

    if(LibArchive_FOUND)
      set(LIBARCHIVE_SYSTEM TRUE)
    endif()
  endif()

  include(CheckCSourceCompiles)
  check_c_source_compiles("#ifndef __TINYC__\n#error not tcc\n#endif\nint main(void){return 0;}" COMPILER_IS_TCC)

  if(LIBARCHIVE_SYSTEM)
    message(STATUS "libarchive: using ${LibArchive_LIBRARIES}")
    set(LIBARCHIVE_LIBRARIES ${LibArchive_LIBRARIES})
    set(LIBARCHIVE_INCLUDE_DIRS ${LibArchive_INCLUDE_DIRS})
    include_directories(${LIBARCHIVE_INCLUDE_DIRS})
  elseif(COMPILER_IS_TCC)
    libarchive_intree()
  else()
    message(STATUS "libarchive: not found, building it from source")
    libarchive_fetch()

    if(CMAKE_POSITION_INDEPENDENT_CODE)
      set(LIBARCHIVE_PIC ON)
    else()
      set(LIBARCHIVE_PIC OFF)
    endif()

    build_libarchive("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" static ${LIBARCHIVE_PIC})

    set(LIBARCHIVE_LIBRARIES ${LibArchive_LIBRARIES_static})
    set(LIBARCHIVE_INCLUDE_DIRS ${LibArchive_INCLUDE_DIRS_static})
    include_directories(${LIBARCHIVE_INCLUDE_DIRS})
  endif()
endmacro(configure_libarchive_builtin)
