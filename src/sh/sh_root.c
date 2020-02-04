#include "../fdstack.h"
#include "../sh.h"
#include "../vartab.h"

struct env sh_root = {/* .parent = */ NULL,
                      /* .cwd = */ {NULL, 0, 0},
                      /* .cwdsym = */ 0,
                      /* .umask = */ 022,
                      /* .exitcode = */ 0,
                      /* .fdstack = */ &fdstack_root,
                      /* .varstack = */ &vartab_root};

struct env* sh = &sh_root;

pid_t sh_pid;
uid_t sh_uid;
const char* sh_home;
char* sh_argv0;
