AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libhistory.a
libhistory_a_SOURCES = $(top_srcdir)/../../src/history/history_advance.c $(top_srcdir)/../../src/history/history_clear.c $(top_srcdir)/../../src/history/history_cmdlen.c $(top_srcdir)/../../src/history/history_free.c $(top_srcdir)/../../src/history/history_load.c $(top_srcdir)/../../src/history/history_next.c $(top_srcdir)/../../src/history/history_prev.c $(top_srcdir)/../../src/history/history_print.c $(top_srcdir)/../../src/history/history_resize.c $(top_srcdir)/../../src/history/history_save.c $(top_srcdir)/../../src/history/history_set.c

MAINTAINERCLEANFILES = Makefile.in
