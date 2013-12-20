#include <unistd.h>
#include <limits.h>
#include "shell.h"
#include "str.h"
#include "exec.h"
#include "var.h"

/* searches for relative path <name> within PATH and returns absolute
 * path if found
 * ----------------------------------------------------------------------- */
char *exec_path(char *name)
{
  const char *vpath;
  static char path[PATH_MAX];
  unsigned long si = 0, pi = 0;
  vpath = var_value("PATH", NULL);

  do
  {
    /* splitting up path variable */
    unsigned long ni;

    if(vpath[si] == ':')
      si++;

    ni = str_chr(&vpath[si], ':');

    if(ni >= PATH_MAX)
    {
      si += ni;
      continue;
    }
    
    /* copying path to temp buffer */
    pi = str_copyn(path, &vpath[si], ni);

    if(pi && pi < PATH_MAX - 1)
      if(path[pi - 1] != '/')
        path[pi++] = '/';

    /* append command name */
    str_copyn(&path[pi], name, PATH_MAX - pi - 1);

    /* check if file accessible and executable */
    if(access(path, X_OK) == 0)
      return shell_strdup(path);

    si += ni;
  }
  while(vpath[si]);

  path[0] = '\0';
  return NULL;
}

