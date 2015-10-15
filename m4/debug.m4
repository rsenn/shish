# $Id: debug.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Sophisticated debugging mode macro for autoconf
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for debug mode
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_DEBUG], 
[ac_cv_debug="no"

AC_MSG_CHECKING([whether to enable debugging mode])

AC_ARG_ENABLE([debug],
[  --enable-debug           debugging mode
  --disable-debug          production mode (default)],
[case "$enableval" in
  yes)
    ac_cv_debug="yes"
    ;;
  no)
    ac_cv_debug="no"
    ;;
esac])

# remove initial debug flag, set by autoconf
#CFLAGS=`echo $CFLAGS | sed 's/-g//'`
CFLAGS=`echo $CFLAGS | sed 's/-O[0-9s]//'`

# set DEBUG variable
if test "$ac_cv_debug" = "yes"; then
  CFLAGS="`echo $CFLAGS -g -ggdb -O0`"
#  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1 -Werror -include assert.h`"
#  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1 -include assert.h`"
  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1`"
  DEBUG="yes"
  AC_MSG_RESULT([yes])
  AC_CHECK_LIB([duma], [DUMA_Exit])
else
	case "$CFLAGS" in
		  *-O*) ;;
		  *) CFLAGS="`echo $CFLAGS -Os -fomit-frame-pointer`" ;;
  esac
  CPPFLAGS="`echo $CPPFLAGS -DNDEBUG`"
  DEBUG="no"
  AC_MSG_RESULT([no])
fi
  
AM_CONDITIONAL([DEBUG],[test "$ac_cv_debug" = yes])
AC_SUBST(DEBUG)])
