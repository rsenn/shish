#include "../windoze.h"

#if WINDOWS_NATIVE
#include <sys/stat.h>
#include "../unix.h"

/* stat(), then patch st_mode's type bits to S_IFLNK if the path is a
 * reparse-point symlink -- stat() itself always follows the link, so
 * without this, callers see the link's target and never the link. */
int
lstat(const char* path, struct stat* buf) {
  int ret = stat(path, buf);

  if(ret == 0 && is_symlink(path))
    buf->st_mode = (buf->st_mode & ~S_IFMT) | S_IFLNK;

  return ret;
}
#endif
