# ===========================================================================
#
# GNU autoconf template for shish
#
# initialize autoconf (require version 2.5x)
# ---------------------------------------------------------------------------
AC_PREREQ([2.50])
AC_INIT([shish], [0.9])
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

# quote-only include path for src/ (its features.h must not shadow
# <features.h>): -iquote (gcc, clang), else only the build dir's src/
# (tcc; builtin_config.h lives there, the rest is included relatively)
# ---------------------------------------------------------------------------
AC_MSG_CHECKING([whether $CC supports -iquote])
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -iquote ."
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
  [IQUOTE_SRC='-iquote $(top_srcdir)/src -iquote $(top_builddir)/src'; ac_iquote_ok=yes],
  [IQUOTE_SRC='-I$(top_builddir)/src'; ac_iquote_ok=no])
CFLAGS="$ac_save_CFLAGS"
AC_MSG_RESULT([$ac_iquote_ok])
AC_SUBST(IQUOTE_SRC)
AC_PROG_AR
AC_PROG_RANLIB
AC_PROG_INSTALL

# check for operating system features
# ---------------------------------------------------------------------------
AC_C_INLINE
AC_FUNC_ALLOCA

dnl AX_CHECK_COMPILE_FLAG([-Wl,--no-whole-archive],[AC_SUBST([PREADD],[-Wl,--whole-archive])
dnl AC_SUBST([POSTADD],[-Wl,--no-whole-archive])
dnl ])


dnl AC_CHECK_LIB([owfat], [array_allocate])
dnl AC_CHECK_TERMIOS
AC_CHECK_LIB([m],[powl],[LIBS="$LIBS -lm"])

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
AC_CHECK_HEADERS([unistd.h io.h pwd.h grp.h signal.h sys/types.h sys/signal.h],,,[])
AC_CHECK_FUNC([glob],AC_DEFINE_UNQUOTED([HAVE_GLOB],[1],[Define to 1 if you have the 'glob' function]))
AC_CHECK_FUNC([globfree],AC_DEFINE_UNQUOTED([HAVE_GLOBFREE],[1],[Define to 1 if you have the 'globfree' function]))

AC_CHECK_FUNC([issetugid],AC_DEFINE_UNQUOTED([HAVE_ISSETUGID],[1],[Define to 1 if you have the 'issetugid' function]))

dnl AM_CONDITIONAL([HAVE_GLOB],[test x$ac_cv_func_glob = xyes])
dnl AM_CONDITIONAL([HAVE_GLOBFREE],[test x$ac_cv_func_globfree = xyes])

if test x$ac_cv_func_globfree = xyes; then
	NO_GLOB="#"
else
	NO_GLOB=""
fi
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
AC_CHECK_FUNCS([sethostname sys_siglist lstat readlink getpwuid_r getpwuid getgrgid_r getgrgid getppid])

dnl fork() needs a hardcoded result on two platform families, the same
dnl as the CMake build (see cmake/Checks.cmake for the full
dnl reasoning): native Windows has no libc fork() for AC_CHECK_FUNC to
dnl find, but lib/unix/fork.c supplies one (via RtlCloneUserProcess);
dnl Emscripten/WASI's musl libc provides a real, linkable fork()
dnl symbol that AC_CHECK_FUNC would happily find, but the underlying
dnl syscall has no implementation in a single-threaded wasm module and
dnl fails at runtime (ENOSYS). Cygwin and MSYS both provide a real
dnl POSIX fork() of their own, so they fall through to the plain check
dnl below like every other POSIX target.
AC_EGREP_CPP([shish_yes_windows_native],[
#if (defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || \
     defined(__MINGW64__) || defined(MSVC)) && \
    !defined(__CYGWIN__) && !defined(__MSYS__)
shish_yes_windows_native
#endif
],[shish_windows_native=yes],[shish_windows_native=no])

AC_EGREP_CPP([shish_yes_forkless_wasm],[
#if defined(__EMSCRIPTEN__) || defined(__wasi__)
shish_yes_forkless_wasm
#endif
],[shish_forkless_wasm=yes],[shish_forkless_wasm=no])

if test "x$shish_windows_native" = xyes; then
	AC_DEFINE_UNQUOTED([HAVE_FORK],[1],[Define this if you have a working fork() -- see cmake/Checks.cmake for why this isn't a plain existence check on every platform])
elif test "x$shish_forkless_wasm" = xyes; then
	: dnl leave HAVE_FORK undefined
else
	AC_CHECK_FUNC([fork],AC_DEFINE_UNQUOTED([HAVE_FORK],[1],[Define this if you have a working fork() -- see cmake/Checks.cmake for why this isn't a plain existence check on every platform]))
fi
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

dnl AC_CHECK_TERMIOS


AC_CHECK_HEADER([sys/wait.h],[AC_DEFINE_UNQUOTED([HAVE_SYS_WAIT_H],[1],[Define this if you have the `sys/wait.h' header file])])
AC_CHECK_HEADER([termios.h],[AC_DEFINE_UNQUOTED([HAVE_TERMIOS_H],[1],[Define this if you have the `termios.h' header file])])

AC_CHECK_HEADER([glob.h],[AC_DEFINE_UNQUOTED([HAVE_GLOB_H],[1],[Define this if you have the `glob.h' header file])])
AC_CHECK_FUNC([glob],[AC_DEFINE_UNQUOTED([HAVE_GLOB],[1],[Define this if you have the `glob' function])])

AC_CHECK_HEADER([fcntl.h],[AC_DEFINE_UNQUOTED([HAVE_FCNTL_H],[1],[Define this if you have the `fcntl.h' header file])])
AC_CHECK_FUNC([fcntl],[AC_DEFINE_UNQUOTED([HAVE_FCNTL],[1],[Define this if you have the `fcntl' function])])

# set debugging stuff after compiler test (-Werror could fuck it up)
# ---------------------------------------------------------------------------
AC_CHECK_DEP
AC_CHECK_DEBUG
AC_CHECK_MAINTAINER

CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H=1"

AC_SUBST([CPPFLAGS])
dnl 
dnl AC_SUBST([PIC],[#])
dnl AC_SUBST([DLM_TRUE],[#])
dnl AC_SUBST([A_LIB],[#])
dnl AC_SUBST([DLM_TRUE],[#])
dnl AC_SUBST([NO_A_LIB],[#])
dnl AC_SUBST([NO_PIE_LIB],[#])
dnl AC_SUBST([PIE_LIB],[#])
AC_SUBST([WIN32_TRUE],[#])

# check for needed types
# ---------------------------------------------------------------------------

dnl ALL_BUILTINS: every BUILTIN_<name> the table in builtin_table.c is
dnl gated by, lowercased (space separated). EXTRA_BUILTINS: the ones whose
dnl source is in src/builtin/extra/. Both are read when this file is turned
dnl into ./configure, so re-run autogen.sh after adding a builtin.
m4_define([ALL_BUILTINS],
  m4_normalize(m4_esyscmd([sed '/#if BUILTIN_/ { N; /{"/ { s,.*BUILTIN_,, ; s,\n.*,,; p } }' -n src/builtin/builtin_table.c | sort -u | tr A-Z a-z | tr '\n' ' '])))

m4_define([EXTRA_BUILTINS],
  m4_normalize(m4_esyscmd([ls src/builtin/extra | sed 's,^builtin_,,; s,\.c$,,' | tr '\n' ' '])))

dnl default set: every builtin that is not in src/builtin/extra/
m4_define([DEFAULT_BUILTINS],
  m4_normalize(m4_foreach_w([BUILTIN], m4_defn([ALL_BUILTINS]),
    [m4_if(m4_index([ ]m4_defn([EXTRA_BUILTINS])[ ], [ ]BUILTIN[ ]), [-1], [BUILTIN ])])))

dnl "all" leaves out digest: it needs third_party/Hash-Algorithms, which
dnl only the CMake build (cmake/Digest.cmake) fetches and compiles.
m4_define([ALL_BUILTINS_NO_DIGEST],
  m4_normalize(m4_foreach_w([BUILTIN], m4_defn([ALL_BUILTINS]),
    [m4_if(BUILTIN, [digest], [], [BUILTIN ])])))

if test -z "${enabled_builtins}"; then
  enabled_builtins="m4_defn([DEFAULT_BUILTINS])"
fi
AC_MSG_CHECKING([for builtins])
AC_ARG_ENABLE([builtins],[AC_HELP_STRING([--enable-builtins=LIST], [also enable these builtins (space or comma separated; "all" or "*" = every builtin)])],[
  for b in `echo "$enableval" | tr ',' ' '`; do
    case "$b" in
      all|'*') enabled_builtins="m4_defn([ALL_BUILTINS_NO_DIGEST])" ;;
      yes|no) ;;
      *) case " $enabled_builtins " in
           *" $b "*) ;;
           *) enabled_builtins="$enabled_builtins $b" ;;
         esac ;;
    esac
  done])
if test -n "${enabled_builtins}"; then
  enabled_builtins=`set -- ${enabled_builtins}; IFS=" "; echo "$*"`
fi
AC_MSG_RESULT([${enabled_builtins}])

m4_foreach_w([BUILTIN], m4_defn([ALL_BUILTINS]), [
case " ${enabled_builtins} " in
 *" BUILTIN "*)
AC_DEFINE_UNQUOTED([BUILTIN_]m4_toupper(BUILTIN),[1],[Build the ]BUILTIN[ builtin])
  ;;
esac
])

AC_DEFINE([HAVE_BUILTIN_CONFIG_H],[1],[builtin_table.c includes src/builtin_config.h])

dnl src/builtin_config.h: BUILTIN_<NAME> 1 or 0 for every builtin, what
dnl builtin_table.c and the eval/job/sh/term sources are compiled against
dnl (the CMake build writes the same file, see cmake/Builtins.cmake).
AC_CONFIG_COMMANDS([src/builtin_config.h],
[for b in $all_builtins; do
   ub=`echo $b | tr a-z A-Z`
   case " $enabled_builtins " in
     *" $b "*) echo "#define BUILTIN_$ub 1" ;;
     *) echo "#define BUILTIN_$ub 0" ;;
   esac
 done > src/builtin_config.h],
[all_builtins="m4_defn([ALL_BUILTINS])"
enabled_builtins="$enabled_builtins"])

AC_SUBST([BUILTINS],[$enabled_builtins])

# library subdirectories
# ---------------------------------------------------------------------------
dnl AC_CONFIG_LIBRARIES([lib/libowfat])
dnl AC_CONFIG_PROGRAMS([src/shish], [lib/libowfat.a])

AC_CONFIG_FILES([
Makefile
config.mk
build.mk
doc/Makefile
doc/posix/Makefile
doc/man/Makefile
lib/Makefile
lib/alloc/Makefile
lib/arena/Makefile
lib/buffer/Makefile
lib/byte/Makefile
lib/fmt/Makefile
lib/hashmap/Makefile
lib/mmap/Makefile
lib/open/Makefile
lib/path/Makefile
lib/scan/Makefile
lib/shell/Makefile
lib/sig/Makefile
lib/str/Makefile
lib/stralloc/Makefile
lib/uint32/Makefile
lib/uint64/Makefile
lib/unix/Makefile
lib/utf8/Makefile
lib/wait/Makefile
text/Makefile
text/dfa/Makefile
text/sed/Makefile
text/awk/Makefile
scripts/Makefile
src/Makefile
src/builtin/Makefile
src/builtin/extra/Makefile
src/debug/Makefile
src/eval/Makefile
src/exec/Makefile
src/expr/Makefile
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
src/source/Makefile
src/term/Makefile
src/trace/Makefile
src/tree/Makefile
src/var/Makefile
src/vartab/Makefile])

AC_CONFIG_STATUS

# be verbose :)
# ---------------------------------------------------------------------------
AC_SUMMARIZE([prefix bindir sysconfdir],
             [COLOR DEPS_TRUE DEBUG | CC CPPFLAGS CFLAGS LDFLAGS LIBS],
             "%20s: %s")

AC_MSG_NOTICE([Enabled builtins: ${enabled_builtins}])

