#include "../../lib/alloc.h"
#include "../exec.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/path.h"
#include "../var.h"
#include "../../lib/windoze.h"
#include <errno.h>
#if WINDOWS_NATIVE
#include <windows.h>
#include <io.h>
#ifndef X_OK
#define X_OK 1
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#endif

int exec_lasterrno;
int exec_redir_error;

/* path_search() predicate: the candidate is executable and not a directory.
 * access(X_OK) alone isn't enough -- directories are "executable" (searchable)
 * too, so a directory on PATH would otherwise match and shadow the real command
 * of the same name (e.g. an SDK's "test/" build output directory sitting on PATH
 * ahead of /usr/bin/test). Failures other than ENOENT are kept in exec_lasterrno. */
static int
exec_path_ok(const char* path, void* arg) {
  (void)arg;

  if(access(path, X_OK) == 0) {
#if !WINDOWS_NATIVE
    struct stat st;

    if(stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
      exec_lasterrno = EISDIR;
      return 0;
    }
#endif
    return 1;
  }

  if(errno != ENOENT)
    exec_lasterrno = errno;

  return 0;
}

/* searches for relative path <name> within PATH and returns absolute
 * path if found
 * ----------------------------------------------------------------------- */
char*
exec_path(char* name) {
  const char* vpath = var_value("PATH", NULL);
  char path[PATH_MAX];

  exec_lasterrno = ENOENT;

  if(path_search(path, sizeof(path), vpath, str_len(vpath), name, str_len(name), exec_path_ok, NULL))
    return str_dup(path);

  return NULL;
}
