#include "sh.h"
#include <unistd.h>
#include <limits.h>
#include <shell.h>
#include <str.h>

void sh_getcwd(struct env *sh)
{
  static char rootcwd[PATH_MAX + 1];

  stralloc_init(&sh->cwd);
  
  if(sh == &sh_root)
    sh->cwd.s = getcwd(rootcwd, sizeof(rootcwd));

  if(sh->cwd.s == NULL)
    shell_getcwd(&sh->cwd, sizeof(rootcwd));
  else
    sh->cwd.len = str_len(sh->cwd.s);
}
