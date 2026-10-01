# configures libarchive builtin (uncompress, extract, ...): 
# wraps libarchive/libarchive, cloned into third_party/ on first configure. 
# Included when BUILTIN_LIBARCHIVE is ON (-DBUILTIN_LIBARCHIVE=ON, or -DENABLE_ALL_BUILTINS=ON).
#
# configure_libarchive_builtin
#
macro(configure_libarchive_builtin)
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

    # init + fetch of one commit: pins the revision without a full history
    foreach(
      GIT_ARGS
      "init|--quiet" "remote|add|origin|${LIBARCHIVE_REPO}"
      "fetch|--quiet|--depth|1|origin|${LIBARCHIVE_REV}"
      "checkout|--quiet|FETCH_HEAD")
      string(REPLACE "|" ";" GIT_ARGS "${GIT_ARGS}")
      execute_process(COMMAND "${GIT_EXECUTABLE}" ${GIT_ARGS} WORKING_DIRECTORY "${LIBARCHIVE_DIR}" RESULT_VARIABLE GIT_RESULT ERROR_VARIABLE GIT_ERROR)

      if(NOT GIT_RESULT EQUAL 0)
        # leave no half-fetched directory behind for the next configure
        file(REMOVE_RECURSE "${LIBARCHIVE_DIR}")
        message(FATAL_ERROR "libarchive builtin: git ${GIT_ARGS} failed: ${GIT_ERROR}")
      endif()
    endforeach()
  endif()

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

endmacro()
