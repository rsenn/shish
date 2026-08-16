#include "../fdstack.h"
#include "../sh.h"
#include "../vartab.h"

struct env sh_root = {/* .parent = */ NULL,
                      /* .cwd = */ {NULL, 0, 0},
                      /* .cwdsym = */ 0,
                      /* .umask = */ 022,
                      /* .exitcode = */ 0,
                      /* .cmdsubst_ran = */ 0,
                      /* .opts = */
                      /* real designated initializers here, unlike the
                         rest of this struct -- positional init of
                         struct shopt would silently mis-assign every
                         default if a field is ever added/reordered. */
                      {.allexport = 0,
                       .errexit = 0,
                       .noglob = 0,
                       .hashall = 1,
                       .monitor = 0,
                       .noexec = 0,
                       .privileged = 0,
                       .unset = 0,
                       .xtrace = 0,
                       .braceexpand = 1,
                       .noclobber = 0,
                       .histexpand = 0},
                      /* .fdstack = */ &fdstack_root,
                      /* .varstack = */ &vartab_root,
                      /* .arg = */ {0},
                      /* .parser = */ 0,
                      /* .eval = */ 0};

struct env* sh = &sh_root;

pid_t sh_pid;
pid_t sh_shpid;
uid_t sh_uid;
const char* sh_home;
char* sh_argv0;
