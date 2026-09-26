include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Functions.cmake)

#
# init_libowfat
#
macro(init_libowfat)
  if(NOT HAVE_LIBOWFAT)
    file(GLOB LIBOWFAT_HEADERS lib/*.h)
    file(GLOB LIBOWFAT_SOURCES lib/*/*.c)

    if(NOT HAVE_MMAP_SUPPORT)
      list(FILTER LIBOWFAT_SOURCES EXCLUDE REGEX "lib.*([^d]mmap|munmap)")
    endif()

    set(LIBOWFAT_SOURCES ${LIBOWFAT_HEADERS} ${LIBOWFAT_SOURCES})

    add_list(ALL_SOURCES ${LIBOWFAT_SOURCES})

    add_library(libowfat STATIC ${LIBOWFAT_SOURCES} ${LIBOWFAT_HEADERS})
    set_target_properties(libowfat PROPERTIES OUTPUT_NAME owfat)

    set(LIBOWFAT_LIBRARY libowfat CACHE STRING "-lowfat library name" FORCE)
    set(LIBOWFAT_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "-lowfat library directory" FORCE)
  endif()
endmacro()
