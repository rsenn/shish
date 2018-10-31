AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libshell.a
libshell_a_SOURCES = $(top_srcdir)/../../lib/shell/shell_alloc.c $(top_srcdir)/../../lib/shell/shell_basename.c $(top_srcdir)/../../lib/shell/shell_canonicalize.c $(top_srcdir)/../../lib/shell/shell_dirname.c $(top_srcdir)/../../lib/shell/shell_error.c $(top_srcdir)/../../lib/shell/shell_errorn.c $(top_srcdir)/../../lib/shell/shell_fnmatch.c $(top_srcdir)/../../lib/shell/shell_getcwd.c $(top_srcdir)/../../lib/shell/shell_gethome.c $(top_srcdir)/../../lib/shell/shell_gethostname.c $(top_srcdir)/../../lib/shell/shell_getopt.c $(top_srcdir)/../../lib/shell/shell_init.c $(top_srcdir)/../../lib/shell/shell_readlink.c $(top_srcdir)/../../lib/shell/shell_realloc.c $(top_srcdir)/../../lib/shell/shell_realpath.c $(top_srcdir)/../../lib/shell/shell_strdup.c

MAINTAINERCLEANFILES = Makefile.in
