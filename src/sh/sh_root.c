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
                      /* real designated initializers here, not the
                         plain-positional-with-comments style the rest
                         of this struct uses -- struct shopt just
                         silently mis-assigned every single default
                         (hashall's "1" landed on noglob, braceexpand's
                         "1" landed on xtrace, ...) the moment
                         "allexport" was added as its new first field,
                         with nothing to catch it at compile time. */
                      {.allexport = 0,
                       .errexit = 0,
                       .noglob = 0,
                       .hashall = 1,
                       .monitor = 0,
                       .noexec = 0,
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
