# check for libowfat
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_LIBOWFAT], [
 AC_MSG_CHECKING([for libowfat prefix])


ac_cv_with_libowfat="/usr/local /usr /opt/diet"
AC_ARG_WITH(libowfat,
[  --with-libowfat[[=DIR]]   libowfat [[auto]]],
[
  case "$withval" in
    yes|no ) ;;
    *) ac_cv_with_libowfat="$withval" ;;
  esac
])
AC_MSG_RESULT($ac_cv_with_libowfat)

for ac_cv_libowfat_dir in $ac_cv_with_libowfat; do

	old_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -I${ac_cv_libowfat_dir}/include"

	AC_CHECK_HEADER([stralloc.h], [ac_cv_libowfat_stralloc_h_found=yes])
	AC_CHECK_HEADER([libowfat/stralloc.h], [LIBOWFAT_CFLAGS="-I${ac_cv_libowfat_dir}/include/libowfat"; ac_cv_libowfat_stralloc_h_found=yes])

	CPPFLAGS="$old_CPPFLAGS"
	AC_MSG_CHECKING([for libowfat in ${ac_cv_libowfat_dir}])


		for DIR in "${ac_cv_libowfat_dir}/lib/${host:+dummy}/" "${ac_cv_libowfat_dir}/lib64" "${ac_cv_libowfat_dir}/lib"; do
			if test -d "$DIR" ; then
				LIBOWFAT_LIBDIR="$DIR"
				break
			fi
		done

	if test "$ac_cv_libowfat_stralloc_h_found" = yes -a -d "${LIBOWFAT_LIBDIR}"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		continue
	fi

		LIBOWFAT_LIBS="-lowfat"
		if test -n "$LIBOWFAT_LIBDIR"; then
			LIBOWFAT_LIBS="-L${LIBOWFAT_LIBDIR} $LIBOWFAT_LIBS"
		fi
		break
done

	if ! test "$ac_cv_libowfat_stralloc_h_found" = yes -a -d "${LIBOWFAT_LIBDIR}"; then
		AC_MSG_ERROR([Need libowfat!])
	fi

AM_CONDITIONAL([LIBOWFAT], [test "$ac_cv_libowfat_stralloc_h_found" = yes])
AC_SUBST([LIBOWFAT_CFLAGS])
AC_SUBST([LIBOWFAT_LIBS])
])

