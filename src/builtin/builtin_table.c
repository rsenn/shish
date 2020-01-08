#include "builtin.h"
#include "history.h"

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
