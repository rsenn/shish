#include "sh.h"
#ifndef WIN32
#include <unistd.h>
#endif
#include <limits.h>
#include "shell.h"
#include "str.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

void sh_getcwd(struct env *sh)
{
  static char rootcwd[PATH_MAX + 1];

  stralloc_init(&sh->cwd);
  
  if (sh == &sh_root) {
#ifdef WIN32
	  GetCurrentDirectoryA(sizeof(rootcwd), rootcwd);
	  sh->cwd.s = rootcwd;
#else
	  sh->cwd.s = getcwd(rootcwd, sizeof(rootcwd));
#endif
  }

  if(sh->cwd.s == NULL)
    shell_getcwd(&sh->cwd, sizeof(rootcwd));
  else
    sh->cwd.len = str_len(sh->cwd.s);
}
