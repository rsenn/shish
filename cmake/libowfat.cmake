include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Functions.cmake)

if(NOT HAVE_LIBOWFAT)
  file(GLOB LIBOWFAT_HEADERS lib/*.h)

  file(GLOB LIBOWFAT_SOURCES lib/*/*.c)
  set(LIBOWFAT_SOURCES ${LIBOWFAT_HEADERS} ${LIBOWFAT_SOURCES})

  list(APPEND ALL_SOURCES ${LIBOWFAT_SOURCES})

  add_library(libowfat STATIC ${LIBOWFAT_SOURCES} ${LIBOWFAT_HEADERS})
  set_target_properties(libowfat PROPERTIES OUTPUT_NAME owfat)

  set(LIBOWFAT_LIBRARY
      libowfat
      CACHE STRING "-lowfat library name" FORCE)
  set(LIBOWFAT_LIBDIR
      "${CMAKE_CURRENT_BINARY_DIR}"
      CACHE STRING "-lowfat library directory" FORCE)

endif(NOT HAVE_LIBOWFAT)
