#include "builtin.h"
#include "history.h"

/* builtin lookup table
 * ----------------------------------------------------------------------- */
struct builtin_cmd builtin_table[] = {
    {".", builtin_source, B_SPECIAL, "file [arguments]"},
    {":", builtin_true, B_SPECIAL, ""},
    {"basename", builtin_basename, B_DEFAULT, "path"},
    {"break", builtin_break, B_DEFAULT, "[n]"},
    {"cd", builtin_cd, B_DEFAULT, "[-L|-P] [directory]"},
    {"continue", builtin_break, B_DEFAULT, "[n]"},
    {"dirname", builtin_dirname, B_DEFAULT, "path"},
#ifdef DEBUG
    {"dump", builtin_dump, B_DEFAULT, "[-v]"},
#endif /* DEBUG */
    {"echo", builtin_echo, B_DEFAULT, "[-ne] [arg ...]"},
    {"eval", builtin_eval, B_SPECIAL, "[args]"},
    {"exec", builtin_exec, B_EXEC, "[-cl] [-a name] [cmd [args]]"},
    {"exit", builtin_exit, B_SPECIAL, "[exitcode]"},
    {"export", builtin_export, B_SPECIAL, "[-np] [name=[value]]"},
    {"false", builtin_false, B_DEFAULT, ""},
    {"fdtable", builtin_fdtable, B_DEFAULT, ""},
    {"hash", builtin_hash, B_DEFAULT, ""},
    {HISTORY, builtin_history, B_DEFAULT, "[-c]"},
    {"hostname", builtin_hostname, B_DEFAULT, "[name]"},
    {"pwd", builtin_pwd, B_DEFAULT, "[-L|-P]"},
    {"set", builtin_set, B_SPECIAL, "[arguments]"},
    {"shift", builtin_shift, B_SPECIAL, "[n]"},
    {"source", builtin_source, B_DEFAULT, "file [arguments]"},
    {"test", builtin_test, B_DEFAULT, "[expr]"},
    {"true", builtin_true, B_DEFAULT, ""},
    {"unset", builtin_unset, B_SPECIAL, "[name ...]"},
    {"[", builtin_test, B_DEFAULT, "[expr] ]"},
    {NULL, NULL, 0, NULL},
};
