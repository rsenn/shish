AUTOMAKE_OPTIONS = subdir-objects

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libvartab.a
libvartab_a_SOURCES = $(top_srcdir)/../../src/vartab/vartab_add.c $(top_srcdir)/../../src/vartab/vartab_cleanup.c $(top_srcdir)/../../src/vartab/vartab_dump.c $(top_srcdir)/../../src/vartab/vartab_hash.c $(top_srcdir)/../../src/vartab/vartab_pop.c $(top_srcdir)/../../src/vartab/vartab_print.c $(top_srcdir)/../../src/vartab/vartab_push.c $(top_srcdir)/../../src/vartab/vartab_root.c $(top_srcdir)/../../src/vartab/vartab_search.c

MAINTAINERCLEANFILES = Makefile.in
