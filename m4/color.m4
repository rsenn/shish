# $Id: color.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
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

ac_cv_silent_rules="yes"
AC_ARG_ENABLE(silent-rules,
[  --disable-silent-rules         echo commands while compiling (automake style)
  --enable-silent-rules          do not echo commands while compiling, print 
                          variable-like name of program used (linux style)],
[case "$enableval" in
  no)
    ac_cv_silent_rules="no"
    ;;
  *)
    ac_cv_silent_rules="yes"
    ;;
esac])
if test "$ac_cv_silent_rules" = "yes"; then
#  MAKEFLAGS=""
  MAKEFLAGS="-s"
  #QUIET="--silent-rules"
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

