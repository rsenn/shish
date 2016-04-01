AUTOMAKE_OPTIONS = subdir-objects

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libsource.a
libsource_a_SOURCES = $(top_srcdir)/../../src/source/source_flush.c $(top_srcdir)/../../src/source/source_get.c $(top_srcdir)/../../src/source/source_msg.c $(top_srcdir)/../../src/source/source_newline.c $(top_srcdir)/../../src/source/source_next.c $(top_srcdir)/../../src/source/source_peek.c $(top_srcdir)/../../src/source/source_peekn.c $(top_srcdir)/../../src/source/source_pop.c $(top_srcdir)/../../src/source/source_push.c $(top_srcdir)/../../src/source/source_skip.c

MAINTAINERCLEANFILES = Makefile.in
