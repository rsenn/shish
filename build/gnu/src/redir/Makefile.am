AUTOMAKE_OPTIONS = subdir-objects

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src

noinst_LIBRARIES = libredir.a
libredir_a_SOURCES = $(top_srcdir)/../../src/redir/redir_addhere.c $(top_srcdir)/../../src/redir/redir_dup.c $(top_srcdir)/../../src/redir/redir_eval.c $(top_srcdir)/../../src/redir/redir_here.c $(top_srcdir)/../../src/redir/redir_open.c $(top_srcdir)/../../src/redir/redir_parse.c $(top_srcdir)/../../src/redir/redir_source.c

MAINTAINERCLEANFILES = Makefile.in
