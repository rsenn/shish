#include "../builtin.h"
#include "../history.h"

#include "src/builtin_config.h"

#ifndef BUILTIN_BREAK
#define BUILTIN_BREAK 1
#endif
#ifndef BUILTIN_CD
#define BUILTIN_CD 1
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
#ifndef BUILTIN_FALSE
#define BUILTIN_FALSE 1
#endif
#ifndef BUILTIN_FDTABLE
#define BUILTIN_FDTABLE 1
#endif
#ifndef BUILTIN_HASH
#define BUILTIN_HASH 1
#endif
#ifndef BUILTIN_HISTORY
#define BUILTIN_HISTORY 1
#endif
#ifndef BUILTIN_PWD
#define BUILTIN_PWD 1
#endif
#ifndef BUILTIN_SET
#define BUILTIN_SET 1
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
#ifndef BUILTIN_UNSET
#define BUILTIN_UNSET 1
#endif

/* builtin lookup table
 * ----------------------------------------------------------------------- */
struct builtin_cmd builtin_table[] = {
#if BUILTIN_SOURCE
    {".", builtin_source, B_SPECIAL, "file [arguments]"},
#endif
#if BUILTIN_TRUE
    {":", builtin_true, B_SPECIAL, ""},
#endif
#if BUILTIN_BASENAME
    {"basename", builtin_basename, B_DEFAULT, "path"},
#endif
#if BUILTIN_BREAK
    {"break", builtin_break, B_DEFAULT, "[n]"},
#endif
#if BUILTIN_CD
    {"cd", builtin_cd, B_DEFAULT, "[-L|-P] [directory]"},
#endif
#if BUILTIN_BREAK
    {"continue", builtin_break, B_DEFAULT, "[n]"},
#endif
#if BUILTIN_DIRNAME
    {"dirname", builtin_dirname, B_DEFAULT, "path"},
#endif
#ifdef DEBUG
#if BUILTIN_DUMP
    {"dump", builtin_dump, B_DEFAULT, "[-v]"},
#endif
#endif /* DEBUG */
#if BUILTIN_ECHO
    {"echo", builtin_echo, B_DEFAULT, "[-ne] [arg ...]"},
#endif
#if BUILTIN_EVAL
    {"eval", builtin_eval, B_SPECIAL, "[args]"},
#endif
#if BUILTIN_EXEC
    {"exec", builtin_exec, B_EXEC, "[-cl] [-a name] [cmd [args]]"},
#endif
#if BUILTIN_EXIT
    {"exit", builtin_exit, B_SPECIAL, "[exitcode]"},
#endif
#if BUILTIN_EXPORT
    {"export", builtin_export, B_SPECIAL, "[-np] [name=[value]]"},
#endif
#if BUILTIN_FALSE
    {"false", builtin_false, B_DEFAULT, ""},
#endif
#if BUILTIN_FDTABLE
    {"fdtable", builtin_fdtable, B_DEFAULT, ""},
#endif
#if BUILTIN_HASH
    {"hash", builtin_hash, B_DEFAULT, ""},
#endif
#if BUILTIN_HISTORY
    {HISTORY, builtin_history, B_DEFAULT, "[-c]"},
#endif
#if BUILTIN_HOSTNAME
    {"hostname", builtin_hostname, B_DEFAULT, "[name]"},
#endif
#if BUILTIN_PWD
    {"pwd", builtin_pwd, B_DEFAULT, "[-L|-P]"},
#endif
#if BUILTIN_SET
    {"set", builtin_set, B_SPECIAL, "[arguments]"},
#endif
#if BUILTIN_SHIFT
    {"shift", builtin_shift, B_SPECIAL, "[n]"},
#endif
#if BUILTIN_SOURCE
    {"source", builtin_source, B_DEFAULT, "file [arguments]"},
#endif
#if BUILTIN_TEST
    {"test", builtin_test, B_DEFAULT, "[expr]"},
#endif
#if BUILTIN_TRUE
    {"true", builtin_true, B_DEFAULT, ""},
#endif
#if BUILTIN_UNSET
    {"unset", builtin_unset, B_SPECIAL, "[name ...]"},
#endif
#if BUILTIN_TEST
    {"[", builtin_test, B_DEFAULT, "[expr] ]"},
#endif
    {NULL, NULL, 0, NULL},
};
