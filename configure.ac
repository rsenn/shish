# $Id: configure.ac,v 1.21 2005/04/23 10:19:31 smoli Exp $
# ===========================================================================
#
# GNU autoconf template for shish
#
# initialize autoconf (require version 2.5x)
# ---------------------------------------------------------------------------
AC_PREREQ([2.50])
AC_INIT([shish], [0.7-pre3])
AC_CONFIG_SRCDIR([src/sh/sh_main.c])
dnl AC_LANG_SOURCE([C])

# check for compiling modes
# ---------------------------------------------------------------------------
AC_CHECK_COLOR

# set some info stuff
# ---------------------------------------------------------------------------
AC_SET_ARGS

# set directories
# ---------------------------------------------------------------------------
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/sh/sh_main.c])
# dnl AC_CONFIG_AUX_DIR([autoconf])

# check host and target
# ---------------------------------------------------------------------------
# dnl AC_CANONICAL_HOST
# dnl AC_CANONICAL_TARGET

# check for programs needed to build shish
# ---------------------------------------------------------------------------
AC_DIETLIBC([/opt/diet /usr/diet])
AC_PROG_CC([cc gcc])
AC_PROG_CC_C99
AC_PROG_AR
AC_PROG_RANLIB
AC_CHECK_TOOL([STRIP],[strip],[:])
AC_PROG_INSTALL

# check for operating system features
# ---------------------------------------------------------------------------
AC_C_INLINE
AC_FUNC_ALLOCA

PREADD= POSTADD=
AX_CHECK_LINK_FLAG([--no-whole-archive],[PREADD="--whole-archive" POSTADD="--no-whole-archive"],[
  AX_CHECK_LINK_FLAG([-Wl,--no-whole-archive],[PREADD="-Wl,--whole-archive" POSTADD="-Wl,--no-whole-archive"],[
    : test
    dnl  AX_CHECK_COMPILE_FLAG([-Wl,--no-whole-archive],[PREADD="-Wl,--whole-archive" POSTADD="-Wl,--no-whole-archive"])
])
])
AC_SUBST([PREADD])
AC_SUBST([POSTADD])

dnl AC_CHECK_LIB([owfat], [array_allocate])
dnl AC_CHECK_TERMIOS

dnl AC_FUNC_POLL([cp $srcdir/iopause.h2 iopause.h], [cp $srcdir/iopause.h1 iopause.h])
dnl AC_FUNC_EPOLL
dnl AC_FUNC_KQUEUE
dnl AC_FUNC_SELECT([cp $srcdir/select.h2 select.h], [cp $srcdir/select.h1 select.h])
dnl AC_FUNC_SENDFILE
dnl AC_FUNC_DEVPOLL
dnl AC_TYPE_SOCKLEN
dnl AC_TYPE_UINT8_T
dnl AC_TYPE_UINT16_T
dnl AC_TYPE_UINT32_T
dnl AC_TYPE_UINT64_T
AC_DEFUN([STDINT_INCLUDES],[
#include <inttypes.h>
#include <stdint.h>
])
AC_CHECK_SIZEOF([uint8_t], [], [STDINT_INCLUDES])
AC_CHECK_SIZEOF([uint16_t], [], [STDINT_INCLUDES])
AC_CHECK_SIZEOF([uint32_t], [], [STDINT_INCLUDES])
AC_CHECK_SIZEOF([uint64_t], [], [STDINT_INCLUDES])

dnl AC_CHECK_TYPE([uint8_t], [], [AC_TYPE_UINT8_T], [STDINT_INCLUDES])
dnl AC_CHECK_TYPE([uint16_t], [], [AC_TYPE_UINT16_T], [STDINT_INCLUDES])
dnl AC_CHECK_TYPE([uint32_t], [], [AC_TYPE_UINT32_T], [STDINT_INCLUDES])
dnl AC_CHECK_TYPE([uint64_t], [], [AC_TYPE_UINT64_T], [STDINT_INCLUDES])

dnl AC_IPV6
dnl AC_SCOPE_ID
dnl AC_CHECK_N2I
AC_CHECK_HEADERS([unistd.h io.h pwd.h signal.h sys/types.h sys/signal.h],,,[])
AC_CHECK_FUNC([glob],[NO_GLOB="#"],[NO_GLOB=""])
AC_SUBST([NO_GLOB])
AC_CHECK_FUNC([gethostname],[],[dnl

	AC_MSG_CHECKING(for gethostname in -lws2_32)
	old_LIBS="$LIBS"
	LIBS="$LIBS -lws2_32"

	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
		/*int __stdcall gethostname(char *, int);*/
		#include <winsock2.h>
		]], [[
		gethostname(0,0);
		]])],[
		AC_DEFINE_UNQUOTED([HAVE_GETHOSTNAME],[1],[Define this if you have the 'gethostname' function])
		AC_MSG_RESULT([yes])
		],[
			AC_MSG_RESULT([no])
			LIBS="$old_LIBS"
		])
		
])
AC_CHECK_FUNCS([sethostname sys_siglist lstat readlink]) 
AC_CHECK_TYPE([ssize_t],AC_DEFINE_UNQUOTED([HAVE_SSIZE_T],[1],[Define this if you have the 'ssize_t' type]),[],[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

AC_CHECK_TYPE([uid_t],AC_DEFINE_UNQUOTED([HAVE_UID_T],[1],[Define this if you have the 'uid_t' type]),[],[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
])

AC_CHECK_TYPE([sigset_t],AC_DEFINE_UNQUOTED([HAVE_SIGSET_T],[1],[Define this if you have the 'sigset_t' type]),[],[
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
])

# set debugging stuff after compiler test (-Werror could fuck it up)
# ---------------------------------------------------------------------------
AC_CHECK_DEP
AC_CHECK_DEBUG
AC_CHECK_MAINTAINER

CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H=1"

AC_SUBST([CPPFLAGS])
dnl 
dnl PIC=#
dnl AC_SUBST([DLM_TRUE],[#])
dnl AC_SUBST([A_LIB],[#])
dnl AC_SUBST([DLM_TRUE],[#])
dnl AC_SUBST([NO_A_LIB],[#])
dnl AC_SUBST([NO_PIE_LIB],[#])
dnl AC_SUBST([PIE_LIB],[#])
AC_SUBST([WIN32_TRUE],[#])

# check for needed types
# ---------------------------------------------------------------------------

# library subdirectories
# ---------------------------------------------------------------------------
dnl AC_CONFIG_LIBRARIES([lib/libshish])
dnl AC_CONFIG_PROGRAMS([src/shish], [lib/libshish.a])

AC_CONFIG_FILES([
Makefile
config.mk
build.mk
doc/Makefile
doc/man/Makefile
doc/posix/Makefile
lib/Makefile
lib/buffer/Makefile
lib/byte/Makefile
lib/fmt/Makefile
lib/mmap/Makefile
lib/open/Makefile
lib/scan/Makefile
lib/shell/Makefile
lib/str/Makefile
lib/stralloc/Makefile
lib/uint32/Makefile
scripts/Makefile
src/Makefile
src/builtin/Makefile
src/debug/Makefile
src/eval/Makefile
src/exec/Makefile
src/expand/Makefile
src/fd/Makefile
src/fdstack/Makefile
src/fdtable/Makefile
src/history/Makefile
src/job/Makefile
src/parse/Makefile
src/prompt/Makefile
src/redir/Makefile
src/sh/Makefile
src/sig/Makefile
src/source/Makefile
src/term/Makefile
src/tree/Makefile
src/var/Makefile
src/vartab/Makefile])

AC_CONFIG_STATUS

# be verbose :)
# ---------------------------------------------------------------------------
AC_SUMMARIZE([prefix bindir sysconfdir],
             [COLOR DEPS_TRUE DEBUG | CC CFLAGS LDFLAGS LIBS],
             "%20s: %s")
