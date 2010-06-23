#include "sh.h"
#include "fdstack.h"
#include "vartab.h"

struct env sh_root =
{
  .parent      = NULL,
//  .flags       = 0,
  .cwd         = { NULL, 0, 0 },
  .umask       = 022,
  .arg.c       = 0,
  .arg.v       = NULL,
  .arg.a       = 0,
  .arg.s       = 0,
  .eval        = NULL,
  .fdstack     = &fdstack_root,
  .varstack    = &vartab_root
};

struct env *sh = &sh_root;

pid_t       sh_pid;
uid_t       sh_uid;
const char *sh_home;
char       *sh_argv0;
