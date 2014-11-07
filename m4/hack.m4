# $Id: hack.m4,v 1.3 2006/09/27 10:52:28 roman Exp $
# ===========================================================================
#
# Hack autoconf default behaviour on some points
#

# AC_CONFIG_AUX_DIRS(DIR ...)
# ---------------------------
# Internal subroutine.
# Search for the configuration auxiliary files in directory list $1.
# We look only for config.guess, so users of AC_PROG_INSTALL
# do not automatically need to distribute the other auxiliary files.
AC_DEFUN([AC_CONFIG_AUX_DIRS],
[ac_aux_dir=
for ac_dir in $1; do
  if test -f $ac_dir/config.guess; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/config.guess -c"
    break
  fi
done
if test -z "$ac_aux_dir"; then
  AC_MSG_ERROR([cannot find config.guess in $1])
fi
ac_config_guess="$SHELL $ac_aux_dir/config.guess"
ac_config_sub="$SHELL $ac_aux_dir/config.sub"
ac_configure="$SHELL $ac_aux_dir/configure" # This should be Cygnus configure.
AC_PROVIDE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
])# AC_CONFIG_AUX_DIRS


dnl AC_DEFUN([AC_CANONICAL_BUILD], [])

dnl AC_DEFUN([AC_CONFIG_AUX_DIRS], [])

dnl m4_define([AC_EGREP_CPP])


AC_DEFUN([AC_SET_ARGS], 
[CONFIGURE_ARGS="$ac_configure_args"
AC_SUBST(ac_configure_args)
AC_SUBST(CONFIGURE_ARGS)])

# AC_PROG_CC([COMPILER ...])
# --------------------------
# COMPILER ... is a space separated list of C compilers to search for.
# This just gives the user an opportunity to specify an alternative
# search list for the C compiler.
AN_MAKEVAR([CC],  [AC_PROG_CC])
AN_PROGRAM([cc],  [AC_PROG_CC])
AN_PROGRAM([gcc], [AC_PROG_CC])
AC_DEFUN([AC_PROG_CC],
[AC_LANG_PUSH(C)dnl
AC_ARG_VAR([CC],     [C compiler command])dnl
AC_ARG_VAR([CFLAGS], [C compiler flags])dnl
_AC_ARG_VAR_LDFLAGS()dnl
_AC_ARG_VAR_CPPFLAGS()dnl
m4_ifval([$1],
      [AC_CHECK_TOOLS(CC, [$1])],
[AC_CHECK_TOOL(CC, gcc)
if test -z "$CC"; then
  AC_CHECK_TOOL(CC, cc)
fi
if test -z "$CC"; then
  AC_CHECK_PROG(CC, cc, cc, , , /usr/ucb/cc)
fi
if test -z "$CC"; then
  AC_CHECK_TOOLS(CC, cl)
fi
])
test -z "$CC" && AC_MSG_FAILURE([no acceptable C compiler found in \$PATH])
CC="$DIET $CC"
# Provide some information about the compiler.
echo "$as_me:$LINENO:" \
     "checking for _AC_LANG compiler version" >&AS_MESSAGE_LOG_FD
ac_compiler=`set X $ac_compile; echo $[2]`
_AC_EVAL([$ac_compiler --version </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -v </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -V </dev/null >&AS_MESSAGE_LOG_FD])

m4_expand_once([_AC_COMPILER_EXEEXT])[]dnl
m4_expand_once([_AC_COMPILER_OBJEXT])[]dnl
dnl _AC_LANG_COMPILER_GNU
dnl GCC=`test $ac_compiler_gnu = yes && echo yes`
dnl _AC_PROG_CC_G
dnl _AC_PROG_CC_STDC
dnl # Some people use a C++ compiler to compile C.  Since we use `exit',
dnl # in C++ we need to declare it.  In case someone uses the same compiler
dnl # for both compiling C and C++ we need to have the C++ compiler decide
dnl # the declaration of exit, since it's the most demanding environment.
dnl _AC_COMPILE_IFELSE([@%:@ifndef __cplusplus
dnl  choke me
dnl @%:@endif],
dnl       [_AC_PROG_CXX_EXIT_DECLARATION])
AC_LANG_POP(C)dnl
])# AC_PROG_CC


# _AC_COMPILER_EXEEXT
# -------------------------------------------------------
# hacked to not check for cross-compiling
m4_define([_AC_COMPILER_EXEEXT],
[AC_LANG_CONFTEST([AC_LANG_PROGRAM()])
ac_clean_files_save=$ac_clean_files
ac_clean_files="$ac_clean_files a.out a.exe b.out"
_AC_COMPILER_EXEEXT_DEFAULT
dnl _AC_COMPILER_EXEEXT_WORKS
rm -f a.out a.exe conftest$ac_cv_exeext b.out
ac_clean_files=$ac_clean_files_save
dnl _AC_COMPILER_EXEEXT_CROSS
_AC_COMPILER_EXEEXT_O
rm -f conftest.$ac_ext
AC_SUBST([EXEEXT], [$ac_cv_exeext])dnl
ac_exeext=$EXEEXT
])# _AC_COMPILER_EXEEXT

# AC_CONFIG_STATUS
# -------------------------------------------------------
# The big finish.
# Produce config.status and enter configure subdirs.
#
# The CONFIG_HEADERS are defined in the m4 variable AC_LIST_HEADERS.
# Pay special attention not to have too long here docs: some old
# shells die.  Unfortunately the limit is not known precisely...
#
AC_DEFUN([AC_CONFIG_STATUS],
[AC_CACHE_SAVE

PACKAGE_PREFIX="${PACKAGE_NAME%%-*}"

case $prefix in
  *$PACKAGE_PREFIX*)
    PREFIX=""
    ;;
  *)
    PREFIX="/${PACKAGE_NAME%%-*}"
    ;;
esac
AC_SUBST([PREFIX])

test "x$prefix" = xNONE && prefix=$ac_default_prefix
# Let make expand exec_prefix.
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'

# VPATH may cause trouble with some makes, so we remove $(srcdir),
# ${srcdir} and @srcdir@ from VPATH if srcdir is ".", strip leading and
# trailing colons and then remove the whole line if VPATH becomes empty
# (actually we leave an empty line to preserve line numbers).
if test "x$srcdir" = x.; then
  ac_vpsub=['/^[	 ]*VPATH[	 ]*=/{
s/:*\$(srcdir):*/:/;
s/:*\${srcdir}:*/:/;
s/:*@srcdir@:*/:/;
s/^\([^=]*=[	 ]*\):*/\1/;
s/:*$//;
s/^[^=]*=[	 ]*$//;
}']
fi

m4_ifset([AC_LIST_HEADERS], [DEFS=-DHAVE_CONFIG_H], [AC_OUTPUT_MAKE_DEFS()])

dnl Commands to run before creating config.status.
AC_OUTPUT_COMMANDS_PRE()dnl

: ${CONFIG_STATUS=./config.status}
ac_clean_files_save=$ac_clean_files
ac_clean_files="$ac_clean_files $CONFIG_STATUS"
_AC_OUTPUT_CONFIG_STATUS()
ac_clean_files=$ac_clean_files_save

dnl Commands to run after config.status was created
AC_OUTPUT_COMMANDS_POST()

# configure is writing to config.log, and then calls config.status.
# config.status does its own redirection, appending to config.log.
# Unfortunately, on DOS this fails, as config.log is still kept open
# by configure, so config.status won't be able to write to it; its
# output is simply discarded.  So we exec the FD to /dev/null,
# effectively closing config.log, so it can be properly (re)opened and
# appended to by config.status.  When coming back to configure, we
# need to make the FD available again.
if test "$no_create" != yes; then
  ac_cs_success=:
  ac_config_status_args="Makefile config.mk build.mk"
  test "$silent" = yes &&
    ac_config_status_args="$ac_config_status_args --silent-rules"
  exec AS_MESSAGE_LOG_FD>/dev/null
  $SHELL $CONFIG_STATUS $ac_config_status_args || ac_cs_success=false
  exec AS_MESSAGE_LOG_FD>>config.log
  # Use ||, not &&, to avoid exiting from the if with $? = 1, which
  # would make configure fail if this is the last instruction.
  $ac_cs_success || AS_EXIT([1])
fi
dnl config.status should not do recursion.
AC_PROVIDE_IFELSE([AC_CONFIG_SUBDIRS], [_AC_OUTPUT_SUBDIRS()])dnl
])# AC_MAKEFILE



# _AC_SRCPATHS(BUILD-DIR-NAME)
# ----------------------------
# Has been taken from autoconf and hacked to also 
# set $ac_thisname and $ac_thisdir
#
m4_define([_AC_SRCPATHS],
[ac_builddir=.

if test $1 != .; then
  ac_dir_name=`echo $1 | sed 's,^\.[[\\/]],,'`
  ac_dir_suffix="/$ac_dir_name"
  # A "../" for each directory in $ac_dir_suffix.
  ac_top_builddir=`echo "$ac_dir_suffix" | sed 's,/[[^\\/]]*,../,g'`
else
  ac_dir_name= ac_dir_suffix= ac_top_builddir=
fi

case $srcdir in
  .)  # No --srcdir option.  We are building in place.
    ac_srcdir=.
    if test -z "$ac_top_builddir"; then
       ac_top_srcdir=.
    else
       ac_top_srcdir=`echo $ac_top_builddir | sed 's,/$,,'`
    fi ;;
  [[\\/]]* | ?:[[\\/]]* )  # Absolute path.
    ac_srcdir=$srcdir$ac_dir_suffix;
    ac_top_srcdir=$srcdir ;;
  *) # Relative path.
    ac_srcdir=$ac_top_builddir$srcdir$ac_dir_suffix
    ac_top_srcdir=$ac_top_builddir$srcdir ;;
esac

ac_thisname="$ac_dir_name";
ac_thisdir="$ac_dir_name/";

# Do not use `cd foo && pwd` to compute absolute paths, because
# the directories may not exist.
dnl AS_SET_CATFILE([ac_abs_builddir],   [`pwd`],            [$1])
dnl AS_SET_CATFILE([ac_abs_top_builddir],
dnl                               [$ac_abs_builddir], [${ac_top_builddir}.])
dnl AS_SET_CATFILE([ac_abs_srcdir],     [$ac_abs_builddir], [$ac_srcdir])
dnl AS_SET_CATFILE([ac_abs_top_srcdir], [$ac_abs_builddir], [$ac_top_srcdir])
])# _AC_SRCPATHS






# _AC_OUTPUT_FILES
# ----------------
# Do the variable substitutions to create the Makefiles or whatever.
# This is a subroutine of AC_OUTPUT.
#
# It has to send itself into $CONFIG_STATUS (eg, via here documents).
# Upon exit, no here document shall be opened.
m4_define([___AC_OUTPUT_FILES],
[
SUBDIRS="$subdirs"
AC_SUBST(SUBDIRS)

cat >>$CONFIG_STATUS <<_ACEOF

#
# CONFIG_FILES section.
#

# No need to generate the scripts if there are no CONFIG_FILES.
# This happens for instance when ./config.status config.h
if test -n "\$CONFIG_FILES"; then
  # Protect against being on the right side of a sed subst in config.status.
dnl Please, pay attention that this sed code depends a lot on the shape
dnl of the sed commands issued by AC_SUBST.  So if you change one, change
dnl the other too.
[  sed 's/,@/@@/; s/@,/@@/; s/,;t t\$/@;t t/; /@;t t\$/s/[\\\\&,]/\\\\&/g;
   s/@@/,@/; s/@@/@,/; s/@;t t\$/,;t t/' >\$tmp/subs.sed <<\\CEOF]
dnl These here document variables are unquoted when configure runs
dnl but quoted when config.status runs, so variables are expanded once.
dnl Insert the sed substitutions of variables.
m4_ifdef([_AC_SUBST_VARS],
   [AC_FOREACH([AC_Var], m4_defn([_AC_SUBST_VARS]),
[s,@AC_Var@,$AC_Var,;t t
])])dnl
m4_ifdef([_AC_SUBST_FILES],
   [AC_FOREACH([AC_Var], m4_defn([_AC_SUBST_FILES]),
[/@AC_Var@/r $AC_Var
s,@AC_Var@,,;t t
])])dnl
CEOF

_ACEOF

  cat >>$CONFIG_STATUS <<\_ACEOF
  # Split the substitutions into bite-sized pieces for seds with
  # small command number limits, like on Digital OSF/1 and HP-UX.
dnl One cannot portably go further than 100 commands because of HP-UX.
dnl Here, there are 2 cmd per line, and two cmd are added later.
  ac_max_sed_lines=48
  ac_sed_frag=1 # Number of current file.
  ac_beg=1 # First line for current file.
  ac_end=$ac_max_sed_lines # Line after last line for current file.
  ac_more_lines=:
  ac_sed_cmds=
  while $ac_more_lines; do
    if test $ac_beg -gt 1; then
      sed "1,${ac_beg}d; ${ac_end}q" $tmp/subs.sed >$tmp/subs.frag
    else
      sed "${ac_end}q" $tmp/subs.sed >$tmp/subs.frag
    fi
    if test ! -s $tmp/subs.frag; then
      ac_more_lines=false
    else
      # The purpose of the label and of the branching condition is to
      # speed up the sed processing (if there are no `@' at all, there
      # is no need to browse any of the substitutions).
      # These are the two extra sed commands mentioned above.
      (echo [':t
  /@[a-zA-Z_][a-zA-Z_0-9]*@/!b'] && cat $tmp/subs.frag) >$tmp/subs-$ac_sed_frag.sed
      if test -z "$ac_sed_cmds"; then
  ac_sed_cmds="sed -f $tmp/subs-$ac_sed_frag.sed"
      else
  ac_sed_cmds="$ac_sed_cmds | sed -f $tmp/subs-$ac_sed_frag.sed"
      fi
      ac_sed_frag=`expr $ac_sed_frag + 1`
      ac_beg=$ac_end
      ac_end=`expr $ac_end + $ac_max_sed_lines`
    fi
  done
  if test -z "$ac_sed_cmds"; then
    ac_sed_cmds=cat
  fi
fi # test -n "$CONFIG_FILES"

_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF
for ac_file in : $CONFIG_FILES; do test "x$ac_file" = x: && continue
  # Support "outfile[:infile[:infile...]]", defaulting infile="outfile.in".
  case $ac_file in
  - | *:- | *:-:* ) # input from stdin
  cat >$tmp/stdin
  ac_file_in=`echo "$ac_file" | sed 's,[[^:]]*:,,'`
  ac_file=`echo "$ac_file" | sed 's,:.*,,'` ;;
  *:* ) ac_file_in=`echo "$ac_file" | sed 's,[[^:]]*:,,'`
  ac_file=`echo "$ac_file" | sed 's,:.*,,'` ;;
  * )   ac_file_in=$ac_file.in ;;
  esac

  # Compute @srcdir@, @top_srcdir@, and @INSTALL@ for subdirectories.
  ac_dir=`AS_DIRNAME(["$ac_file"])`
  AS_MKDIR_P(["$ac_dir"])
  _AC_SRCPATHS(["$ac_dir"])

AC_PROVIDE_IFELSE([AC_PROG_INSTALL],
[  case $INSTALL in
  [[\\/$]]* | ?:[[\\/]]* ) ac_INSTALL=$INSTALL ;;
  *) ac_INSTALL=$ac_top_builddir$INSTALL ;;
  esac
])dnl
  if test x"$ac_file" != x-; then
    AC_MSG_NOTICE([creating $ac_file])
    rm -f "$ac_file"
  fi
  # Let's still pretend it is `configure' which instantiates (i.e., don't
  # use $as_me), people would be surprised to read:
  #    /* config.h.  Generated by config.status.  */
  if test x"$ac_file" = x-; then
    configure_input=
  else
    configure_input="$ac_file.  "
  fi
  #configure_input=$configure_input"Generated from `echo $ac_file_in |
  #           sed 's,.*/,,'` by configure."

  # First look for the input files in the build tree, otherwise in the
  # src tree.
  ac_file_inputs=`IFS=:
    for f in $ac_file_in; do
      case $f in
      -) echo $tmp/stdin ;;
      [[\\/$]]*)
   # Absolute (can't be DOS-style, as IFS=:)
   test -f "$f" || AC_MSG_ERROR([cannot find input file: $f])
   echo "$f";;
      *) # Relative
   if test -f "$f"; then
     # Build tree
     echo "$f"
   elif test -f "$srcdir/$f"; then
     # Source tree
     echo "$srcdir/$f"
   else
     # /dev/null tree
     AC_MSG_ERROR([cannot find input file: $f])
   fi;;
      esac
    done` || AS_EXIT([1])
_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF
dnl Neutralize VPATH when `$srcdir' = `.'.
  sed "$ac_vpsub
dnl Shell code in configure.ac might set extrasub.
dnl FIXME: do we really want to maintain this feature?
$extrasub
_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF
:t
[/@[a-zA-Z_][a-zA-Z_0-9]*@/!b]
s,@configure_input@,$configure_input,;t t
s,@srcdir@,$ac_srcdir,;t t
s,@thisname@,$ac_thisname,;t t
s,@thisdir@,$ac_thisdir,;t t
s,@abs_srcdir@,$ac_abs_srcdir,;t t
s,@top_srcdir@,$ac_top_srcdir,;t t
s,@abs_top_srcdir@,$ac_abs_top_srcdir,;t t
s,@builddir@,$ac_builddir,;t t
s,@abs_builddir@,$ac_abs_builddir,;t t
s,@top_builddir@,$ac_top_builddir,;t t
s,@abs_top_builddir@,$ac_abs_top_builddir,;t t
AC_PROVIDE_IFELSE([AC_PROG_INSTALL], [s,@INSTALL@,$ac_INSTALL,;t t
])dnl
dnl The parens around the eval prevent an "illegal io" in Ultrix sh.
" $ac_file_inputs | (eval "$ac_sed_cmds") >$tmp/out
  rm -f $tmp/stdin
dnl This would break Makefile dependencies.
dnl  if diff $ac_file $tmp/out >/dev/null 2>&1; then
dnl    echo "$ac_file is unchanged"
dnl   else
dnl     rm -f $ac_file
dnl    mv $tmp/out $ac_file
dnl  fi
  if test x"$ac_file" != x-; then
    mv $tmp/out $ac_file
  else
    cat $tmp/out
    rm -f $tmp/out
  fi

m4_ifset([AC_LIST_FILES_COMMANDS],
[  # Run the commands associated with the file.
  case $ac_file in
AC_LIST_FILES_COMMANDS()dnl
  esac
])dnl
done
_ACEOF
])# _AC_OUTPUT_FILES



# _AC_OUTPUT_SUBDIRS
# ------------------
# This is a subroutine of AC_OUTPUT, but it does not go into
# config.status, rather, it is called after running config.status.
m4_define([_AC_OUTPUT_SUBDIRS],
[
#
# CONFIG_SUBDIRS section.
#
if test "$no_recursion" != yes; then
  
  # Remove --cache-file and --srcdir arguments so they do not pile up.
  ac_sub_configure_args=
  ac_prev=
  for ac_arg in $ac_configure_args; do
    if test -n "$ac_prev"; then
      ac_prev=
      continue
    fi
    case $ac_arg in
    -cache-file | --cache-file | --cache-fil | --cache-fi \
    | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
      ac_prev=cache_file ;;
    -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
    | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* \
    | --c=*)
      ;;
    --config-cache | -C)
      ;;
    -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
      ac_prev=srcdir ;;
    -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
      ;;
    -prefix | --prefix | --prefi | --pref | --pre | --pr | --p)
      ac_prev=prefix ;;
    -prefix=* | --prefix=* | --prefi=* | --pref=* | --pre=* | --pr=* | --p=*)
      ;;
    *) ac_sub_configure_args="$ac_sub_configure_args $ac_arg" ;;
    esac
  done

  # Always prepend --prefix to ensure using the same prefix
  # in subdir configurations.
  ac_sub_configure_args="--prefix=$prefix $ac_sub_configure_args"
  
  ac_popdir=`pwd`
  for ac_dir in : $subdirs; do test "x$ac_dir" = x: && continue
      
    # Do not complain, so a configure script can configure whichever
    # parts of a large source tree are present.
    test -d $srcdir/$ac_dir || continue
    
    AC_MSG_NOTICE([configuring in $ac_dir])
    AS_MKDIR_P(["$ac_dir"])
    _AC_SRCPATHS(["$ac_dir"])
    
    cd $ac_dir
      
    # Check for guested configure; otherwise get Cygnus style configure.
    if test -f $ac_srcdir/configure.gnu; then
      ac_sub_configure="$SHELL '$ac_srcdir/configure.gnu'"
    elif test -f $ac_srcdir/configure; then
      ac_sub_configure="$SHELL '$ac_srcdir/configure'"
    elif test -f $ac_srcdir/autogen.sh; then
        ac_sub_configure="$SHELL '$ac_srcdir/autogen.sh'"
    elif test -f $ac_srcdir/configure.in; then
      ac_sub_configure=$ac_configure
    else
      AC_MSG_WARN([no configuration information is in $ac_dir])
      ac_sub_configure=
    fi



    # The recursion is here.
    if test -n "$ac_sub_configure"; then
      # Make the cache file name correct relative to the subdirectory.
      case $cache_file in
      [[\\/]]* | ?:[[\\/]]* ) ac_sub_cache_file=$cache_file ;;
      *) # Relative path.
  ac_sub_cache_file=$ac_top_builddir$cache_file ;;
      esac

      AC_MSG_NOTICE([running $ac_sub_configure $ac_sub_configure_args --cache-file=$ac_sub_cache_file --srcdir=$ac_srcdir])
      # The eval makes quoting arguments work.
      (set -x; eval $ac_sub_configure $ac_sub_configure_args \
     --cache-file=$ac_sub_cache_file --srcdir=$ac_srcdir) ||
  AC_MSG_ERROR([$ac_sub_configure failed for $ac_dir])
    fi
    cd $ac_popdir
  done
fi
])# _AC_OUTPUT_SUBDIRS

# _AC_OUTPUT_CONFIG_STATUS
# ------------------------
# Produce config.status.  Called by AC_OUTPUT.
# Pay special attention not to have too long here docs: some old
# shells die.  Unfortunately the limit is not known precisely...
m4_define([___AC_OUTPUT_CONFIG_STATUS],
[AC_MSG_NOTICE([creating $CONFIG_STATUS])
cat >$CONFIG_STATUS <<_ACEOF
#! $SHELL
# Generated by $as_me.
# Run this file to recreate the current configuration.
# Compiler output produced by configure, useful for debugging
# configure, is in config.log if it exists.

debug=false
ac_cs_recheck=false
ac_cs_silent=false
SHELL=\${CONFIG_SHELL-$SHELL}
_ACEOF

cat >>$CONFIG_STATUS <<\_ACEOF
AS_SHELL_SANITIZE
dnl Watch out, this is directly the initializations, do not use
dnl AS_PREPARE, otherwise you'd get it output in the initialization
dnl of configure, not config.status.
_AS_PREPARE
exec AS_MESSAGE_FD>&1

# Open the log real soon, to keep \$[0] and so on meaningful, and to
# report actual input values of CONFIG_FILES etc. instead of their
# values after options handling.  Logging --version etc. is OK.
exec AS_MESSAGE_LOG_FD>>config.log
{
  echo
  AS_BOX([Running $as_me.])
} >&AS_MESSAGE_LOG_FD
cat >&AS_MESSAGE_LOG_FD <<_CSEOF

This file was extended by m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])dnl
$as_me[]m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION]), which was
generated by m4_PACKAGE_STRING.  Invocation command line was

  CONFIG_FILES    = $CONFIG_FILES
  CONFIG_HEADERS  = $CONFIG_HEADERS
  CONFIG_LINKS    = $CONFIG_LINKS
  CONFIG_COMMANDS = $CONFIG_COMMANDS
  $ $[0] $[@]

_CSEOF
echo "on `(hostname || uname -n) 2>/dev/null | sed 1q`" >&AS_MESSAGE_LOG_FD
echo >&AS_MESSAGE_LOG_FD
_ACEOF

# Files that config.status was made for.
if test -n "$ac_config_files"; then
  echo "config_files=\"$ac_config_files\"" >>$CONFIG_STATUS
fi

if test -n "$ac_config_headers"; then
  echo "config_headers=\"$ac_config_headers\"" >>$CONFIG_STATUS
fi

if test -n "$ac_config_links"; then
  echo "config_links=\"$ac_config_links\"" >>$CONFIG_STATUS
fi

if test -n "$ac_config_commands"; then
  echo "config_commands=\"$ac_config_commands\"" >>$CONFIG_STATUS
fi

cat >>$CONFIG_STATUS <<\_ACEOF

ac_cs_usage="\
\`$as_me' instantiates files from templates according to the
current configuration.

Usage: $[0] [[OPTIONS]] [[FILE]]...

  -h, --help       print this help, then exit
  -V, --version    print version number, then exit
  -q, --silent-rules      do not print progress messages
  -d, --debug      don't remove temporary files
      --recheck    update $as_me by reconfiguring in the same conditions
m4_ifset([AC_LIST_FILES],
[[  --file=FILE[:TEMPLATE]
		   instantiate the configuration file FILE
]])dnl
m4_ifset([AC_LIST_HEADERS],
[[  --header=FILE[:TEMPLATE]
		   instantiate the configuration header FILE
]])dnl

m4_ifset([AC_LIST_FILES],
[Configuration files:
$config_files

])dnl
m4_ifset([AC_LIST_HEADERS],
[Configuration headers:
$config_headers

])dnl
m4_ifset([AC_LIST_LINKS],
[Configuration links:
$config_links

])dnl
m4_ifset([AC_LIST_COMMANDS],
[Configuration commands:
$config_commands

])dnl
Report bugs to <bug-autoconf@gnu.org>."
_ACEOF

cat >>$CONFIG_STATUS <<_ACEOF
ac_cs_version="\\
m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])config.status[]dnl
m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION])
configured by $[0], generated by m4_PACKAGE_STRING,
  with options \\"`echo "$ac_configure_args" | sed 's/[[\\""\`\$]]/\\\\&/g'`\\"

Copyright (C) 2003 Free Software Foundation, Inc.
This config.status script is free software; the Free Software Foundation
gives unlimited permission to copy, distribute and modify it."
srcdir=$srcdir
AC_PROVIDE_IFELSE([AC_PROG_INSTALL],
[dnl Leave those double quotes here: this $INSTALL is evaluated in a
dnl here document, which might result in `INSTALL=/bin/install -c'.
INSTALL="$INSTALL"
])dnl
_ACEOF

cat >>$CONFIG_STATUS <<\_ACEOF
# If no file are specified by the user, then we need to provide default
# value.  By we need to know if files were specified by the user.
ac_need_defaults=:
while test $[#] != 0
do
  case $[1] in
  --*=*)
    ac_option=`expr "x$[1]" : 'x\([[^=]]*\)='`
    ac_optarg=`expr "x$[1]" : 'x[[^=]]*=\(.*\)'`
    ac_shift=:
    ;;
  -*)
    ac_option=$[1]
    ac_optarg=$[2]
    ac_shift=shift
    ;;
  *) # This is not an option, so the user has probably given explicit
     # arguments.
     ac_option=$[1]
     ac_need_defaults=false;;
  esac

  case $ac_option in
  # Handling of the options.
_ACEOF
cat >>$CONFIG_STATUS <<\_ACEOF
  -recheck | --recheck | --rechec | --reche | --rech | --rec | --re | --r)
    ac_cs_recheck=: ;;
  --version | --vers* | -V )
    echo "$ac_cs_version"; exit 0 ;;
  --he | --h)
    # Conflict between --help and --header
    AC_MSG_ERROR([ambiguous option: $[1]
Try `$[0] --help' for more information.]);;
  --help | --hel | -h )
    echo "$ac_cs_usage"; exit 0 ;;
  --debug | --d* | -d )
    debug=: ;;
  --file | --fil | --fi | --f )
    $ac_shift
    CONFIG_FILES="$CONFIG_FILES $ac_optarg"
    ac_need_defaults=false;;
  --header | --heade | --head | --hea )
    $ac_shift
    CONFIG_HEADERS="$CONFIG_HEADERS $ac_optarg"
    ac_need_defaults=false;;
  -q | -silent-rules | --silent-rules | --quie | --qui | --qu | --q \
  | -silent | --silent | --silen | --sile | --sil | --si | --s)
    ac_cs_silent=: ;;

  # This is an error.
  -*) AC_MSG_ERROR([unrecognized option: $[1]
Try `$[0] --help' for more information.]) ;;

  *) ac_config_targets="$ac_config_targets $[1]" ;;

  esac
  shift
done

ac_configure_extra_args=

if $ac_cs_silent; then
  exec AS_MESSAGE_FD>/dev/null
  ac_configure_extra_args="$ac_configure_extra_args --silent"
fi

_ACEOF
cat >>$CONFIG_STATUS <<_ACEOF
if \$ac_cs_recheck; then
  echo "running $SHELL $[0] " $ac_configure_args \$ac_configure_extra_args " --no-create --no-recursion" >&AS_MESSAGE_FD
  exec $SHELL $[0] $ac_configure_args \$ac_configure_extra_args --no-create --no-recursion
fi

_ACEOF

dnl We output the INIT-CMDS first for obvious reasons :)
m4_ifset([_AC_OUTPUT_COMMANDS_INIT],
[cat >>$CONFIG_STATUS <<_ACEOF
#
# INIT-COMMANDS section.
#

_AC_OUTPUT_COMMANDS_INIT()
_ACEOF])


dnl Issue this section only if there were actually config files.
dnl This checks if one of AC_LIST_HEADERS, AC_LIST_FILES, AC_LIST_COMMANDS,
dnl or AC_LIST_LINKS is set.
m4_ifval(AC_LIST_HEADERS()AC_LIST_LINKS()AC_LIST_FILES()AC_LIST_COMMANDS(),
[
cat >>$CONFIG_STATUS <<\_ACEOF
for ac_config_target in $ac_config_targets
do
  case "$ac_config_target" in
  # Handling of arguments.
AC_FOREACH([AC_File], AC_LIST_FILES,
[  "m4_bpatsubst(AC_File, [:.*])" )dnl
 CONFIG_FILES="$CONFIG_FILES AC_File" ;;
])dnl
AC_FOREACH([AC_File], AC_LIST_LINKS,
[  "m4_bpatsubst(AC_File, [:.*])" )dnl
 CONFIG_LINKS="$CONFIG_LINKS AC_File" ;;
])dnl
AC_FOREACH([AC_File], AC_LIST_COMMANDS,
[  "m4_bpatsubst(AC_File, [:.*])" )dnl
 CONFIG_COMMANDS="$CONFIG_COMMANDS AC_File" ;;
])dnl
AC_FOREACH([AC_File], AC_LIST_HEADERS,
[  "m4_bpatsubst(AC_File, [:.*])" )dnl
 CONFIG_HEADERS="$CONFIG_HEADERS AC_File" ;;
])dnl
  *) CONFIG_HEADERS="$CONFIG_HEADERS $ac_config_target" ;;
  esac
done

# If the user did not use the arguments to specify the items to instantiate,
# then the envvar interface is used.  Set only those that are not.
# We use the long form for the default assignment because of an extremely
# bizarre bug on SunOS 4.1.3.
if $ac_need_defaults; then
m4_ifset([AC_LIST_FILES],
[  test "${CONFIG_FILES+set}" = set || CONFIG_FILES=$config_files
])dnl
m4_ifset([AC_LIST_HEADERS],
[  test "${CONFIG_HEADERS+set}" = set || CONFIG_HEADERS=$config_headers
])dnl
m4_ifset([AC_LIST_LINKS],
[  test "${CONFIG_LINKS+set}" = set || CONFIG_LINKS=$config_links
])dnl
m4_ifset([AC_LIST_COMMANDS],
[  test "${CONFIG_COMMANDS+set}" = set || CONFIG_COMMANDS=$config_commands
])dnl
fi

# Have a temporary directory for convenience.  Make it in the build tree
# simply because there is no reason to put it here, and in addition,
# creating and moving files from /tmp can sometimes cause problems.
AS_TMPDIR([confstat], [.])

_ACEOF
])[]dnl m4_ifval

dnl The following four sections are in charge of their own here
dnl documenting into $CONFIG_STATUS.
m4_ifset([AC_LIST_FILES],    [_AC_OUTPUT_FILES()])dnl
m4_ifset([AC_LIST_HEADERS],  [_AC_OUTPUT_HEADERS()])dnl
m4_ifset([AC_LIST_LINKS],    [_AC_OUTPUT_LINKS()])dnl
m4_ifset([AC_LIST_COMMANDS], [_AC_OUTPUT_COMMANDS()])dnl

cat >>$CONFIG_STATUS <<\_ACEOF

AS_EXIT(0)
_ACEOF
chmod +x $CONFIG_STATUS
])# _AC_OUTPUT_CONFIG_STATUS

# _AC_OUTPUT_HEADERS
# ------------------
#
# Output the code which instantiates the `config.h' files from their
# `config.h.in'.
#
# This is a subroutine of _AC_OUTPUT_CONFIG_STATUS.  It has to send
# itself into $CONFIG_STATUS (eg, via here documents).  Upon exit, no
# here document shall be opened.
#
#
# The code produced used to be extremely costly: there are was a
# single sed script (n lines) handling both `#define' templates,
# `#undef' templates with trailing space, and `#undef' templates
# without trailing spaces.  The full script was run on each of the m
# lines of `config.h.in', i.e., about n x m.
#
# Now there are two scripts: `conftest.defines' for the `#define'
# templates, and `conftest.undef' for the `#undef' templates.
#
# Optimization 1.  It is incredibly costly to run two `#undef'
# scripts, so just remove trailing spaces first.  Removes about a
# third of the cost.
#
# Optimization 2.  Since `#define' are rare and obsoleted,
# `conftest.defines' is built and run only if grep says there are
# `#define'.  Improves by at least a factor 2, since in addition we
# avoid the cost of *producing* the sed script.
#
# Optimization 3.  In each script, first check that the current input
# line is a template.  This avoids running the full sed script on
# empty lines and comments (divides the cost by about 3 since each
# template chunk is typically a comment, a template, an empty line).
#
# Optimization 4.  Once a substitution performed, since there can be
# only one per line, immediately restart the script on the next input
# line (using the `t' sed instruction).  Divides by about 2.
# *Note:* In the case of the AC_SUBST sed script (_AC_OUTPUT_FILES)
# this optimization cannot be applied as is, because there can be
# several substitutions per line.
#
#
# The result is about, hm, ... times blah... plus....  Ahem.  The
# result is about much faster.
m4_define([_AC_OUTPUT_HEADERS],
[cat >>$CONFIG_STATUS <<\_ACEOF

#
# CONFIG_HEADER section.
#

# These sed commands are passed to sed as "A NAME B NAME C VALUE D", where
# NAME is the cpp macro being defined and VALUE is the value it is being given.
#
# ac_d sets the value in "#define NAME VALUE" lines.
dnl Double quote for the `[ ]' and `define'.
[ac_dA='s,^\([	 ]*\)#\([	 ]*define[	 ][	 ]*\)'
ac_dB='[	 ].*$,\1#\2'
ac_dC=' '
ac_dD=',;t'
# ac_u turns "#undef NAME" without trailing blanks into "#define NAME VALUE".
ac_uA='s,^\([	 ]*\)#\([	 ]*\)undef\([	 ][	 ]*\)'
ac_uB='$,\1#\2define\3'
ac_uC=' '
ac_uD=',;t']

for ac_file in : $CONFIG_HEADERS; do test "x$ac_file" = x: && continue
  # Support "outfile[:infile[:infile...]]", defaulting infile="outfile.in".
  case $ac_file in
  - | *:- | *:-:* ) # input from stdin
	cat >$tmp/stdin
	ac_file_in=`echo "$ac_file" | sed 's,[[^:]]*:,,'`
	ac_file=`echo "$ac_file" | sed 's,:.*,,'` ;;
  *:* ) ac_file_in=`echo "$ac_file" | sed 's,[[^:]]*:,,'`
	ac_file=`echo "$ac_file" | sed 's,:.*,,'` ;;
  * )   ac_file_in=$ac_file.in ;;
  esac

  test x"$ac_file" != x- && AC_MSG_NOTICE([creating $ac_file])

  # First look for the input files in the build tree, otherwise in the
  # src tree.
  ac_file_inputs=`IFS=:
    for f in $ac_file_in; do
      case $f in
      -) echo $tmp/stdin ;;
      [[\\/$]]*)
	 # Absolute (can't be DOS-style, as IFS=:)
	 test -f "$f" || AC_MSG_ERROR([cannot find input file: $f])
	 # Do quote $f, to prevent DOS paths from being IFS'd.
	 echo "$f";;
      *) # Relative
	 if test -f "$f"; then
	   # Build tree
	   echo "$f"
	 elif test -f "$srcdir/$f"; then
	   # Source tree
	   echo "$srcdir/$f"
	 else
	   # /dev/null tree
	   AC_MSG_ERROR([cannot find input file: $f])
	 fi;;
      esac
    done` || AS_EXIT([1])
  # Remove the trailing spaces.
  sed 's/[[	 ]]*$//' $ac_file_inputs >$tmp/in

_ACEOF

# Transform confdefs.h into two sed scripts, `conftest.defines' and
# `conftest.undefs', that substitutes the proper values into
# config.h.in to produce config.h.  The first handles `#define'
# templates, and the second `#undef' templates.
# And first: Protect against being on the right side of a sed subst in
# config.status.  Protect against being in an unquoted here document
# in config.status.
rm -f conftest.defines conftest.undefs
# Using a here document instead of a string reduces the quoting nightmare.
# Putting comments in sed scripts is not portable.
#
# `end' is used to avoid that the second main sed command (meant for
# 0-ary CPP macros) applies to n-ary macro definitions.
# See the Autoconf documentation for `clear'.
cat >confdef2sed.sed <<\_ACEOF
dnl Double quote for `[ ]' and `define'.
[s/[\\&,]/\\&/g
s,[\\$`],\\&,g
t clear
: clear
s,^[	 ]*#[	 ]*define[	 ][	 ]*\([^	 (][^	 (]*\)\(([^)]*)\)[	 ]*\(.*\)$,${ac_dA}\1${ac_dB}\1\2${ac_dC}\3${ac_dD},gp
t end
s,^[	 ]*#[	 ]*define[	 ][	 ]*\([^	 ][^	 ]*\)[	 ]*\(.*\)$,${ac_dA}\1${ac_dB}\1${ac_dC}\2${ac_dD},gp
: end]
_ACEOF
# If some macros were called several times there might be several times
# the same #defines, which is useless.  Nevertheless, we may not want to
# sort them, since we want the *last* AC-DEFINE to be honored.
uniq confdefs.h | sed -n -f confdef2sed.sed >conftest.defines
sed 's/ac_d/ac_u/g' conftest.defines >conftest.undefs
rm -f confdef2sed.sed

# This sed command replaces #undef with comments.  This is necessary, for
# example, in the case of _POSIX_SOURCE, which is predefined and required
# on some systems where configure will not decide to define it.
cat >>conftest.undefs <<\_ACEOF
[s,^[	 ]*#[	 ]*undef[	 ][	 ]*[a-zA-Z_][a-zA-Z_0-9]*,/* & */,]
_ACEOF

# Break up conftest.defines because some shells have a limit on the size
# of here documents, and old seds have small limits too (100 cmds).
echo '  # Handle all the #define templates only if necessary.' >>$CONFIG_STATUS
echo '  if grep ["^[	 ]*#[	 ]*define"] $tmp/in >/dev/null; then' >>$CONFIG_STATUS
echo '  # If there are no defines, we may have an empty if/fi' >>$CONFIG_STATUS
echo '  :' >>$CONFIG_STATUS
rm -f conftest.tail
while grep . conftest.defines >/dev/null
do
  # Write a limited-size here document to $tmp/defines.sed.
  echo '  cat >$tmp/defines.sed <<CEOF' >>$CONFIG_STATUS
  # Speed up: don't consider the non `#define' lines.
  echo ['/^[	 ]*#[	 ]*define/!b'] >>$CONFIG_STATUS
  # Work around the forget-to-reset-the-flag bug.
  echo 't clr' >>$CONFIG_STATUS
  echo ': clr' >>$CONFIG_STATUS
  #sed ${ac_max_here_lines:-\$}q conftest.defines >>$CONFIG_STATUS
  cat conftest.defines >>$CONFIG_STATUS
  echo 'CEOF
  sed -f $tmp/defines.sed $tmp/in >$tmp/out
  rm -f $tmp/in
  mv $tmp/out $tmp/in
' >>$CONFIG_STATUS
  #sed 1,${ac_max_here_lines:-\$}d conftest.defines >conftest.tail
  echo -n >conftest.tail
  rm -f conftest.defines
  mv conftest.tail conftest.defines
done
#rm -f conftest.defines
echo '  fi # grep' >>$CONFIG_STATUS
echo >>$CONFIG_STATUS

# Break up conftest.undefs because some shells have a limit on the size
# of here documents, and old seds have small limits too (100 cmds).
echo '  # Handle all the #undef templates' >>$CONFIG_STATUS
rm -f conftest.tail
while grep . conftest.undefs >/dev/null
do
  # Write a limited-size here document to $tmp/undefs.sed.
  echo '  cat >$tmp/undefs.sed <<CEOF' >>$CONFIG_STATUS
  # Speed up: don't consider the non `#undef'
  echo ['/^[	 ]*#[	 ]*undef/!b'] >>$CONFIG_STATUS
  # Work around the forget-to-reset-the-flag bug.
  echo 't clr' >>$CONFIG_STATUS
  echo ': clr' >>$CONFIG_STATUS
  #sed ${ac_max_here_lines}q conftest.undefs >>$CONFIG_STATUS
  cat conftest.undefs >>$CONFIG_STATUS
  echo 'CEOF
  sed -f $tmp/undefs.sed $tmp/in >$tmp/out
  rm -f $tmp/in
  mv $tmp/out $tmp/in
' >>$CONFIG_STATUS
  #sed 1,${ac_max_here_lines:-\$}d conftest.undefs >conftest.tail
  echo -n >conftest.tail
  rm -f conftest.undefs
  mv conftest.tail conftest.undefs
done
rm -f conftest.undefs

dnl Now back to your regularly scheduled config.status.
cat >>$CONFIG_STATUS <<\_ACEOF
  # Let's still pretend it is `configure' which instantiates (i.e., don't
  # use $as_me), people would be surprised to read:
  #    /* config.h.  Generated by config.status.  */
#  if test x"$ac_file" = x-; then
#    echo "/* Generated by configure.  */" >$tmp/config.h
#  else
#    echo "/* $ac_file.  Generated by configure.  */" >$tmp/config.h
#  fi
  cat $tmp/in >$tmp/config.h
  rm -f $tmp/in
  if test x"$ac_file" != x-; then
    if diff $ac_file $tmp/config.h >/dev/null 2>&1; then
      AC_MSG_NOTICE([$ac_file is unchanged])
    else
      ac_dir=`AS_DIRNAME(["$ac_file"])`
      AS_MKDIR_P(["$ac_dir"])
      rm -f $ac_file
      mv $tmp/config.h $ac_file
    fi
  else
    cat $tmp/config.h
    rm -f $tmp/config.h
  fi
dnl If running for Automake, be ready to perform additional
dnl commands to set up the timestamp files.
m4_ifdef([_AC_AM_CONFIG_HEADER_HOOK],
	 [_AC_AM_CONFIG_HEADER_HOOK([$ac_file])
])dnl
m4_ifset([AC_LIST_HEADERS_COMMANDS],
[  # Run the commands associated with the file.
  case $ac_file in
AC_LIST_HEADERS_COMMANDS()dnl
  esac
])dnl
done
_ACEOF
])# _AC_OUTPUT_HEADERS
