#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

void
sh_getcwd(struct env* sh) {
  //static char rootcwd[PATH_MAX + 1];

  stralloc_init(&sh->cwd);

  //if(sh == &sh_root)
  //  sh->cwd.s = getcwd(rootcwd, sizeof(rootcwd));

  if(sh == &sh_root || sh->cwd.s == NULL)
    shell_getcwd(&sh->cwd);
  else
    sh->cwd.len = str_len(sh->cwd.s);
}
