#include "../fd.h"
#include "../../lib/fmt.h"
#include "../sh.h"
#include "../../lib/str.h"

/* try to get a name for the (fd)
 * ----------------------------------------------------------------------- */
int
fd_getname(struct fd* fd) {
  static int noproc;
  static int plen;
  static char path[6 + FMT_ULONG + 4 + FMT_ULONG + 1];
  stralloc name;

  /* if we're a duplicate then the name depends on our duplicatee */
  if(fd->dup) {
    int ret = 0;

    if(!fd->dup->name)
      ret = fd_getname(fd->dup);

    fd->name = fd->dup->name;
    return ret;
  }

  /* assemble /proc/<pid>/fd/ path */
  if(!plen) {
    plen += str_copy(path, "/proc/");
    plen += fmt_ulong(&path[plen], sh_pid);
    plen += str_copy(&path[plen], "/fd/");
  }

  stralloc_init(&name);

  /* try to read /proc/<pid>/fd/<fd> if not previously failed */
  if(!noproc) {
    /* format fd number and nul-terminate */
    path[plen + fmt_ulong(&path[plen], fd->n)] = '\0';

    /* try to read the link */
    if(!shell_readlink(path, &name)) {
      fd->name = name.s;
      fd->mode |= D_FREENAME;
      return 0;
    }

    noproc = 1;
  }

  /* we don't seem to have a /proc tree */
  if(fd->mode & D_FILE)
    fd->name = "file";
  if(fd->mode & D_DIR)
    fd->name = "directory";
  if(fd->mode & D_LINK)
    fd->name = "link";
  if(fd->mode & D_CHAR)
    fd->name = "char device";
  if(fd->mode & D_BLOCK)
    fd->name = "block device";
  if(fd->mode & D_SOCKET)
    fd->name = "socket";
  if(fd->mode & D_PIPE)
    fd->name = "pipe";

  return 0;
}
