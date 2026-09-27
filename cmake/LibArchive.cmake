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
  add_subdirectory("${LIBARCHIVE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/third_party/libarchive" EXCLUDE_FROM_ALL)

  # Expose headers for your shell builtin implementation files
  include_directories("${LIBARCHIVE_DIR}/libarchive")

endmacro()
