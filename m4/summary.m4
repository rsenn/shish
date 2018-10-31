# $Id: summary.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# m4 macros for outputting a summary after the ./configure script ran
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>
#
m4_define([AC_SUMMARY_FORMAT], "%27s: %s")
#
# directories
# ---------------------------------------------------------------------------
m4_defun([AC_INIT_DIRS], [m4_define([AC_SWITCH_DIRS])
AC_ADD_DIR([prefix],         [Prefix])dnl
AC_ADD_DIR([bindir],         [User executables])dnl
AC_ADD_DIR([sbindir],        [System admin executables])dnl
AC_ADD_DIR([libexecdir],     [Program executables])dnl
AC_ADD_DIR([datadir],        [Read-only data])dnl
AC_ADD_DIR([sysconfdir],     [Configuration files])dnl
AC_ADD_DIR([sharedstatedir], [Portable data])dnl
AC_ADD_DIR([localstatedir],  [Binary data])dnl
AC_ADD_DIR([libdir],         [Libraries])dnl
AC_ADD_DIR([includedir],     [C header files])dnl
AC_ADD_DIR([oldincludedir],  [C header files for non-gcc])dnl
AC_ADD_DIR([infodir],        [Info documentation])dnl
AC_ADD_DIR([mandir],         [Manual pages])])
#
# variables
# ---------------------------------------------------------------------------
m4_defun([AC_INIT_VARS], [m4_define([AC_SWITCH_VARS])
AC_ADD_VAR([host],     [Host system])dnl
AC_ADD_VAR([build],    [Build system])dnl
AC_ADD_VAR([target],   [Target system])dnl
AC_ADD_VAR([CC],       [Compiler])dnl
AC_ADD_VAR([CXX],      [C++ compiler])dnl
AC_ADD_VAR([CFLAGS],   [Compiler flags])dnl
AC_ADD_VAR([PIC_CFLAGS], [PIC compiler flags])dnl
AC_ADD_VAR([CXXFLAGS], [C++ compiler flags])dnl
AC_ADD_VAR([DEFS],     [Preprocessor defines])dnl
AC_ADD_VAR([CPPFLAGS], [Preprocessor flags])dnl
AC_ADD_VAR([LDFLAGS],  [Linker flags])dnl
AC_ADD_VAR([PIE_LDFLAGS],  [PIE linker flags])dnl
AC_ADD_VAR([LIBS],     [Libraries])dnl
AC_ADD_VAR([A_ENABLE],     [Build static library])dnl
AC_ADD_VAR([PIE_ENABLE],   [Build shared library])dnl
AC_ADD_VAR([DLM_ENABLE],   [Build loadable modules])dnl
AC_ADD_VAR([SSL_CFLAGS],   [SSL includes])dnl
AC_ADD_VAR([SSL_LIBS],     [SSL libs])dnl
AC_ADD_VAR([FT2_CFLAGS],   [Freetype 2 includes])dnl
AC_ADD_VAR([FT2_LIBS],     [Freetype 2 libs])dnl
AC_ADD_VAR([PSQL_CFLAGS], [PostgreSQL includes])dnl
AC_ADD_VAR([PSQL_LIBS],   [PostgreSQL libs])dnl
AC_ADD_VAR([MYSQL_CFLAGS], [MySQL includes])dnl
AC_ADD_VAR([MYSQL_LIBS],   [MySQL libs])dnl
AC_ADD_VAR([SDL_CFLAGS],   [SDL includes])dnl
AC_ADD_VAR([SDL_LIBS],     [SDL libs])dnl
AC_ADD_VAR([LIBPNG_CFLAGS],   [libpng includes])dnl
AC_ADD_VAR([LIBPNG_LIBS],     [libpng libs])dnl
AC_ADD_VAR([DEP],      [Dependency tracking])dnl
AC_ADD_VAR([COLOR],    [Colorful makefile])dnl
AC_ADD_VAR([DEBUG],    [Debug build])])
#
# AC_SUMMARIZE
# ---------------------------------------------------------------------------
# print a summary of configured options after the script ran
#
# Syntax:
#
#   AC_SUMMARIZE([directories], [variables], [format]) 
#
#   [directories]     set the directories you want in the summary
#   [variables]       set the variables you want in the summary
#   [format]          format string for output line
#
# Example:
#
#   AC_SUMMARIZE([bindir sysconfdir], [CFLAGS LIBS])
#
AC_DEFUN([AC_SUMMARIZE],
[AC_SUMMARIZE_HEADER
echo
AC_SUMMARIZE_DIRS($1, $3)
echo
AC_SUMMARIZE_VARS($2, $3)
echo])
#
# AC_SUMMARIZE_DIRS
# ---------------------------------------------------------------------------
# print out a directory summary
#
# Syntax:
#
#   AC_SUMMARIZE_DIRS([directories], [format])
#
#   [directories]   names of variables you want to display,
#                   if none are specified, you'll get all
#   [format]          format string for output line
#
# Example:
#
#   AC_SUMMARIZE_DIRS([sbindir sysconfdir sharedstatedir])
#
AC_DEFUN([AC_SUMMARIZE_DIRS],
[m4_ifvaln([$2], m4_define([AC_SUMMARY_FORMAT], $2))AC_INIT_DIRS
# if no args given, display all substituted dirs
m4_ifvaln([$1], [ac_summarize_dirs="$1"], [ac_summarize_dirs="AC_DEFAULT_DIRS"])

# loop through substituted dirs and show info
for ac_summarize_dir in $ac_summarize_dirs
do
  case $ac_summarize_dir in AC_SWITCH_DIRS
    *)
      # print an empty line
      echo
      ;;
  esac
done])
#
# AC_SUMMARIZE_VARS
# ---------------------------------------------------------------------------
# print out a variable summary
#
# Syntax:
#
#   AC_SUMMARIZE_VARS([variables])
#
#   [variables]    names of variables you want to display,
#                  if none are specified, you'll get all
# Example:
#
#   AC_SUMMARIZE_VARS([CC CFLAGS CXX CXXFLAGS])
#
AC_DEFUN([AC_SUMMARIZE_VARS], 
[m4_ifvaln([$2], m4_define([AC_SUMMARY_FORMAT], $2))AC_INIT_VARS
# if no args given, display all substituted vars
m4_ifvaln([$1], [ac_summarize_vars="$1"], [ac_summarize_vars="AC_DEFAULT_VARS"])

# loop through substituted vars and show info
for ac_summarize_var in $ac_summarize_vars
do
  case $ac_summarize_var in AC_SWITCH_VARS
    *)
      # print an empty line
      echo
      ;;
  esac
done])
#
# AC_SUMMARIZE_HEADER
# ---------------------------------------------------------------------------
# print out a summary header
#
# Syntax:
#
#   AC_SUMMARIZE_HEADER
#
AC_DEFUN([AC_SUMMARIZE_HEADER],
[echo
echo "$PACKAGE_NAME-$PACKAGE_VERSION has been configured with the following options:"])
#
# AC_ADD_DIR
# ---------------------------------------------------------------------------
# internally used by summary functions
#
m4_defun([AC_ADD_DIR],
[m4_append_uniq([AC_DEFAULT_DIRS], [$1], [ ])m4_append([AC_SWITCH_DIRS], [    "$1")
      eval $1="$$1" ; eval $1="$$1"
      printf AC_SUMMARY_FORMAT "$2" ""
      AC_MSG_DIR([$$1]) ;;])])
#
# AC_ADD_VAR
# ---------------------------------------------------------------------------
# internally used by summary functions
#
m4_defun([AC_ADD_VAR],
[m4_append_uniq([AC_DEFAULT_VARS], [$1], [ ])m4_append([AC_SWITCH_VARS], [
    "$1")
      $1=`echo $$1`
      if test "$$1"; then
        printf AC_SUMMARY_FORMAT "$2" ""
        AC_MSG_RESULT([$$1])
      fi 
      ;;
])])
