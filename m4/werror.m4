# $Id: werror.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Treat warnings as errors
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for werror mode
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_WERROR], 
[WARNFLAGS="-Wall -Wextra"

ac_cv_werror=no
AC_MSG_CHECKING([whether to enable -Werror])
AC_ARG_ENABLE([werror],
[  --enable-werror          treat warnings as errors],
[case "$enableval" in
  yes) ac_cv_werror=yes ;;
esac])

WARNFLAGS="$WARNFLAGS -Wno-unused"
WARNFLAGS="$WARNFLAGS -Wno-unused-parameter"
WARNFLAGS="$WARNFLAGS -Wno-missing-field-initializers"

# set -Werror compoiler flag
if test "$ac_cv_werror" = "yes"; then
  WARNFLAGS="$WARNFLAGS -Werror"
  CFLAGS="$CFLAGS $WARNFLAGS"
  #CPPFLAGS="$CPPFLAGS $WARNFLAGS"
  AC_MSG_RESULT([yes])
else
  WARNFLAGS=`echo "$WARNFLAGS" | sed 's|\s*-Werror| |g'`
  CPPFLAGS=`echo $CPPFLAGS | sed 's|\s*-Werror\s*| |g'`
  CFLAGS=`echo $CFLAGS | sed 's|\s*-Werror\s*| |g'`
  AC_MSG_RESULT([no])
fi
  
AC_SUBST([WARNFLAGS])
AM_CONDITIONAL([WERROR], [test "$ac_cv_werror" = yes])
])
