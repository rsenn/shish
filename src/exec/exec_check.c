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
#include <sys/stat.h>
#include <errno.h>
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

  /* otherwise check immediately for access. access(X_OK) alone would
     also accept a directory (it's "executable" as in searchable) --
     reject that explicitly with EISDIR, matching what execve() itself
     would report */
  else if(access(path, X_OK) == 0) {
#if !WINDOWS_NATIVE
    struct stat st;

    if(stat(path, &st) == 0 && S_ISDIR(st.st_mode))
      errno = EISDIR;
    else
#endif
      ret = path;
  }

  if(ret == NULL)
    sh_error_errno(path);

  return ret;
}
