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
DEPFLAGS=
if test "$ac_cv_dependency_tracking" = yes; then
  # -MMD (gcc, clang), else -MD (tcc); neither -> no tracking
  for ac_dep_flag in -MMD -MD; do
    AC_MSG_CHECKING([whether $CC supports $ac_dep_flag])
    ac_save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $ac_dep_flag"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
      [ac_dep_ok=yes], [ac_dep_ok=no])
    CFLAGS="$ac_save_CFLAGS"
    AC_MSG_RESULT([$ac_dep_ok])
    if test "$ac_dep_ok" = yes; then
      DEPFLAGS=$ac_dep_flag
      break
    fi
  done
  if test -z "$DEPFLAGS"; then
    AC_MSG_WARN([$CC has no -MMD/-MD, disabling dependency tracking])
    ac_cv_dependency_tracking=no
  fi
fi
AC_SUBST(DEPFLAGS)
if test "$ac_cv_dependency_tracking" = yes; then
  DEP_DISABLED="#"
else
  DEP_ENABLED="# "
fi
AM_CONDITIONAL([DEPS],[test "$ac_cv_dependency_tracking" = yes])
AC_SUBST(DEP_ENABLED,[$DEP_ENABLED])
AC_SUBST(DEP_DISABLED, [$DEP_DISABLED])
])
