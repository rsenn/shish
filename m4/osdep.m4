# $Id: osdep.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Configures generic compiling
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

m4_define([_AC_COMPILER_OBJEXT], [])


AC_DEFUN([AC_CONFIG_HOST], [
  case $host in
  *mingw32*)
    OBJEXT="o"
    ;;
  *)
    OBJEXT="o"
    ;;
  esac

ac_objext=$OBJEXT
AC_SUBST([OBJEXT])
])

