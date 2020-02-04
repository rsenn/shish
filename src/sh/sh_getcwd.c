#include "sh.h"
#include "shell.h"
#include "str.h"
#include <limits.h>
#include "windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

void
sh_getcwd(struct env* sh) {
  static char rootcwd[PATH_MAX + 1];

  stralloc_init(&sh->cwd);

  if(sh == &sh_root)
    sh->cwd.s = getcwd(rootcwd, sizeof(rootcwd));

  if(sh->cwd.s == NULL)
    shell_getcwd(&sh->cwd);
  else
    sh->cwd.len = str_len(sh->cwd.s);
}
