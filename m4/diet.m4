# $Id: diet.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# dietlibc compiling support
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_DIETLIBC],
[DIETLIBC="no"
DIETDIRS="m4_default([$1], [/usr /opt/diet /usr/diet /usr/local/diet])"
DIETDIRS="$DIETDIRS `echo $PATH | sed 's,:,\n,g' | sed -e 's,/s*bin,,g' | uniq`"
AC_ARG_VAR([DIET], [dietlibc compiler wrapper])
AC_ARG_WITH([dietlibc],
[  --with-dietlibc=[[PREFIX]] compile with dietlibc (default)
  --without-dietlibc       do not compile with dietlibc],
[case $withval in
  yes)
    DIETLIBC="yes"
    ;;
  no)
    DIETLIBC="no"
    ;;
  *)
    DIETLIBC="yes"
    DIETDIRS="$withval $DIETDIRS"
    ;;
  esac])
 DIETLIBS="$DIETDIRS /"
 if test "$DIETLIBC" = "yes"
 then
   AC_MSG_CHECKING([for dietlibc])
   if test ! -x "$DIET"
   then
     for dir in $DIETDIRS
     do
       if test -x "$dir/bin/diet"
       then
         DIETVERSION="`$dir/bin/diet 2>&1 | sed 's,[[a-z ]]*-,,;;s,^.*[[:-]].*$,,'`"
         
         if test -n "$DIETVERSION"
         then
           DIET="$dir/bin/diet -Os"
           break
         fi
       fi
     done
   else
     DIETVERSION="`$dir/bin/diet 2>&1 | sed 's,[[a-z ]]*-,,;;s,^.*[[:-]].*$,,'`"
   fi
   
   DIETDIR="$($DIET -v $CC 2>&1 | sed 's,.*\-L,,;;s,/lib.*,,' | head -n1)"
   DIETLIBS="$DIETDIR $DIETLIBS"

   if test -z "$DIETVERSION"
   then
     DIET=""
     AC_MSG_RESULT([no])
   elif test "$CC" != "${CC%diet*cc}"
   then
     DIET=""
     AC_MSG_RESULT([yes])
   else
dnl     DIET=""
   
	    
		 test -e "$DIETDIR/lib64" && 
     LIBDIR="$DIETDIR/lib64" ||
     LIBDIR="$DIETDIR/lib"
     
     for x in $DIETLIBS
     do     
       DIETLIB="`ls -d $x/lib-* $x/lib* 2>/dev/null | head -n1`"
       
       if test -f "$DIETLIB/start.o"
       then 
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
 
 AC_MSG_CHECKING([wheter compiler supports -falign])
 saved_CFLAGS="$CFLAGS"
 CFLAGS="$CFLAGS -mpreferred-stack-boundary=4 -falign-functions=4 -falign-jumps=1 -falign-loops=1"
 AC_TRY_COMPILE([], [], [
 AC_MSG_RESULT([yes])], [
 AC_MSG_RESULT([no])
 CFLAGS="$saved_CFLAGS"])
 
AC_SUBST(DIETLIBC)
AC_SUBST(DIET)
AC_SUBST(DIETDIR)
AC_SUBST(DIETLIB)
AC_SUBST(DIETVERSION)])
