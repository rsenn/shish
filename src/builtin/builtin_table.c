#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../builtin.h"
#include "../history.h"

#ifdef HAVE_BUILTIN_CONFIG_H
#include "src/builtin_config.h"
#endif

#ifndef BUILTIN_ALIAS
#define BUILTIN_ALIAS 1
#endif
#ifndef BUILTIN_BREAK
#define BUILTIN_BREAK 1
#endif
#ifndef BUILTIN_CAT
#define BUILTIN_CAT 0
#endif
#ifndef BUILTIN_CD
#define BUILTIN_CD 1
#endif
#ifndef BUILTIN_CHMOD
#define BUILTIN_CHMOD 0
#endif
#ifndef BUILTIN_CONTINUE
#define BUILTIN_CONTINUE 1
#endif
#ifndef BUILTIN_DUMP
#define BUILTIN_DUMP 1
#endif
#ifndef BUILTIN_ECHO
#define BUILTIN_ECHO 1
#endif
#ifndef BUILTIN_EVAL
#define BUILTIN_EVAL 1
#endif
#ifndef BUILTIN_EXEC
#define BUILTIN_EXEC 1
#endif
#ifndef BUILTIN_EXIT
#define BUILTIN_EXIT 1
#endif
#ifndef BUILTIN_EXPORT
#define BUILTIN_EXPORT 1
#endif
#ifndef BUILTIN_EXPR
#define BUILTIN_EXPR 0
#endif
#ifndef BUILTIN_FALSE
#define BUILTIN_FALSE 1
#endif
#ifndef BUILTIN_FDTABLE
#define BUILTIN_FDTABLE 1
#endif
#ifndef BUILTIN_HASH
#define BUILTIN_HASH 1
#endif
#ifndef BUILTIN_GETOPTS
#define BUILTIN_GETOPTS 1
#endif
#ifndef BUILTIN_HELP
#define BUILTIN_HELP 0
#endif
#ifndef BUILTIN_HISTORY
#define BUILTIN_HISTORY 1
#endif
#ifndef BUILTIN_LOCAL
#define BUILTIN_LOCAL 1
#endif
#ifndef BUILTIN_LN
#define BUILTIN_LN 0
#endif
#ifndef BUILTIN_MKDIR
#define BUILTIN_MKDIR 0
#endif
#ifndef BUILTIN_PWD
#define BUILTIN_PWD 1
#endif
#ifndef BUILTIN_SET
#define BUILTIN_SET 1
#endif
#ifndef BUILTIN_READ
#define BUILTIN_READ 1
#endif
#ifndef BUILTIN_READONLY
#define BUILTIN_READONLY 1
#endif
#ifndef BUILTIN_RETURN
#define BUILTIN_RETURN 1
#endif
#ifndef BUILTIN_RM
#define BUILTIN_RM 0
#endif
#ifndef BUILTIN_RMDIR
#define BUILTIN_RMDIR 0
#endif
#ifndef BUILTIN_SHIFT
#define BUILTIN_SHIFT 1
#endif
#ifndef BUILTIN_SOURCE
#define BUILTIN_SOURCE 1
#endif
#ifndef BUILTIN_TRUE
#define BUILTIN_TRUE 1
#endif
#ifndef BUILTIN_TYPE
#define BUILTIN_TYPE 1
#endif
#ifndef BUILTIN_UMASK
#define BUILTIN_UMASK 1
#endif
#ifndef BUILTIN_UNSET
#define BUILTIN_UNSET 1
#endif
#ifndef BUILTIN_UNAME
#define BUILTIN_UNAME 0
#endif
#ifndef BUILTIN_WAIT
#define BUILTIN_WAIT 1
#endif
#ifndef BUILTIN_WHICH
#define BUILTIN_WHICH 0
#endif

/* builtin lookup table
 * ----------------------------------------------------------------------- */
struct builtin_cmd builtin_table[] = {
#if BUILTIN_SOURCE
    {".", &builtin_source, B_SPECIAL, "file [arguments]"},
#endif
#if BUILTIN_TRUE
    {":", &builtin_true, B_SPECIAL, ""},
#endif
#if BUILTIN_ALIAS
    {"alias", &builtin_alias, B_SPECIAL, "[-p] [name[=value] ...]"},
#endif
#if BUILTIN_BASENAME
    {"basename", &builtin_basename, B_DEFAULT, "path"},
#endif
#if BUILTIN_JOBS
    {"bg", &builtin_fg, B_DEFAULT, "[job...]"},
#endif
#if BUILTIN_BREAK
    {"break", &builtin_break, B_DEFAULT, "[n]"},
#endif
#if BUILTIN_CAT
    {"cat", &builtin_cat, B_DEFAULT, "[-nb] [FILE]..."},
#endif
#if BUILTIN_CD
    {"cd", &builtin_cd, B_DEFAULT, "[-L|-P] [directory]"},
#endif
#if BUILTIN_CHMOD
    {"chmod", &builtin_chmod, B_DEFAULT, "[-v] [FILE]..."},
#endif
#if BUILTIN_COMMAND
    {"command", &builtin_command, B_DEFAULT, "[-pVv] command [arg ...]"},
#endif
#if BUILTIN_CONTINUE
    {"continue", &builtin_break, B_DEFAULT, "[n]"},
#endif
#if BUILTIN_DIRNAME
    {"dirname", &builtin_dirname, B_DEFAULT, "path"},
#endif
#if BUILTIN_DUMP //&& defined(DEBUG_OUTPUT)
    {"dump", &builtin_dump, B_DEFAULT, "[-Fvltsfm] [-u FD]"},
#endif
#if BUILTIN_ECHO
    {"echo", &builtin_echo, B_DEFAULT, "[-ne] [arg ...]"},
#endif
#if BUILTIN_EVAL
    {"eval", &builtin_eval, B_SPECIAL, "[args]"},
#endif
#if BUILTIN_EXEC
    {"exec", &builtin_exec, B_EXEC, "[-cl] [-a name] [cmd [args]]"},
#endif
#if BUILTIN_EXIT
    {"exit", &builtin_exit, B_SPECIAL, "[exitcode]"},
#endif
#if BUILTIN_EXPORT
    {"export", &builtin_export, B_SPECIAL, "[-np] [name=[value]]"},
#endif
#if BUILTIN_EXPR
    {"expr", &builtin_expr, B_DEFAULT, "[expression]"},
#endif
#if BUILTIN_FALSE
    {"false", &builtin_false, B_DEFAULT, ""},
#endif
#if BUILTIN_FDTABLE
    {"fdtable", &builtin_fdtable, B_DEFAULT, "[-u FD]"},
#endif
#if BUILTIN_JOBS
    {"fg", &builtin_fg, B_DEFAULT, "[job...]"},
#endif
#if BUILTIN_GETOPTS
    {"getopts", &builtin_getopts, B_SPECIAL, "optstring name [arg ... ]"},
#endif
#if BUILTIN_HASH
    {"hash", &builtin_hash, B_DEFAULT, ""},
#endif
#if BUILTIN_HELP
    {"help", &builtin_help, B_DEFAULT, "[command]"},
#endif
#if BUILTIN_HISTORY
    {HISTORY, builtin_history, B_DEFAULT, "[-c]"},
#endif
#if BUILTIN_HOSTNAME
    {"hostname", &builtin_hostname, B_DEFAULT, "[name]"},
#endif
#if BUILTIN_JOBS
    {"jobs", &builtin_jobs, B_DEFAULT, "[-lnprs] [job...]"},
#endif
#if BUILTIN_LN
    {"ln", &builtin_ln, B_DEFAULT, "[-sfv]"},
#endif
#if BUILTIN_LOCAL
    {"local", &builtin_local, B_SPECIAL, "[option] name[=value] ..."},
#endif
#if BUILTIN_MKDIR
    {"mkdir", &builtin_mkdir, B_DEFAULT, "[-p]"},
#endif
#if BUILTIN_MKTEMP
    {"mktemp", &builtin_mktemp, B_DEFAULT, "[-dt] [-p DIR] [TEMPLATE]"},
#endif
#if BUILTIN_PWD
    {"pwd", &builtin_pwd, B_DEFAULT, "[-L|-P]"},
#endif
#if BUILTIN_SET
    {"set", &builtin_set, B_SPECIAL, "[arguments]"},
#endif
#if BUILTIN_SHIFT
    {"shift", &builtin_shift, B_SPECIAL, "[n]"},
#endif
#if BUILTIN_READ
    {"read",
     &builtin_read,
     B_SPECIAL,
     "[-rs] [-d DELIM] [-n|-N NCHARS] [-p PROMPT] [-t TIMEOUT] [-u FD] [name "
     "...]"},
#endif
#if BUILTIN_READONLY
    {"readonly", &builtin_readonly, B_SPECIAL, "[-p] [name[=value] ...]"},
#endif
#if BUILTIN_RETURN
    {"return", &builtin_return, B_SPECIAL, "[n]"},
#endif
#if BUILTIN_RM
    {"rm", &builtin_rm, B_DEFAULT, "[-vrf] [file]..."},
#endif
#if BUILTIN_RMDIR
    {"rmdir", &builtin_rmdir, B_DEFAULT, "[-p] [directory]..."},
#endif
#if BUILTIN_SOURCE
    {"source", &builtin_source, B_DEFAULT, "file [arguments]"},
#endif
#if BUILTIN_TEST
    {"test", &builtin_test, B_DEFAULT, "[expr]"},
#endif
#if BUILTIN_TRUE
    {"true", &builtin_true, B_DEFAULT, ""},
#endif
#if BUILTIN_TYPE
    {"type", &builtin_type, B_DEFAULT, "[-afptP] name [name ...]"},
#endif
#if BUILTIN_UMASK
    {"umask", &builtin_umask, B_SPECIAL, "[-p] [-S] [mode]"},
#endif
#if BUILTIN_UNSET
    {"unset", &builtin_unset, B_SPECIAL, "[name ...]"},
#endif
#if BUILTIN_TEST
    {"[", &builtin_test, B_DEFAULT, "expr ]"},
#endif
#if BUILTIN_TRAP
    {"trap", &builtin_trap, B_DEFAULT, "[-lp] [[arg] signal_spec ...]"},
#endif
#if BUILTIN_TYPE
    {"type", &builtin_type, B_DEFAULT, "name ..."},
#endif
#if BUILTIN_ALIAS
    {"unqalias", &builtin_alias, B_SPECIAL, "[-p] [name[=value] ...]"},
#endif
#if BUILTIN_UNAME
    {"uname", &builtin_uname, B_DEFAULT, "[-amnrspvio]"},
#endif
#if BUILTIN_WAIT
    {"wait", &builtin_wait, B_DEFAULT, "[pid...]"},
#endif
#if BUILTIN_WHICH
    {"which", &builtin_which, B_DEFAULT, "[-a] filename ..."},
#endif
    {NULL, NULL, 0, NULL},
};
