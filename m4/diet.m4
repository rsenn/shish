# $Id: diet.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# dietlibc compiling support
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_DIETLIBC],[DIETLIBC="no"
DIETDIRS="m4_default([$1], [/usr /opt/diet /usr/diet /usr/local/diet])"
DIETDIRS="$DIETDIRS $(echo $PATH | sed 's,:,\n,g' | sed -e 's,/s*bin,,g' | uniq)"
AC_ARG_VAR([DIET], [dietlibc compiler wrapper])
AC_ARG_WITH([dietlibc],[  --with-dietlibc=[[PREFIX]] compile with dietlibc (default)
  --without-dietlibc       do not compile with dietlibc],[if test "$withval" = yes -o "$withval" = no; then
  DIETLIBC="$withval"
else
  DIETLIBC="yes"
  DIETDIRS="$withval $DIETDIRS"
fi])
DIETLIBS="$DIETDIRS /"
if test "$DIETLIBC" = "yes"; then
  AC_MSG_CHECKING([for dietlibc])
  if test ! -x "$DIET"; then
    for dir in $DIETDIRS; do
      if test -x "$dir/bin/diet"; then
        DIETVERSION=$($dir/bin/diet 2>&1 | sed 's,[[a-z ]]*-,,;;s,^.*[[:-]].*$,,')
        
        if test -n "$DIETVERSION"; then
          DIET="$dir/bin/diet -Os"
          break
        fi
      fi
    done
  else
    DIETVERSION=$($dir/bin/diet 2>&1 | sed 's,[[a-z ]]*-,,;;s,^.*[[:-]].*$,,')
  fi
  
  DIETDIR=$($DIET -v $CC 2>&1 | sed 's,.*\-L,,;;s,/lib.*,,' | head -n1)
  DIETLIBS="$DIETDIR $DIETLIBS"

  if test -z "$DIETVERSION"; then
    DIET=""
    AC_MSG_RESULT([no])
  elif test "$CC" != "${CC%diet*cc}"; then
    DIET=""
    AC_MSG_RESULT([yes])
  else
    if test -e "$DIETDIR/lib64"; then
      LIBDIR="$DIETDIR/lib64"
    else
      LIBDIR="$DIETDIR/lib"
    fi
    
    for x in $DIETLIBS; do
      DIETLIB=$(ls -d $x/lib-* $x/lib* 2>/dev/null | head -n1)
      
      if test -f "$DIETLIB/start.o"; then
        LIBDIR="$DIETLIB"
        break
      fi
    done
    
    CPPFLAGS="$CPPFLAGS -nostdinc -D__dietlibc__ -idirafter $DIETDIR/include"
    CFLAGS="$CFLAGS"
#     LDFLAGS="$LDFLAGS -nostdlib -static -L$LIBDIR $LIBDIR/start.o"
    LDFLAGS="$LDFLAGS -nostdlib -static -L$LIBDIR"
    LIBS="$LIBS -lc -lgcc"
    AC_MSG_RESULT([$DIETVERSION])
  fi
fi

ac_cv_size_opt=yes

AC_ARG_ENABLE([size-opt],
[  --enable-size-opt        optimize by size (default)
  --disable-size-opt       disable optimization by size
],
[case "$enableval" in
  yes|no) ac_cv_size_opt="$enableval" ;;
	*) ac_cv_size_opt=yes ;;
esac])

if test "$ac_cv_size_opt" = yes; then
	
        if test "$ax_cv_c_compiler_vendor" = clang; then
          AX_CHECK_COMPILE_FLAG([-O5], [CFLAGS="$CFLAGS -O5"],
            [AX_CHECK_COMPILE_FLAG([-Os], [CFLAGS="$CFLAGS -Os"])])
	  AX_CHECK_COMPILE_FLAG([-mstack-alignment=4], [CFLAGS="$CFLAGS -mstack-alignment=4"])
        else
          :
       dnl   AX_CHECK_COMPILE_FLAG([-Os], [CFLAGS="$CFLAGS -Os"])
        fi
	AX_CHECK_COMPILE_FLAG([-flto], [CFLAGS="$CFLAGS -flto"])
	AX_CHECK_COMPILE_FLAG([-fno-inline], [CFLAGS="$CFLAGS -fno-inline"])
	AX_CHECK_COMPILE_FLAG([-fno-inline-functions], [CFLAGS="$CFLAGS -fno-inline-functions"])
	AX_CHECK_COMPILE_FLAG([-fno-inline-small-functions], [CFLAGS="$CFLAGS -fno-inline-small-functions"])
	AX_CHECK_COMPILE_FLAG([-ffunction-sections], [CFLAGS="$CFLAGS -ffunction-sections"])
	AX_CHECK_COMPILE_FLAG([-fdata-sections], [CFLAGS="$CFLAGS -fdata-sections"])
	AX_CHECK_COMPILE_FLAG([-mpreferred-stack-boundary=4], [CFLAGS="$CFLAGS -mpreferred-stack-boundary=4"])
	AX_CHECK_COMPILE_FLAG([-falign-functions=4], [CFLAGS="$CFLAGS -falign-functions=4"])
	AX_CHECK_COMPILE_FLAG([-falign-jumps=1], [CFLAGS="$CFLAGS -falign-jumps=1"])
	AX_CHECK_COMPILE_FLAG([-falign-loops=1], [CFLAGS="$CFLAGS -falign-loops=1"])
dnl	AX_CHECK_LINK_FLAG([-Wl,-s], [LDFLAGS="$LDFLAGS -Wl,-s"],
dnl	  [AX_CHECK_LINK_FLAG([-s], [LDFLAGS="$LDFLAGS -s"])])
fi

AM_CONDITIONAL([SIZE_OPT],[test "$ac_cv_size_opt" = yes])

dnl  AC_MSG_CHECKING([wheter compiler supports -falign])
dnl  saved_CFLAGS="$CFLAGS"
dnl  CFLAGS="$CFLAGS -mpreferred-stack-boundary=4 -falign-functions=4 -falign-jumps=1 -falign-loops=1"
dnl  AC_TRY_COMPILE([], [], [
dnl  AC_MSG_RESULT([yes])], [
dnl  AC_MSG_RESULT([no])
dnl  CFLAGS="$saved_CFLAGS"])
 
 
AC_SUBST(DIETLIBC)
AC_SUBST(DIET)
AC_SUBST(DIETDIR)
AC_SUBST(DIETLIB)
AC_SUBST(DIETVERSION)])
