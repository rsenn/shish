AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libscan.a
libscan_a_SOURCES = $(top_srcdir)/../../lib/scan/scan_8long.c $(top_srcdir)/../../lib/scan/scan_int.c $(top_srcdir)/../../lib/scan/scan_long.c $(top_srcdir)/../../lib/scan/scan_uint.c $(top_srcdir)/../../lib/scan/scan_ulong.c

MAINTAINERCLEANFILES = Makefile.in
