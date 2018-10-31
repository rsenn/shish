AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libopen.a
libopen_a_SOURCES = $(top_srcdir)/../../lib/open/open_read.c $(top_srcdir)/../../lib/open/open_temp.c $(top_srcdir)/../../lib/open/open_trunc.c

MAINTAINERCLEANFILES = Makefile.in
