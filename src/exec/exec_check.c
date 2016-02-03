<<<<<<< HEAD
#include "../msvc-compat.h"
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <unistd.h>
#endif
#include "str.h"
#include "exec.h"
#include "sh.h"

/* check if the path is valid
 * ----------------------------------------------------------------------- */
char *exec_check(char *path)
{
  char *ret = NULL;

  /* when there is no directory delimiter in the
     supplied path then search for the file */
  if(!path[str_chr(path, '/')])
    ret = exec_path(path);

  /* otherwise check immediately for access */
  else if(access(path, X_OK) == 0)
    ret = path;

  if(ret == NULL)
    sh_error(path);

  return ret;
}

