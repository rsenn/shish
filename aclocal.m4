# generated automatically by aclocal 1.7.8 -*- Autoconf -*-

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# $Id: color.m4,v 1.14 2005/05/03 13:04:01 smoli Exp $
# ===========================================================================
#
# implement colorful configure scripts :)
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# color names
# ---------------------------------------------------------------------------
m4_define([AC_COLOR_NAMES], [DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN,
                             LIGHT_RED, LIGHT_PURPLE, YELLOW, WHITE, BLACK, 
                             BLUE, GREEN, CYAN, RED, PURPLE, BROWN, GRAY])
                             
# types color can be bound to
# ---------------------------------------------------------------------------
m4_define([AC_COLOR_TYPES], [YES, NO, NONE, FILE, DIR, VAR, EXT, LIB, PROG, 
                             INFO, ARGS, MK, DEFAULT])
  
# AC_MSG_RESULT
# ---------------------------------------------------------------------------
# if result color is chooseable at macro-expansion time then output result,
# else generate a runtime evaluator
#
AC_DEFUN([AC_MSG_RESULT], 
[m4_bmatch([$1],
  [^$],                  [AC_MSG_NONE],
  [^none],               [AC_MSG_NONE([$1])],
  [yes],                 [AC_MSG_YES],
  [no],                  [AC_MSG_NO],
  
  [ac_file],             [AC_MSG_FILE([$1])],
  [objext],              [AC_MSG_FILE([$1])],
  [exeext],              [AC_MSG_FILE([$1])],
  [lex_root],            [AC_MSG_FILE([$1])],
  [VERSION],             [AC_MSG_FILE([$1])],
  
  [^.prefix$],           [AC_MSG_DIR([$1])],
  [[a-z]+dir$],          [AC_MSG_DIR([$1])],
  
  [.ac_cv_[a-z]*ext],    [AC_MSG_EXT([$1])],
  
  [LIBS],                [AC_MSG_ARGS([$1])],
  [FLAGS],               [AC_MSG_ARGS([$1])],  
  
  [build],               [AC_MSG_INFO([$1])],
  [host],                [AC_MSG_INFO([$1])],
  [^.host$],             [AC_MSG_INFO([$1])],
  [.ac_cv_target],       [AC_MSG_INFO([$1])],
  
  [cross_compiling$],    [AC_MSG_YESNO([$1])],
  [compiler_gnu],        [AC_MSG_YESNO([$1])],
  [gcc_traditional],     [AC_MSG_YESNO([$1])],
  [lib_fl_yywrap],       [AC_MSG_YESNO([$1])],
  [lex_yytext_pointer],  [AC_MSG_YESNO([$1])],
  [prog_cc_g],           [AC_MSG_YESNO([$1])],
  [.ac_[hH]eader],       [AC_MSG_YESNO([$1])],
  [.ac_cv_lib],          [AC_MSG_YESNO([$1])],
  [.ac_cv_func],         [AC_MSG_YESNO([$1])],
  [.ac_cv_c],            [AC_MSG_YESNO([$1])],
  [.ac_cv_type],         [AC_MSG_YESNO([$1])],
  [.ac_cv_header],       [AC_MSG_YESNO([$1])],
  [.ac_cv_working],      [AC_MSG_YESNO([$1])],
  [.ac_cv_os_cray],      [AC_MSG_YESNO([$1])],
  [.ac_cv_c_const],      [AC_MSG_YESNO([$1])],
  [as_ac_var],           [AC_MSG_YESNO([$1])],
  [ENABLE],              [AC_MSG_YESNO([$1])],
  [COLOR],               [AC_MSG_YESNO([$1])],
  [DEP],                 [AC_MSG_YESNO([$1])],
  [DEBUG],               [AC_MSG_YESNO([$1])],
  
  [.ac_ct],              [AC_MSG_PROG([$1])],
  [.ac_cv_prog],         [AC_MSG_PROG([$1])],
  [.CC],                 [AC_MSG_PROG([$1])],
  [.CPP],                [AC_MSG_PROG([$1])],
  [.AWK],                [AC_MSG_PROG([$1])],
  [.LEX],                [AC_MSG_PROG([$1])],
  [.YACC],               [AC_MSG_PROG([$1])],
  
  [AC_MSG_DEFAULT([$1])]
  )])

AC_DEFUN([AC_MSG_RESULT_UNQUOTED], [AC_MSG_RESULT([$1], [$2])])

# echo stuff using a color
# ---------------------------------------------------------------------------
AC_DEFUN([AC_ECHO_COLOR],
[echo -e ${COLOR_$2}$1${NC} >&AS_MESSAGE_FD])

# echo result using a color
# ---------------------------------------------------------------------------
AC_DEFUN([AC_MSG_COLOR],
[_AS_ECHO([$as_me:$LINENO: result: $1], AS_MESSAGE_LOG_FD) ; AC_ECHO_COLOR(m4_default([$1], ["<none>"]), $2)])

# define some shortcuts
# ---------------------------------------------------------------------------
AC_DEFUN([AC_MSG_YES],     [AC_MSG_COLOR([yes], [YES])])
AC_DEFUN([AC_MSG_NO],      [AC_MSG_COLOR([no],  [NO])])
AC_DEFUN([AC_MSG_NONE],    [AC_MSG_COLOR([$1],  [NONE])])
AC_DEFUN([AC_MSG_FILE],    [AC_MSG_COLOR([$1],  [FILE])])
AC_DEFUN([AC_MSG_EXT],     [AC_MSG_COLOR([$1],  [EXT])])
AC_DEFUN([AC_MSG_PROG],    [AC_MSG_COLOR([$1],  [PROG])])
AC_DEFUN([AC_MSG_LIB],     [AC_MSG_COLOR([$1],  [LIB])])
AC_DEFUN([AC_MSG_DIR],     [AC_MSG_COLOR([$1],  [DIR])])
AC_DEFUN([AC_MSG_VAR],     [AC_MSG_COLOR([$1],  [VAR])])
AC_DEFUN([AC_MSG_INFO],    [AC_MSG_COLOR([$1],  [INFO])])
AC_DEFUN([AC_MSG_ARGS],    [AC_MSG_COLOR([$1],  [ARGS])])
AC_DEFUN([AC_MSG_MK],      [AC_MSG_COLOR([$1],  [MK])])
AC_DEFUN([AC_MSG_DEFAULT], [AC_MSG_COLOR([$1],  [DEFAULT])])

# choose color at runtime
# ---------------------------------------------------------------------------
AC_DEFUN([AC_MSG_YESNO], 
[if test "$1" = "yes"; then
    AC_MSG_YES
  else
    AC_MSG_COLOR([$1], [NO])
  fi])

# set a theme
# ---------------------------------------------------------------------------
#
# NAME_YES                   configure script "yes" color
# NAME_NO                    configure script "yes" color
# NAME_NONE                  color for unspecified values
# NAME_FILE                  color for filenames
# NAME_DIR                   color for directory names
# NAME_EXT                   color for file extensions
# NAME_LIB                   color for libraries
# NAME_PROG                  color for program names
# NAME_INFO                  color for infos
# NAME_DEFAULT               default color (fallback)
#
AC_DEFUN([AC_SET_THEME],
[
  case "$1" in
    no)
      ;;
    mat*)
      # configure colors for theme "matrix"
      THEME_SPLASH="${LIGHT_GREEN}m${CYAN}a${GREEN}t${CLA}r${LIGHT_GREEN}i${CYAN}x${NC}"
      THEME_NAME="matrix"
      
      NAME_YES="LIGHT_GREEN"
      NAME_NO="CYAN"
      NAME_NONE="DARK_GRAY"
      NAME_FILE="GREEN"
      NAME_DIR="CYAN"
      NAME_VAR="YELLOW"
      NAME_EXT="LIGHT_GREEN"
      NAME_LIB="LIGHT_GREEN"
      NAME_PROG="WHITE"
      NAME_INFO="GREEN"
      NAME_ARGS="GREEN"
      NAME_MK="LIGHT_RED"
      NAME_DEFAULT="WHITE"
      ;;
    oce*)
      # configure colors for theme "oceanic"
      THEME_SPLASH="${BLUE}o${LIGHT_BLUE}c${CYAN}e${LIGHT_CYAN}a${LIGHT_GREEN}n${GREEN}i${BLUE}c${NC}"
      THEME_NAME="oceanic"
      
      NAME_YES="LIGHT_GREEN"
      NAME_NO="BLUE"
      NAME_NONE="BLUE"
      NAME_FILE="LIGHT_BLUE"
      NAME_DIR="LIGHT_BLUE"
      NAME_VAR="YELLOW"
      NAME_EXT="LIGHT_GREEN"
      NAME_LIB="LIGHT_GREEN"
      NAME_PROG="GREEN"
      NAME_INFO="LIGHT_CYAN"
      NAME_ARGS="YELLOW"
      NAME_MK="LIGHT_RED"
      NAME_DEFAULT="BLUE"
      ;;
    hell*)
      # configure colors for theme "hellfire"
      THEME_SPLASH="${RED}h${LIGHT_RED}e${BROWN}l${YELLOW}l${RED}f${LIGHT_RED}i${BROWN}r${YELLOW}e${NC}"
      THEME_NAME="hellfire"
      
      NAME_YES="YELLOW"
      NAME_NO="LIGHT_RED"
      NAME_NONE="RED"
      NAME_FILE="BROWN"
      NAME_DIR="LIGHT_BLUE"
      NAME_VAR="YELLOW"
      NAME_EXT="YELLOW"
      NAME_LIB="YELLOW"
      NAME_PROG="YELLOW"
      NAME_INFO="LIGHT_RED"
      NAME_ARGS="YELLOW"
      NAME_MK="LIGHT_RED"
      NAME_DEFAULT="RED"
      ;;
    *)
      # configure colors for theme "chaos"
      THEME_SPLASH="${RED}c${LIGHT_RED}h${YELLOW}a${GREEN}o${LIGHT_BLUE}s${NC}"
      THEME_NAME="chaos"
      
      NAME_YES="GREEN"
      NAME_NO="RED"
      NAME_NONE="DARK_GRAY"
      NAME_FILE="LIGHT_CYAN"
      NAME_DIR="LIGHT_BLUE"
      NAME_VAR="YELLOW"
      NAME_EXT="LIGHT_CYAN"
      NAME_LIB="LIGHT_GREEN"
      NAME_PROG="YELLOW"
      NAME_INFO="YELLOW"
      NAME_ARGS="LIGHT_CYAN"
      NAME_MK="LIGHT_RED"
      NAME_DEFAULT="LIGHT_BLUE"
      ;;
  esac
  
  # evaluate escape codes
  m4_define([AC_COLOR_EVAL], [])dnl
  m4_foreach([AC_COLOR_TYPE], [AC_COLOR_TYPES], 
  [m4_append([AC_COLOR_EVAL], [  eval COLOR_])dnl
   m4_append([AC_COLOR_EVAL], AC_COLOR_TYPE)dnl
   m4_append([AC_COLOR_EVAL], [="\$$NAME_])dnl
   m4_append([AC_COLOR_EVAL], AC_COLOR_TYPE)dnl
   m4_append([AC_COLOR_EVAL], ["
])dnl
   m4_append([AC_COLOR_EVAL], [AC_SUBST(NAME_])dnl
   m4_append([AC_COLOR_EVAL], AC_COLOR_TYPE)dnl
   m4_append([AC_COLOR_EVAL], [)
])])dnl
  AC_COLOR_EVAL dnl
  AC_SUBST(THEME_NAME)dnl
  AC_SUBST(THEME_SPLASH)dnl
])

# check for --{enable,disable}-color
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_COLOR], 
[# set color variables
AC_MSG_CHECKING(whether to enable colored compiling)

ac_cv_quiet="yes"
AC_ARG_ENABLE(quiet,
[  --disable-quiet         echo commands while compiling (automake style)
  --enable-quiet          do not echo commands while compiling, print 
                          variable-like name of program used (linux style)],
[case "$enableval" in
  no)
    ac_cv_quiet="no"
    ;;
  *)
    ac_cv_quiet="yes"
    ;;
esac])
if test "$ac_cv_quiet" = "yes"; then
#  MAKEFLAGS=""
  MAKEFLAGS="-s"
  QUIET="--quiet"
  ECHO="echo"
  REDIR=">/dev/null"
  REDIR2="2>/dev/null"
  QUIET="@"
  SILENT=""
  NOSILENT="#"
else
  MAKEFLAGS=""
  QUIET=""
  ECHO="true"
  REDIR=""
  REDIR2=""
  QUIET=""
  SILENT="#"
  NOSILENT=""
fi
AC_SUBST(ECHO)
AC_SUBST(QUIET)
AC_SUBST(MAKEFLAGS)
AC_SUBST(REDIR)
AC_SUBST(REDIR2)
AC_SUBST(QUIET)
AC_SUBST(SILENT)
AC_SUBST(NOSILENT)
COLOR="no"
AC_ARG_ENABLE(color,
[  --disable-color         black and white compiling
  --enable-color[[=THEME]]  colored compiling [(default)]
  
  Available Themes: chaos [(default)], matrix, oceanic, hell
],
[THEME="$enableval"
case "$enableval" in
  yes|mat*|oce*|hell*|chaos)
    COLOR="yes"
    ;;
  no)
    COLOR="no"
    ;;
esac], [THEME="$enableval"])

if test "$COLOR" = "yes"
then
  DARK_GRAY="###ESCAPE###1;30m"       # dark gray
  LIGHT_BLUE="###ESCAPE###1;34m"      # light blue
  LIGHT_GREEN="###ESCAPE###1;32m"     # light green
  LIGHT_CYAN="###ESCAPE###1;36m"      # light cyan
  LIGHT_RED="###ESCAPE###1;31m"       # light red
  LIGHT_PURPLE="###ESCAPE###1;35m"    # light purple
  YELLOW="###ESCAPE###1;33m"          # yellow
  WHITE="###ESCAPE###1;37m"           # white
  
  BLACK="###ESCAPE###0;30m"           # black
  BLUE="###ESCAPE###0;34m"            # blue
  GREEN="###ESCAPE###0;32m"           # green
  CYAN="###ESCAPE###0;36m"            # cyan
  RED="###ESCAPE###0;31m"             # red
  PURPLE="###ESCAPE###0;35m"          # purple
  BROWN="###ESCAPE###0;33m"           # brown
  GRAY="###ESCAPE###0;37m"            # gray
  
  NC="###ESCAPE###0m"

  AC_SET_THEME($THEME)
  AC_MSG_RESULT([yes])
  AC_MSG_CHECKING([for the chosen theme])
  AC_MSG_RESULT($THEME_SPLASH)
else
  AC_MSG_RESULT([yes])
fi    
m4_foreach([AC_COLOR_NAME], [AC_COLOR_NAMES], [AC_SUBST(AC_COLOR_NAME)])
AC_SUBST(NC)
AC_SUBST(COLOR)])


# $Id: hack.m4,v 1.12 2005/05/03 13:04:01 smoli Exp $
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
    ac_config_status_args="$ac_config_status_args --quiet"
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
m4_define([_AC_OUTPUT_FILES],
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
  configure_input=$configure_input"Generated from `echo $ac_file_in |
             sed 's,.*/,,'` by configure."

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
      eval $ac_sub_configure $ac_sub_configure_args \
     --cache-file=$ac_sub_cache_file --srcdir=$ac_srcdir ||
  AC_MSG_ERROR([$ac_sub_configure failed for $ac_dir])
    fi
    cd $ac_popdir
  done
fi
])# _AC_OUTPUT_SUBDIRS


# AC_PROG_DOXYGEN
# --------------
AN_MAKEVAR([DOXYGEN_NOARG], [AC_PROG_DOXYGEN])
AN_PROGRAM([doxygen], [AC_PROG_DOXYGEN])
AC_DEFUN([AC_PROG_DOXYGEN],
[AC_CHECK_TOOL(DOXYGEN_NOARG, [doxygen], [:])
 DOXYGEN="$DOXYGEN_NOARG"
 
 if test "$DOXYGEN" = ":"; then
   BUILD_DOC="#"
 else
   NO_BUILD_DOC="#"
 fi

 AC_SUBST([BUILD_DOC])
 AC_SUBST([NO_BUILD_DOC])
 AC_SUBST(DOXYGEN)])

# AC_PROG_LN
# --------------
AN_MAKEVAR([LN_NOARG], [AC_PROG_LN])
AN_PROGRAM([ln], [AC_PROG_LN])
AC_DEFUN([AC_PROG_LN],
[AC_CHECK_TOOL(LN_NOARG, ln, :)
 LN="$LN_NOARG"
 AC_SUBST(LN)])

# AC_PROG_AR
# --------------
AN_MAKEVAR([AR_NOARG], [AC_PROG_AR])
AN_PROGRAM([ar], [AC_PROG_AR])
AC_DEFUN([AC_PROG_AR],
[AC_CHECK_TOOL(AR_NOARG, ar, :)
 AR="$AR_NOARG cr"
 AC_SUBST(AR)])

# AC_PROG_INSTALL
# --------------
AN_MAKEVAR([INSTALL], [AC_PROG_INSTALL])
AN_PROGRAM([install], [AC_PROG_INSTALL])
AC_DEFUN([AC_PROG_INSTALL],
[AC_CHECK_TOOL(INSTALL, install, :)
AC_SUBST([INSTALL])])

# AC_PROG_DLLWRAP
# --------------
AN_MAKEVAR([DLLWRAP], [AC_PROG_DLLWRAP])
AN_PROGRAM([dllwrap], [AC_PROG_DLLWRAP])
AC_DEFUN([AC_PROG_DLLWRAP],
[AC_CHECK_TOOL(DLLWRAP, dllwrap, :)
AC_SUBST([DLLWRAP])])


# $Id: diet.m4,v 1.14 2005/04/15 06:24:08 smoli Exp $
# ===========================================================================
#
# dietlibc compiling support
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_DIETLIBC],
[DIETLIBC="no"
DIETDIRS="m4_default([$1], [/opt/diet /usr/diet /usr/local/diet])"
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
           DIETDIR="$dir"
           break
         fi
       fi
     done
   else
     DIETVERSION="`$dir/bin/diet 2>&1 | sed 's,[[a-z ]]*-,,;;s,^.*[[:-]].*$,,'`"
   fi
   
   if test -z "$DIETVERSION"
   then
     DIET=""
     AC_MSG_RESULT([no])
   else
     DIET=""
     
     LIBDIR="$DIETDIR/lib"
     
     for x in $DIETLIBS
     do     
       DIETLIB="`ls -d $x/lib-* $x/lib* 2>/dev/null | head -n1`"
       
       if test -f "$DIETLIB/start.o"
       then 
         LIBDIR="$DIETLIB"
       fi
     done
     
     CPPFLAGS="$CPPFLAGS -nostdinc -D__dietlibc__ -idirafter $DIETDIR/include"
     CFLAGS="$CFLAGS"
     LDFLAGS="$LDFLAGS -nostdlib -static -L$LIBDIR $LIBDIR/start.o"
     LIBS="$LIBS -lc -lgcc"
     AC_MSG_RESULT([$DIETVERSION])
   fi
 fi
 
dnl  AC_MSG_CHECKING([wheter compiler supports -falign])
dnl  saved_CFLAGS="$CFLAGS"
dnl  CFLAGS="$CFLAGS -mpreferred-stack-boundary=4 -falign-functions=4 -falign-jumps=1 -falign-loops=1"
dnl  AC_TRY_COMPILE([], [], [
dnl  AC_MSG_RESULT([yes])], [
dnl  AC_MSG_RESULT([no])
dnl  CFLAGS="$saved_CFLAGS"])
 
AC_SUBST(DIETLIBC)
AC_SUBST(DIET)])

# $Id: funcs.m4,v 1.14 2005/04/15 06:34:51 smoli Exp $
# ===========================================================================
#
# Macros checking for system depedencies.
# Do so without preprocessor tests, because the preprocessor is almost
# never invoked separately
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for PostgreSQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_PSQL],
[default_directory="/usr /usr/local /usr/pgsql /usr/local/pgsql"

AC_ARG_WITH(pgsql,
    [  --with-pgsql=DIR        support for PostgreSQL],
    [ with_pgsql="$withval" ],
    [ with_pgsql=no ])

if test "$with_pgsql" != "no"; then
  if test "$with_pgsql" = "yes"; then
    pgsql_directory="$default_directory "
    pgsql_fail="yes"
  elif test -d $withval; then
    pgsql_directory="$withval $default_directory"
    pgsql_fail="yes"
  elif test "$with_pgsql" = ""; then
    pgsql_directory="$default_directory"
    pgsql_fail="no"
  fi

  AC_MSG_CHECKING(for PostgreSQL headers)

  for i in $pgsql_directory; do
    if test -r $i/include/pgsql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/pgsql
    elif test -r $i/include/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include
    elif test -r $i/include/postgresql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/postgresql
    fi
  done

  if test ! -d "$PGSQL_INC_DIR"; then
    AC_MSG_ERROR("PostgreSQL header file (libpq-fe.h)", "$PGSQL_DIR $PGSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for PostgreSQL client library)

  for i in lib lib/pgsql; do
    str="$PGSQL_DIR/$i/libpq.*"
    for j in `echo $str`; do
      if test -r $j; then
          PGSQL_LIB_DIR="$PGSQL_DIR/$i"
          break 2
      fi
    done
  done

    if test -z "$PGSQL_LIB_DIR"; then
      if test "$postgresql_fail" != "no"; then
        AC_MSG_ERROR("PostgreSQL library libpq",
        "$PGSQL_DIR/lib $PGSQL_DIR/lib/pgsql")
      else
        AC_MSG_RESULT(no);
      fi
    else
      AC_MSG_RESULT($PGSQL_LIB_DIR)
      PSQL_LIBS="-L${PGSQL_LIB_DIR} -lpq"
      PSQL_CFLAGS="-I${PGSQL_INC_DIR}"
      DBSUPPORT="$DBSUPPORT PostgreSQL"
      AC_DEFINE_UNQUOTED(HAVE_PGSQL, "1", [Define this if you have PostgreSQL])
    fi

fi
AC_SUBST(PSQL_LIBS)
AC_SUBST(PSQL_CFLAGS)])

# check for MySQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_MYSQL],
[default_directory="/usr /usr/local /usr/mysql /opt/mysql"

with_mysql="yes"
AC_ARG_WITH(mysql,
    [  --with-mysql=DIR        support for MySQL],
    [ with_mysql="$withval" ],
    [ with_mysql=no ])

if test "$with_mysql" != "no"; then
  if test "$with_mysql" = "yes"; then
    mysql_directory="$default_directory";
    mysql_fail="yes"
  elif test -d $withval; then
    mysql_directory="$withval"
    mysql_fail="no"
  elif test "$with_mysql" = ""; then
    mysql_directory="$default_directory";
    mysql_fail="no"
  fi

  AC_MSG_CHECKING(for MySQL headers)

  for i in $mysql_directory; do
    if test -r $i/include/mysql/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include/mysql
    elif test -r $i/include/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include
    fi
  done

  if test ! -d "$MYSQL_INC_DIR"; then
    AC_MSG_RESULT(not found)
    with_mysql="no"
#    FAIL_MESSAGE("MySQL Headers", "$MYSQL_DIR $MYSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for MySQL client library)

  for i in lib lib/mysql; do
    str="$MYSQL_DIR/$i/libmysqlclient.*"
    for j in `echo $str`; do
      if test -r $j; then
        MYSQL_LIB_DIR="$MYSQL_DIR/$i"
        break 2
      fi
    done
  done

  if test -z "$MYSQL_LIB_DIR"; then
    if test "$mysql_fail" != "no"; then
      AC_MSG_RESULT(not found)
      with_mysql="no"
#      FAIL_MESSAGE("mysqlclient library",
#                   "$MYSQL_LIB_DIR")
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT($MYSQL_LIB_DIR)
    MYSQL_LIBS="-L${MYSQL_LIB_DIR} -Wl,-rpath,${MYSQL_LIB_DIR} -lmysqlclient"
    MYSQL_CFLAGS="-I${MYSQL_INC_DIR}"
    AC_CHECK_LIB(z, compress)
    DBSUPPORT="$DBSUPPORT MySQL"
    AC_DEFINE_UNQUOTED(HAVE_MYSQL, "1", [Define this if you have MySQL])
  fi
fi

AC_SUBST([MYSQL_LIBS])
AC_SUBST([MYSQL_CFLAGS])
])

# check for libsgui
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SGUI],
[if test -d "$srcdir/libsgui"
then
  libsgui="shipped"
else
  libsgui="installed"
fi
AC_ARG_WITH(libsgui,
[  --with-libsgui=installed          (default)
   --with-libsgui=shipped],
 [
  case "$withval" in
    installed)
      libsgui="installed"
      ;;
    shipped)
      libsgui="shipped"
      ;;
    no)
      libsgui="shipped"
      ;;
    yes)
      libsgui="installed"
      ;;
  esac])

if test "$libsgui" = "installed"
then
  AM_PATH_SGUI([2.0.0], libsgui="installed", libsgui="shipped")
fi

if test "$libsgui" = "shipped"
then
  if test -d "$srcdir/libsgui"
  then
    AC_CONFIG_SUBDIRS([libsgui])
    SGUI_CFLAGS="-I\$(top_srcdir)/libsgui/include -I\$(top_builddir)/libsgui/include"
    SGUI_LIBS="\$(top_builddir)/libsgui/src/libsgui.a"
    SGUI="libsgui"
  else
    AC_MSG_ERROR([No shipped and no installed libsgui!])
  fi
fi
AC_SUBST([SGUI_CFLAGS])
AC_SUBST([SGUI_LIBS])
AC_SUBST([SGUI])
 ])


# check for libchaos
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_CHAOS],
[if test -d $srcdir/libchaos
then
  libchaos="shipped"
else
  libchaos="installed"
fi

AC_ARG_WITH(libchaos,
[  --with-libchaos=installed          [(default)]
  --with-libchaos=shipped],
[
  case "$withval" in
  installed)
    libchaos="installed"
    ;;
  shipped)
    libchaos="shipped"
    ;;
  no)
    libchaos="shipped"
    ;;
  yes)
    libchaos="installed"
    ;;
  esac
])

if test $libchaos = "installed"
then
  AM_PATH_CHAOS([2.1.0], libchaos="installed", libchaos="shipped")
fi

if test $libchaos = "shipped"
then
  if test -d $srcdir/libchaos
  then
    AC_CONFIG_SUBDIRS(libchaos)
    SUBDIRS="libchaos $SUBDIRS"
    CHAOS_CFLAGS="-I\$(top_srcdir)/libchaos/include -I\$(top_builddir)/libchaos/include"
    CHAOS_LIBS="../libchaos/src/libchaos.a -ldl -lm"
  else
    AC_MSG_ERROR([No shipped and no installed libchaos!])
  fi
fi
AC_SUBST([CHAOS_CFLAGS])
AC_SUBST([CHAOS_LIBS])
])

# check for Freetype 2
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_FT2],
[dnl Check for the FreeType 2 library
dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
HAVE_FT2="no"
AC_ARG_WITH(ft2,[  --with-ft2[[=prefix]]      Support for FreeType 2 fonts],
            ft2_prefix="$withval", ft2_prefix="")

if test x$ft2_exec_prefix != x ; then
     ft2_args="$ft2_args --exec-prefix=$ft2_exec_prefix"
     if test x${FREETYPE_CONFIG+set} != xset ; then
        FREETYPE_CONFIG=$ft2_exec_prefix/bin/freetype-config
     fi
fi
if test x$ft2_prefix != x ; then
     ft2_args="$ft2_args --prefix=$ft2_prefix"
     if test x${FREETYPE_CONFIG+set} != xset ; then
        FREETYPE_CONFIG=$ft2_prefix/bin/freetype-config
     fi
     AC_PATH_PROG(FREETYPE_CONFIG, freetype-config, no)
     no_ft2=""
     if test "$FREETYPE_CONFIG" = "no" ; then
       AC_MSG_ERROR([
       *** Unable to find FreeType2 library (http://www.freetype.org/)
       ])
     else
       FT2_CFLAGS="`$FREETYPE_CONFIG $ft2conf_args --cflags`"
       FT2_LIBS="`$FREETYPE_CONFIG $ft2conf_args --libs`"

      TTFSUPPORT="ft2"
      AC_DEFINE_UNQUOTED(HAVE_FT2, "1", [Define this if you have freetype 2])
      HAVE_FT2="yes"
     fi
fi

AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)])
# check for OpenSSL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SSL],
[AC_MSG_CHECKING(whether to compile with SSL support)
ac_cv_ssl="auto"
AC_ARG_WITH(ssl,
[  --with-ssl[[=yes|no|auto]]   OpenSSL support [[auto]]],
[
  case "$enableval" in
    y*) ac_cv_ssl="yes" ;;
    n*) ac_cv_ssl="no" ;;
    *) ac_cv_ssl="auto" ;;
  esac
])
AC_MSG_RESULT($ac_cv_ssl)

SSL_LIBS=""
SSL_CFLAGS=""
OPENSSL=""
if test "$ac_cv_ssl" = "yes" || test "$ac_cv_ssl" = "auto"
then
  saved_libs="$LIBS"
  AC_CHECK_LIB(crypto, ERR_load_crypto_strings)

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find libcrypto, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "yes"
  then
    SSL_LIBS="-lcrypto"
  fi

  LIBS="$SSL_LIBS $saved_libs"
  AC_CHECK_LIB(ssl, SSL_load_error_strings)

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find libssl, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = "yes"
  then
    SSL_LIBS="-lssl $SSL_LIBS"
  fi

  LIBS="$saved_libs"
  AC_CHECK_HEADERS(openssl/opensslv.h)

  if test "$ac_cv_header_openssl_opensslv_h" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find openssl/opensslv.h, install openssl >= 0.9.7)
    exit 1
  fi

  AC_MSG_CHECKING(for the OpenSSL UI)

  OPENSSL=`which openssl 2>/dev/null`

  if test "x$OPENSSL" = "x"
  then
    if test -f /usr/bin/openssl
    then
      OPENSSL=/usr/bin/openssl
      AC_MSG_RESULT($OPENSSL)
    else
      AC_MSG_RESULT(not found)
    fi
  else
    AC_MSG_RESULT($OPENSSL)
  fi

  if test "x$OPENSSL" = "x" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find OpenSSL command line tool, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "yes" && test "$ac_cv_lib_ssl_SSL_load_error_strings" = "yes" && test "$ac_cv_header_openssl_opensslv_h" = "yes"
  then
    HAVE_SSL=yes
    AC_DEFINE_UNQUOTED(HAVE_SSL, 1, [Define this if you have OpenSSL])
  else
    HAVE_SSL=no
    SSL_LIBS=""
    SSL_CFLAGS=""
    OPENSSL=""
  fi
fi

AC_SUBST(SSL_LIBS)
AC_SUBST(SSL_CFLAGS)
])                        

# check for libowfat
# ------------------------------------------------------------------
AC_DEFUN([AC_LOWFAT],
[AC_MSG_CHECKING([for libofatipv6 support])
], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])])

# check for termios
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_TERMIOS],
[AC_SYS_POSIX_TERMIOS
if test "$ac_cv_sys_posix_termios" = "yes"; then
  AC_DEFINE_UNQUOTED([HAVE_TERMIOS], 1, 
  [Define this if you have the POSIX termios library])
fi
])

# check for if_indextoname
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_N2I],
[AC_MSG_CHECKING([for if_indextoname])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main() {
  static char ifname[IFNAMSIZ];
  char *tmp=if_indextoname(0,ifname);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_N2I], 1,
[Define this if you have the if_indextoname() call])], [AC_MSG_RESULT([no])])])

# check for IPv6 scope ids
# ------------------------------------------------------------------
AC_DEFUN([AC_SCOPE_ID],
[AC_MSG_CHECKING([for ipv6 scope ids])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
  sa.sin6_scope_id = 23;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SCOPE_ID], 1,
[Define this if your libc supports ipv6 scope ids])], [AC_MSG_RESULT([no])])])


# check for IPv6 support
# ------------------------------------------------------------------
AC_DEFUN([AC_IPV6],
[AC_MSG_CHECKING([for ipv6 support])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_IPV6], 1,
[Define this if your libc supports ipv6])
AC_DEFINE_UNQUOTED([LIBC_HAS_IP6], 1,
[Define this if your libc supports ipv6])], [AC_MSG_RESULT([no])])])


# check for alloca() function and header
# ------------------------------------------------------------------
AN_HEADER([alloca.h], [AC_FUNC_ALLOCA])
AC_DEFUN([AC_FUNC_ALLOCA],
[AC_MSG_CHECKING([for alloca])
AC_COMPILE_IFELSE([
#include <stdlib.h>
#include <alloca.h>
dnl
main() {
  char* c=alloca(23);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_ALLOCA], 1,
[Define this if your compiler supports alloca()])], [AC_MSG_RESULT([no])])])


# check for epoll() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_EPOLL],
[AC_MSG_CHECKING([for epoll])
AC_COMPILE_IFELSE([
  int efd=epoll_create(10);
  struct epoll_event x;
  if (efd==-1) return 111;
  x.events=EPOLLIN;
  x.data.fd=0;
  if (epoll_ctl(efd,EPOLL_CTL_ADD,0 /* fd */,&x)==-1) return 111;
  {
    int i,n;
    struct epoll_event y[100];
    if ((n=epoll_wait(efd,y,100,1000))==-1) return 111;
    if (n>0)
      printf("event %d on fd #%d\n",y[0].events,y[0].data.fd);
  }
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_EPOLL], 1,
[Define this if your OS supports epoll()])], [AC_MSG_RESULT([no])])])

# check for select() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SELECT],
[AC_MSG_CHECKING([for select])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/time.h>

#include <string.h>     /* BSD braindeadness */

#include <sys/select.h> /* SVR4 silliness */

int main()
{
  fd_set x;
  int ret;
  struct timeval tv;
  tv.tv_sec = 2; tv.tv_usec = 0;
  FD_ZERO(&x); FD_SET(22, &x);
  ret = select(23, &x, &x, NULL, &tv);
  if(FD_ISSET(22, &x)) FD_CLR(22, &x);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SELECT], 1,
[Define this if your OS supports select()])], [$2
AC_MSG_RESULT([no])])])

# check for poll() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_POLL],
[AC_MSG_CHECKING([for poll])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>

int main()
{
  struct pollfd x;

  x.fd = open("trypoll.c",O_RDONLY);
  if (x.fd == -1) _exit(111);
  x.events = POLLIN;
  if (poll(&x,1,10) == -1) _exit(1);
  if (x.revents != POLLIN) _exit(1);

  /* XXX: try to detect and avoid poll() imitation libraries */

  _exit(0);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_POLL], 1,
[Define this if your OS supports poll()])], [$2
AC_MSG_RESULT([no])])])

# check for kqueue functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_KQUEUE],
[AC_MSG_CHECKING([for kqueue])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main() {
  int kq=kqueue();
  struct kevent kev;
  struct timespec ts;
  if (kq==-1) return 111;
  EV_SET(&kev, 0 /* fd */, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, 0);
  ts.tv_sec=0; ts.tv_nsec=0;
  if (kevent(kq,&kev,1,0,0,&ts)==-1) return 111;

  {
    struct kevent events[100];
    int i,n;
    ts.tv_sec=1; ts.tv_nsec=0;
    switch (n=kevent(kq,0,0,events,100,&ts)) {
    case -1: return 111;
    case 0: puts("no data on fd #0"); break;
    }
    for (i=0; i<n; ++i) {
      printf("ident %d, filter %d, flags %d, fflags %d, data %d\n",
             events[i].ident,events[i].filter,events[i].flags,
             events[i].fflags,events[i].data);
    }
  }
  return 0;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_KQUEUE], 1,
[Define this if your OS supports kqueues])], [AC_MSG_RESULT([no])])])

# check for sigio functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SIGIO],
[AC_MSG_CHECKING([for sigio])
AC_COMPILE_IFELSE([
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/poll.h>
#include <signal.h>
#include <fcntl.h>

int main() {
  int signum=SIGRTMIN+1;
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss,signum);
  sigaddset(&ss,SIGIO);
  sigprocmask(SIG_BLOCK,&ss,0);

  fcntl(0 /* fd */,F_SETOWN,getpid());
  fcntl(0 /* fd */,F_SETSIG,signum);
#if defined(O_ONESIGFD) && defined(F_SETAUXFL)
  fcntl(0 /* fd */, F_SETAUXFL, O_ONESIGFD);
#endif
  fcntl(0 /* fd */,F_SETFL,fcntl(0 /* fd */,F_GETFL)|O_NONBLOCK|O_ASYNC);

  {
    siginfo_t info;
    struct timespec timeout;
    int r;
    timeout.tv_sec=1; timeout.tv_nsec=0;
    switch ((r=sigtimedwait(&ss,&info,&timeout))) {
    case SIGIO:
      /* signal queue overflow */
      signal(signum,SIG_DFL);
      /* do poll */
      break;
    default:
      if (r==signum) {
  printf("event %c%c on fd #%d\n",info.si_band&POLLIN?'r':'-',info.si_band&POLLOUT?'w':'-',info.si_fd);
      }
    }
  }
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SIGIO], 1,
[Define this if your OS supports I/O signales])], [AC_MSG_RESULT([no])])])

# check for /dev/poll i/o multiplexing 
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_DEVPOLL],
[AC_MSG_CHECKING([for /dev/poll])
AC_COMPILE_IFELSE([
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/devpoll.h>

main() {
  int fd=open("/dev/poll",O_RDWR);
  struct pollfd p[100];
  int i,r;
  dvpoll_t timeout;
  p[0].fd=0;
  p[0].events=POLLIN;
  write(fd,p,sizeof(struct pollfd));
  timeout.dp_timeout=100;       /* milliseconds? */
  timeout.dp_nfds=1;
  timeout.dp_fds=p;
  r=ioctl(fd,DP_POLL,&timeout);
  for (i=0; i<r; ++i)
    printf("event %d on fd #%d\n",p[i].revents,p[i].fd);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_DEVPOLL], 1, 
[Define this if your OS supports /dev/poll])], [AC_MSG_RESULT([no])])])

# check for sendfile() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SENDFILE], 
[m4_define([AC_HAVE_SENDFILE], [dnl
AC_DEFINE_UNQUOTED([HAVE_SENDFILE], 1, [Define this if you have the sendfile() system call])])
AC_MSG_CHECKING([for BSD sendfile])
AC_COMPILE_IFELSE([
/* for macos X, dont ask */
#define SENDFILE 1

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int main() {
  struct sf_hdtr hdr;
  struct iovec v[17+23];
  int r,fd=1;
  off_t sbytes;
  hdr.headers=v; hdr.hdr_cnt=17;
  hdr.trailers=v+17; hdr.trl_cnt=23;
  r=sendfile(0,1,37,42,&hdr,&sbytes,0);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_BSDSENDFILE], 1, [Define this if you a BSD style sendfile()])],
[AC_MSG_RESULT([luckyl not])
AC_MSG_CHECKING([for sendfile])
AC_COMPILE_IFELSE([
#ifdef __hpux__
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

int main() {
/*
      sbsize_t sendfile(int s, int fd, off_t offset, bsize_t nbytes,
                    const struct iovec *hdtrl, int flags);
*/
  struct iovec x[2];
  int fd=open("configure",0);
  x[0].iov_base="header";
  x[0].iov_len=6;
  x[1].iov_base="footer";
  x[1].iov_len=6;
  sendfile(1 /* dest socket */,fd /* src file */,
           0 /* offset */, 23 /* nbytes */,
           x, 0);
  perror("sendfile");
  return 0;
}
#elif defined (__sun__) && defined(__svr4__)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <stdio.h>

int main() {
  off_t o;
  o=0;
  sendfile(1 /* dest */, 0 /* src */,&o,23 /* nbytes */);
  perror("sendfile");
  return 0;
}
#elif defined (_AIX)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  struct sf_parms p;
  int destfd=1;
  p.header_data="header";
  p.header_length=6;
  p.file_descriptor=fd;
  p.file_offset=0;
  p.file_bytes=23;
  p.trailer_data="footer";
  p.trailer_length=6;
  if (send_file(&destfd,&p,0)>=0)
    printf("sent %lu bytes.\n",p.bytes_sent);
}
#elif defined(__linux__)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <unistd.h>
#if defined(__GLIBC__)
#include <sys/sendfile.h>
#elif defined(__dietlibc__)
#include <sys/sendfile.h>
#else
#include <linux/unistd.h>
_syscall4(int,sendfile,int,out,int,in,long *,offset,unsigned long,count)
#endif
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  off_t o=0;
  off_t r=sendfile(1,fd,&o,23);
  if (r!=-1)
    printf("sent %llu bytes.\n",r);
}
#endif
], [AC_HAVE_SENDFILE
AC_MSG_RESULT([yes])
])], [AC_MSG_RESULT([no])])])



# stolen from Sam Lantinga 9/21/99
# which has stolen it from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SGUI([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libsgui, and define SGUI_CFLAGS and SGUI_LIBS
dnl
AC_DEFUN([AM_PATH_SGUI],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sgui-prefix,[  --with-sgui-prefix=PFX   Prefix where libsgui is installed (optional)],
            sgui_prefix="$withval", sgui_prefix="")
AC_ARG_WITH(sgui-exec-prefix,[  --with-sgui-exec-prefix=PFX Exec prefix where libsgui is installed (optional)],
            sgui_exec_prefix="$withval", sgui_exec_prefix="")
AC_ARG_ENABLE(sguitest, [  --disable-sguitest       Do not try to compile and run a test sgui program],
		    , enable_sguitest=yes)

  if test x$sgui_exec_prefix != x ; then
     sgui_args="$sgui_args --exec-prefix=$sgui_exec_prefix"
     if test x${SGUI_CONFIG+set} != xset ; then
        SGUI_CONFIG=$sgui_exec_prefix/bin/libsgui-config
     fi
  fi
  if test x$sgui_prefix != x ; then
     sgui_args="$sgui_args --prefix=$sgui_prefix"
     if test x${SGUI_CONFIG+set} != xset ; then
        SGUI_CONFIG=$sgui_prefix/bin/libsgui-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(SGUI_CONFIG, libsgui-config, no, [$PATH])
  min_sgui_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for libsgui - version >= $min_sgui_version)
  no_sgui=""
  if test "$SGUI_CONFIG" = "no" ; then
    no_sgui=yes
  else
    SGUI_CFLAGS=`$SGUI_CONFIG $sguiconf_args --cflags`
    SGUI_LIBS=`$SGUI_CONFIG $sguiconf_args --libs`

    sgui_major_version=`$SGUI_CONFIG $sgui_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sgui_minor_version=`$SGUI_CONFIG $sgui_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sgui_micro_version=`$SGUI_CONFIG $SGUI_CONFIG_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sguitest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SGUI_CFLAGS"
      LIBS="$LIBS $SGUI_LIBS"
dnl
dnl Now check if the installed sgui is sufficiently new. (Also sanity
dnl checks the results of libsgui-config to some extent
dnl
      rm -f conf.sguitest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libsgui/io.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sguitest");
  */
  { FILE *fp = fopen("conf.sguitest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sgui_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sgui_version");
     exit(1);
   }

   if (($sgui_major_version > major) ||
      (($sgui_major_version == major) && ($sgui_minor_version > minor)) ||
      (($sgui_major_version == major) && ($sgui_minor_version == minor) && ($sgui_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'libsgui-config --version' returned %d.%d.%d, but the minimum version\n", $sgui_major_version, $sgui_minor_version, $sgui_micro_version);
      printf("*** of sgui required is %d.%d.%d. If libsgui-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If libsgui-config was wrong, set the environment variable SGUI_CONFIG\n");
      printf("*** to point to the correct copy of libsgui-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sgui=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sgui" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SGUI_CONFIG" = "no" ; then
       echo "*** The libsgui-config script installed by libsgui could not be found"
       echo "*** If libsgui was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SGUI_CONFIG environment variable to the"
       echo "*** full path to libsgui-config."
     else
       if test -f conf.sguitest ; then
        :
       else
          echo "*** Could not run sgui test program, checking why..."
          CFLAGS="$CFLAGS $SGUI_CFLAGS"
          LIBS="$LIBS $SGUI_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "sgui.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding sgui or finding the wrong"
          echo "*** version of sgui. If it is not finding sgui, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means sgui was incorrectly installed"
          echo "*** or that you have moved sgui since it was installed. In the latter case, you"
          echo "*** may want to edit the libsgui-config script: $SGUI_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SGUI_CFLAGS=""
     SGUI_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SGUI_CFLAGS)
  AC_SUBST(SGUI_LIBS)
  rm -f conf.sguitest
])

# stolen from Sam Lantinga 9/21/99
# which has stolen it from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_CHAOS([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libchaos, and define CHAOS_CFLAGS and CHAOS_LIBS
dnl
AC_DEFUN([AM_PATH_CHAOS],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(chaos-prefix,[  --with-chaos-prefix=PFX   Prefix where libchaos is installed (optional)],
            chaos_prefix="$withval", chaos_prefix="")
AC_ARG_WITH(chaos-exec-prefix,[  --with-chaos-exec-prefix=PFX Exec prefix where libchaos is installed (optional)],
            chaos_exec_prefix="$withval", chaos_exec_prefix="")
AC_ARG_ENABLE(chaostest, [  --disable-chaostest       Do not try to compile and run a test chaos program],
		    , enable_chaostest=yes)

  if test x$chaos_exec_prefix != x ; then
     chaos_args="$chaos_args --exec-prefix=$chaos_exec_prefix"
     if test x${chaos_CONFIG+set} != xset ; then
        chaos_CONFIG=$chaos_exec_prefix/bin/libchaos-config
     fi
  fi
  if test x$chaos_prefix != x ; then
     chaos_args="$chaos_args --prefix=$chaos_prefix"
     if test x${chaos_CONFIG+set} != xset ; then
        chaos_CONFIG=$chaos_prefix/bin/libchaos-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(chaos_CONFIG, libchaos-config, no, [$PATH])
  min_chaos_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for libchaos - version >= $min_chaos_version)
  no_chaos=""
  if test "$chaos_CONFIG" = "no" ; then
    no_chaos=yes
  else
    CHAOS_CFLAGS=`$chaos_CONFIG $chaosconf_args --cflags`
    CHAOS_LIBS=`$chaos_CONFIG $chaosconf_args --libs`

    chaos_major_version=`$chaos_CONFIG $chaos_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    chaos_minor_version=`$chaos_CONFIG $chaos_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    chaos_micro_version=`$chaos_CONFIG $chaos_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_chaostest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $CHAOS_CFLAGS"
      LIBS="$LIBS $CHAOS_LIBS"
dnl
dnl Now check if the installed chaos is sufficiently new. (Also sanity
dnl checks the results of libchaos-config to some extent
dnl
      rm -f conf.chaostest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libchaos/io.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.chaostest");
  */
  { FILE *fp = fopen("conf.chaostest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_chaos_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_chaos_version");
     exit(1);
   }

   if (($chaos_major_version > major) ||
      (($chaos_major_version == major) && ($chaos_minor_version > minor)) ||
      (($chaos_major_version == major) && ($chaos_minor_version == minor) && ($chaos_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'libchaos-config --version' returned %d.%d.%d, but the minimum version\n", $chaos_major_version, $chaos_minor_version, $chaos_micro_version);
      printf("*** of chaos required is %d.%d.%d. If libchaos-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If libchaos-config was wrong, set the environment variable chaos_CONFIG\n");
      printf("*** to point to the correct copy of libchaos-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_chaos=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_chaos" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$chaos_CONFIG" = "no" ; then
       echo "*** The libchaos-config script installed by libchaos could not be found"
       echo "*** If libchaos was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the chaos_CONFIG environment variable to the"
       echo "*** full path to libchaos-config."
     else
       if test -f conf.chaostest ; then
        :
       else
          echo "*** Could not run chaos test program, checking why..."
          CFLAGS="$CFLAGS $CHAOS_CFLAGS"
          LIBS="$LIBS $CHAOS_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "chaos.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding chaos or finding the wrong"
          echo "*** version of chaos. If it is not finding chaos, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means chaos was incorrectly installed"
          echo "*** or that you have moved chaos since it was installed. In the latter case, you"
          echo "*** may want to edit the libchaos-config script: $chaos_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     CHAOS_CFLAGS=""
     CHAOS_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(CHAOS_CFLAGS)
  AC_SUBST(CHAOS_LIBS)
  rm -f conf.chaostest
])

# $Id: dep.m4,v 1.2 2005/02/19 08:19:08 smoli Exp $
# ===========================================================================
#
# Implement dependency tracking
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_DEP], 
[AC_MSG_CHECKING([whether to enable dependencies])

AC_ARG_ENABLE([dep],
[  --enable-dep             dependency tracking
  --disable-dep            no dependency tracking (default)],
[case $enableval in
  yes)
    DEP="yes"
    AC_MSG_RESULT([yes])
    ;;
  *)
    DEP="no"
    AC_MSG_RESULT([no])
    ;;
  esac], 
  [DEP="no"
  AC_MSG_RESULT([no])])
if test "$DEP" = "yes"; then
  NODEP=""
else
  NODEP="# "
fi
AC_SUBST(NODEP)
AC_SUBST(DEP)])

# $Id: debug.m4,v 1.10 2005/05/04 22:20:32 smoli Exp $
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
  CFLAGS="`echo $CFLAGS -g -ggdb -O0 -Wall`"
#  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1 -Werror -include assert.h`"
#  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1 -include assert.h`"
  CPPFLAGS="`echo $CPPFLAGS -DDEBUG=1`"
  DEBUG="yes"
  AC_MSG_RESULT([yes])
else
  CFLAGS="`echo $CFLAGS -Os -fexpensive-optimizations -fomit-frame-pointer -Wall`"
#  CPPFLAGS="`echo $CPPFLAGS -DNDEBUG -include assert.h`"
  CPPFLAGS="`echo $CPPFLAGS -DNDEBUG`"
  DEBUG="no"
  AC_MSG_RESULT([no])
fi
  
AC_SUBST(DEBUG)])

# $Id: maintainer.m4,v 1.4 2005/05/04 22:20:32 smoli Exp $
# ===========================================================================
#
# Sophisticated debugging mode macro for autoconf
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for maintainer mode
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_MAINTAINER],
[ac_cv_maintainer="no"

AC_MSG_CHECKING([whether to enable maintainer mode])

AC_ARG_ENABLE([maintainer],
[  --enable-maintainer      maintainer mode
  --disable-maintainer     no maintainer mode (default)],
[case "$enableval" in
  yes)
    ac_cv_maintainer="yes"
    ;;
  no)
    ac_cv_maintainer="no"
    ;;
esac])

if test "$ac_cv_maintainer" = "yes"; then
  MAINTAINER=""
  NO_MAINTAINER="#"
  AC_MSG_RESULT([yes])
else
  MAINTAINER="#"
  NO_MAINTAINER=""
  AC_MSG_RESULT([no])
fi
  
AC_SUBST(MAINTAINER)
AC_SUBST(NO_MAINTAINER)
])

# $Id: summary.m4,v 1.9 2005/05/04 22:20:32 smoli Exp $
# ===========================================================================
#
# m4 macros for outputting a summary after the ./configure script ran
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>
#
m4_define([AC_SUMMARY_FORMAT], "%27s: %s")
#
# directories
# ---------------------------------------------------------------------------
m4_defun([AC_INIT_DIRS], [m4_define([AC_SWITCH_DIRS])
AC_ADD_DIR([prefix],         [Prefix])dnl
AC_ADD_DIR([bindir],         [User executables])dnl
AC_ADD_DIR([sbindir],        [System admin executables])dnl
AC_ADD_DIR([libexecdir],     [Program executables])dnl
AC_ADD_DIR([datadir],        [Read-only data])dnl
AC_ADD_DIR([sysconfdir],     [Configuration files])dnl
AC_ADD_DIR([sharedstatedir], [Portable data])dnl
AC_ADD_DIR([localstatedir],  [Binary data])dnl
AC_ADD_DIR([libdir],         [Libraries])dnl
AC_ADD_DIR([includedir],     [C header files])dnl
AC_ADD_DIR([oldincludedir],  [C header files for non-gcc])dnl
AC_ADD_DIR([infodir],        [Info documentation])dnl
AC_ADD_DIR([mandir],         [Manual pages])])
#
# variables
# ---------------------------------------------------------------------------
m4_defun([AC_INIT_VARS], [m4_define([AC_SWITCH_VARS])
AC_ADD_VAR([host],     [Host system])dnl
AC_ADD_VAR([build],    [Build system])dnl
AC_ADD_VAR([target],   [Target system])dnl
AC_ADD_VAR([CC],       [Compiler])dnl
AC_ADD_VAR([CXX],      [C++ compiler])dnl
AC_ADD_VAR([CFLAGS],   [Compiler flags])dnl
AC_ADD_VAR([PIC_CFLAGS], [PIC compiler flags])dnl
AC_ADD_VAR([CXXFLAGS], [C++ compiler flags])dnl
AC_ADD_VAR([DEFS],     [Preprocessor defines])dnl
AC_ADD_VAR([CPPFLAGS], [Preprocessor flags])dnl
AC_ADD_VAR([LDFLAGS],  [Linker flags])dnl
AC_ADD_VAR([PIE_LDFLAGS],  [PIE linker flags])dnl
AC_ADD_VAR([LIBS],     [Libraries])dnl
AC_ADD_VAR([A_ENABLE],     [Build static library])dnl
AC_ADD_VAR([PIE_ENABLE],   [Build shared library])dnl
AC_ADD_VAR([SSL_CFLAGS],   [SSL includes])dnl
AC_ADD_VAR([SSL_LIBS],     [SSL libs])dnl
AC_ADD_VAR([FT2_CFLAGS],   [Freetype 2 includes])dnl
AC_ADD_VAR([FT2_LIBS],     [Freetype 2 libs])dnl
AC_ADD_VAR([PSQL_CFLAGS], [PostgreSQL includes])dnl
AC_ADD_VAR([PSQL_LIBS],   [PostgreSQL libs])dnl
AC_ADD_VAR([MYSQL_CFLAGS], [MySQL includes])dnl
AC_ADD_VAR([MYSQL_LIBS],   [MySQL libs])dnl
AC_ADD_VAR([SDL_CFLAGS],   [SDL includes])dnl
AC_ADD_VAR([SDL_LIBS],     [SDL libs])dnl
AC_ADD_VAR([LIBPNG_CFLAGS],   [libpng includes])dnl
AC_ADD_VAR([LIBPNG_LIBS],     [libpng libs])dnl
AC_ADD_VAR([DEP],      [Dependency tracking])dnl
AC_ADD_VAR([COLOR],    [Colorful makefile])dnl
AC_ADD_VAR([DEBUG],    [Debug build])])
#
# AC_SUMMARIZE
# ---------------------------------------------------------------------------
# print a summary of configured options after the script ran
#
# Syntax:
#
#   AC_SUMMARIZE([directories], [variables], [format]) 
#
#   [directories]     set the directories you want in the summary
#   [variables]       set the variables you want in the summary
#   [format]          format string for output line
#
# Example:
#
#   AC_SUMMARIZE([bindir sysconfdir], [CFLAGS LIBS])
#
AC_DEFUN([AC_SUMMARIZE],
[AC_SUMMARIZE_HEADER
echo
AC_SUMMARIZE_DIRS($1, $3)
echo
AC_SUMMARIZE_VARS($2, $3)
echo])
#
# AC_SUMMARIZE_DIRS
# ---------------------------------------------------------------------------
# print out a directory summary
#
# Syntax:
#
#   AC_SUMMARIZE_DIRS([directories], [format])
#
#   [directories]   names of variables you want to display,
#                   if none are specified, you'll get all
#   [format]          format string for output line
#
# Example:
#
#   AC_SUMMARIZE_DIRS([sbindir sysconfdir sharedstatedir])
#
AC_DEFUN([AC_SUMMARIZE_DIRS],
[m4_ifvaln([$2], m4_define([AC_SUMMARY_FORMAT], $2))AC_INIT_DIRS
# if no args given, display all substituted dirs
m4_ifvaln([$1], [ac_summarize_dirs="$1"], [ac_summarize_dirs="AC_DEFAULT_DIRS"])

# loop through substituted dirs and show info
for ac_summarize_dir in $ac_summarize_dirs
do
  case $ac_summarize_dir in AC_SWITCH_DIRS
    *)
      # print an empty line
      echo
      ;;
  esac
done])
#
# AC_SUMMARIZE_VARS
# ---------------------------------------------------------------------------
# print out a variable summary
#
# Syntax:
#
#   AC_SUMMARIZE_VARS([variables])
#
#   [variables]    names of variables you want to display,
#                  if none are specified, you'll get all
# Example:
#
#   AC_SUMMARIZE_VARS([CC CFLAGS CXX CXXFLAGS])
#
AC_DEFUN([AC_SUMMARIZE_VARS], 
[m4_ifvaln([$2], m4_define([AC_SUMMARY_FORMAT], $2))AC_INIT_VARS
# if no args given, display all substituted vars
m4_ifvaln([$1], [ac_summarize_vars="$1"], [ac_summarize_vars="AC_DEFAULT_VARS"])

# loop through substituted vars and show info
for ac_summarize_var in $ac_summarize_vars
do
  case $ac_summarize_var in AC_SWITCH_VARS
    *)
      # print an empty line
      echo
      ;;
  esac
done])
#
# AC_SUMMARIZE_HEADER
# ---------------------------------------------------------------------------
# print out a summary header
#
# Syntax:
#
#   AC_SUMMARIZE_HEADER
#
AC_DEFUN([AC_SUMMARIZE_HEADER],
[echo
echo "$PACKAGE_NAME-$PACKAGE_VERSION has been configured with the following options:"])
#
# AC_ADD_DIR
# ---------------------------------------------------------------------------
# internally used by summary functions
#
m4_defun([AC_ADD_DIR],
[m4_append_uniq([AC_DEFAULT_DIRS], [$1], [ ])m4_append([AC_SWITCH_DIRS], [    "$1")
      eval $1="$$1" ; eval $1="$$1"
      printf AC_SUMMARY_FORMAT "$2" ""
      AC_MSG_DIR([$$1]) ;;])])
#
# AC_ADD_VAR
# ---------------------------------------------------------------------------
# internally used by summary functions
#
m4_defun([AC_ADD_VAR],
[m4_append_uniq([AC_DEFAULT_VARS], [$1], [ ])m4_append([AC_SWITCH_VARS], [
    "$1")
      $1=`echo $$1`
      if test "$$1"; then
        printf AC_SUMMARY_FORMAT "$2" ""
        AC_MSG_RESULT([$$1])
      fi 
      ;;
])])

