# $Id: dep.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Implement dependency tracking
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_DEP], [ac_cv_dependency_tracking=no
AC_MSG_CHECKING([whether to enable dependencies])
AC_ARG_ENABLE([dependency-tracking],[ --enable-dependency-tracking             dependency tracking
  --disable-dependency-tracking            no dependency tracking (default)], [case $enableval in
  yes) ac_cv_dependency_tracking=yes ;;
esac])
AC_MSG_RESULT([$ac_cv_dependency_tracking])
if test "$ac_cv_dependency_tracking" = yes; then
  DEP_DISABLED="#"
else
  DEP_ENABLED="# "
fi
AM_CONDITIONAL([DEPS],[test "$ac_cv_dependency_tracking" = yes])
AC_SUBST(DEP_ENABLED,[$DEP_ENABLED])
AC_SUBST(DEP_DISABLED, [$DEP_DISABLED])
])
