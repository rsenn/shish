AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libsh.a
libsh_a_SOURCES = $(top_srcdir)/../../src/sh/sh_error.c $(top_srcdir)/../../src/sh/sh_errorn.c $(top_srcdir)/../../src/sh/sh_exit.c $(top_srcdir)/../../src/sh/sh_forked.c $(top_srcdir)/../../src/sh/sh_getcwd.c $(top_srcdir)/../../src/sh/sh_gethome.c $(top_srcdir)/../../src/sh/sh_init.c $(top_srcdir)/../../src/sh/sh_loop.c $(top_srcdir)/../../src/sh/sh_msg.c $(top_srcdir)/../../src/sh/sh_pop.c $(top_srcdir)/../../src/sh/sh_popargs.c $(top_srcdir)/../../src/sh/sh_push.c $(top_srcdir)/../../src/sh/sh_pushargs.c $(top_srcdir)/../../src/sh/sh_root.c $(top_srcdir)/../../src/sh/sh_setargs.c $(top_srcdir)/../../src/sh/sh_subshell.c $(top_srcdir)/../../src/sh/sh_usage.c

MAINTAINERCLEANFILES = Makefile.in
