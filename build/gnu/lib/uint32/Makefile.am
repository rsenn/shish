AUTOMAKE_OPTIONS = subdir-objects

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libuint32.a
libuint32_a_SOURCES = $(top_srcdir)/../../lib/uint32/uint32_prng.c $(top_srcdir)/../../lib/uint32/uint32_random.c $(top_srcdir)/../../lib/uint32/uint32_seed.c

MAINTAINERCLEANFILES = Makefile.in
