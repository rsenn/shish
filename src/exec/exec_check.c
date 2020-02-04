#include "../exec.h"
#include "../sh.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#include <io.h>
#ifndef X_OK
#define X_OK 1
#endif
#else
#include <unistd.h>
#endif

/* check if the path is valid
 * ----------------------------------------------------------------------- */
char*
exec_check(char* path) {
  char* ret = NULL;

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
