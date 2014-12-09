# $Id: info.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# obsolete? huh?
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>


# set some info stuff
# ---------------------------------------------------------------------------
AC_DEFUN([AC_SET_INFO], [
  CREATION=$(date)
  PLATFORM=$($(CC) -dumpmachine 2>&/dev/null || uname -a)
  RELEASE="$*"

  AC_DEFINE(CREATION, $CREATION, [Creation time of this package])
  AC_DEFINE(PLATFORM, $PLATFORM, [Platform this package was compiled for])
  AC_DEFINE(RELEASE, $RELEASE, [Define to the release name of this package.])
  
  AC_SUBST(CREATION)
  AC_SUBST(PLATFORM)
  AC_SUBST(RELEASE)
])
