# Strip a MinSizeRel executable harder than an install-strip would: -s drops the
# symbol table, -R drops whole sections nothing reads back.
#
# .eh_frame/.eh_frame_hdr  unwind data -- 15% of an untuned binary
# .comment/.note* toolchain provenance
#
# strip ignores a -R for a section that is not there, so one list serves every
# target format.
#
# strip_minsize TARGET
#
macro(strip_minsize TARGET)
  if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND MINSIZE_STRIP AND CMAKE_STRIP AND NOT EMSCRIPTEN)
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND
        ${CMAKE_STRIP} -s -R .comment -R .note -R .note.ABI-tag -R .note.gnu.build-id -R .note.gnu.property -R .eh_frame -R .eh_frame_hdr $<TARGET_FILE:${TARGET}>
      COMMENT "Stripping ${TARGET}")
  endif()
endmacro()

#
# target_cmake_format [SOURCES-VAR]
#
macro(target_cmake_format)
  set(ARGUMENTS ${ARGN})
  if(NOT ARGUMENTS)
    file(GLOB ARGUMENTS CMakeLists.txt *.cmake cmake/*.cmake)
    list(FILTER ARGUMENTS EXCLUDE REGEX ".*\\.h\\.cmake")
  endif()

  relative_paths(FILES "${CMAKE_CURRENT_SOURCE_DIR}" ${ARGUMENTS})

  getenv_default(LINE_WIDTH WIDTH "80")

  add_custom_target(
    cmake-format
    COMMAND cmake-format --line-width ${LINE_WIDTH} --max-pargs-hwrap 100 -i ${FILES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Format CMake files"
    SOURCES ${FILES})
endmacro()

#
# target_clang_format [SOURCES-VAR]
#
macro(target_clang_format)
  set(ARGUMENTS ${ARGN})
  if(NOT ARGUMENTS)
    file(GLOB ARGUMENTS */*/*.c */*.h)
    #message(WARNING "target_clang_format globbed files: ${ARGUMENTS}")
  endif()

  #dump_list(ARGUMENTS)
  relative_paths(FILES "${CMAKE_CURRENT_SOURCE_DIR}" ${ARGUMENTS})

  #get_cwd(CWD)
  #dump(CWD)

  add_custom_target(
    clang-format
    COMMAND clang-format --verbose --style=file -i ${FILES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Format *.c/*.h source files"
    SOURCES ${FILES})
endmacro()
