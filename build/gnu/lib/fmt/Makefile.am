AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libfmt.a
libfmt_a_SOURCES = $(top_srcdir)/../../lib/fmt/fmt_long.c $(top_srcdir)/../../lib/fmt/fmt_minus.c $(top_srcdir)/../../lib/fmt/fmt_ulong.c $(top_srcdir)/../../lib/fmt/fmt_ulong0.c $(top_srcdir)/../../lib/fmt/fmt_xlong.c $(top_srcdir)/../../lib/fmt/fmt_xlonglong.c

MAINTAINERCLEANFILES = Makefile.in
