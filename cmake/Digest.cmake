# digest builtin (md5sum, sha1sum, ...): wraps th-blitz/Hash-Algorithms, cloned into third_party/ on first configure. Included when BUILTIN_DIGEST is ON (-DBUILTIN_DIGEST=ON, or -DENABLE_ALL_BUILTINS=ON).

set(HASH_ALGORITHMS_REPO "https://github.com/th-blitz/Hash-Algorithms")
set(HASH_ALGORITHMS_REV "953a0b6bd432e911a85fd2d015469380b5b15c42")
set(HASH_ALGORITHMS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Hash-Algorithms")

if(NOT EXISTS "${HASH_ALGORITHMS_DIR}/md5.c")
  find_program(GIT_EXECUTABLE git)

  if(NOT GIT_EXECUTABLE)
    message(FATAL_ERROR "digest builtin: git is needed to fetch ${HASH_ALGORITHMS_REPO}")
  endif(NOT GIT_EXECUTABLE)

  message(STATUS "Fetching ${HASH_ALGORITHMS_REPO} @ ${HASH_ALGORITHMS_REV}")
  file(MAKE_DIRECTORY "${HASH_ALGORITHMS_DIR}")

  # init + fetch of one commit: pins the revision without a full history
  foreach(GIT_ARGS "init|--quiet" "remote|add|origin|${HASH_ALGORITHMS_REPO}" "fetch|--quiet|--depth|1|origin|${HASH_ALGORITHMS_REV}" "checkout|--quiet|FETCH_HEAD")
    string(REPLACE "|" ";" GIT_ARGS "${GIT_ARGS}")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" ${GIT_ARGS}
      WORKING_DIRECTORY "${HASH_ALGORITHMS_DIR}"
      RESULT_VARIABLE GIT_RESULT
      ERROR_VARIABLE GIT_ERROR)

    if(NOT GIT_RESULT EQUAL 0)
      # leave no half-fetched directory behind for the next configure
      file(REMOVE_RECURSE "${HASH_ALGORITHMS_DIR}")
      message(FATAL_ERROR "digest builtin: git ${GIT_ARGS} failed: ${GIT_ERROR}")
    endif(NOT GIT_RESULT EQUAL 0)
  endforeach(GIT_ARGS)
endif(NOT EXISTS "${HASH_ALGORITHMS_DIR}/md5.c")

set(HASH_ALGORITHMS_SOURCES)
foreach(
  ALGO
  md5
  sha1
  sha224
  sha256
  sha384
  sha512-224
  sha512-256
  sha512)
  list(APPEND HASH_ALGORITHMS_SOURCES "third_party/Hash-Algorithms/${ALGO}.c")
endforeach(ALGO)

# vendored code: keep our warning flags (and WARN_WERROR) off it
set_source_files_properties(${HASH_ALGORITHMS_SOURCES} PROPERTIES COMPILE_FLAGS "-w")

# builtin_digest.c itself is added by Builtins.cmake
list(APPEND BUILTIN_SOURCES ${HASH_ALGORITHMS_SOURCES})
