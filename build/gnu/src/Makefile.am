SUBDIRS = builtin debug eval exec expand fd fdstack fdtable history job parse prompt redir sh sig source term tree var vartab

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/../.. -I$(top_srcdir)/../../lib -I$(top_srcdir)/../../src
LDFLAGS = -Wl,--whole-archive
LIBS = -Wl,--no-whole-archive
bin_PROGRAMS = shish

shish_SOURCES = $(top_srcdir)/../../src/sh/sh_main.c
shish_LDADD = $(top_builddir)/lib/open/libopen.a $(top_builddir)/lib/mmap/libmmap.a $(top_builddir)/lib/uint32/libuint32.a $(top_builddir)/lib/byte/libbyte.a $(top_builddir)/lib/buffer/libbuffer.a $(top_builddir)/lib/shell/libshell.a $(top_builddir)/lib/fmt/libfmt.a $(top_builddir)/lib/str/libstr.a $(top_builddir)/lib/scan/libscan.a $(top_builddir)/lib/stralloc/libstralloc.a $(top_builddir)/src/var/libvar.a $(top_builddir)/src/history/libhistory.a $(top_builddir)/src/sig/libsig.a $(top_builddir)/src/term/libterm.a $(top_builddir)/src/eval/libeval.a $(top_builddir)/src/source/libsource.a $(top_builddir)/src/prompt/libprompt.a $(top_builddir)/src/tree/libtree.a $(top_builddir)/src/builtin/libbuiltin.a $(top_builddir)/src/sh/libsh.a $(top_builddir)/src/fdstack/libfdstack.a $(top_builddir)/src/vartab/libvartab.a $(top_builddir)/src/fdtable/libfdtable.a $(top_builddir)/src/debug/libdebug.a $(top_builddir)/src/job/libjob.a $(top_builddir)/src/exec/libexec.a $(top_builddir)/src/expand/libexpand.a $(top_builddir)/src/redir/libredir.a $(top_builddir)/src/parse/libparse.a $(top_builddir)/src/fd/libfd.a
