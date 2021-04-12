if(NOT HAVE_LIBOWFAT)
  file(GLOB LIBOWFAT_HEADERS *.h)

   file(
    GLOB
    LIBOWFAT_SOURCES
    lib/alloc/*.c
    lib/buffer/*.c
    lib/byte/*.c
    lib/fmt/*.c
    lib/mmap/*.c
    lib/open/*.c
    lib/path/*.c
    lib/scan/*.c
    lib/shell/*.c
    lib/sig/*.c
    lib/str/*.c
    lib/uint32/*.c
    lib/unix/*.c
    lib/utf8/*.c
    lib/wait/*.c)
  set(LIBOWFAT_SOURCES
      ${LIBOWFAT_SOURCES}
      ${LIBOWFAT_HEADERS}
      lib/stralloc/mmap_filename.c
      lib/stralloc/stralloc_append.c
      lib/stralloc/stralloc_cat.c
      lib/stralloc/stralloc_catb.c
      lib/stralloc/stralloc_catc.c
      lib/stralloc/stralloc_catlong0.c
      lib/stralloc/stralloc_catm_internal.c
      lib/stralloc/stralloc_cats.c
      lib/stralloc/stralloc_catulong.c
      lib/stralloc/stralloc_catulong0.c
      lib/stralloc/stralloc_copy.c
      lib/stralloc/stralloc_copyb.c
      lib/stralloc/stralloc_copys.c
      lib/stralloc/stralloc_diffs.c
      lib/stralloc/stralloc_dump.c
      lib/stralloc/stralloc_init.c
      lib/stralloc/stralloc_insertb.c
      lib/stralloc/stralloc_move.c
      lib/stralloc/stralloc_nul.c
      lib/stralloc/stralloc_remove.c
      lib/stralloc/stralloc_replacec.c
      lib/stralloc/stralloc_starts.c
      lib/stralloc/stralloc_trimr.c
      lib/stralloc/stralloc_write.c
      lib/stralloc/stralloc_zero.c
      lib/stralloc/stralloc_free${NAME_DEBUG}.c
      lib/stralloc/stralloc_ready${NAME_DEBUG}.c
      lib/stralloc/stralloc_readyplus${NAME_DEBUG}.c
      lib/stralloc/stralloc_trunc${NAME_DEBUG}.c)


  add_library(libowfat STATIC ${LIBOWFAT_SOURCES})
  set_target_properties(libowfat PROPERTIES OUTPUT_NAME owfat)

  set(LIBOWFAT_LIBRARY libowfat CACHE STRING "-lowfat library name" FORCE)
  set(LIBOWFAT_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}"CACHE STRING "-lowfat library directory" FORCE)

endif(NOT HAVE_LIBOWFAT)