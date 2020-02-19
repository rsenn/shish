#include "../../lib/windoze.h"
#include "../fd.h"
#include <sys/stat.h>

#if WINDOWS_NATIVE
#include <io.h>
#define stat _stat
#define fstat _fstat
#ifndef S_IFMT
#define S_IFMT 0xf000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0x4000
#endif /* defined(S_IFDIR) */
#ifndef S_IFCHR
#define S_IFCHR 0x2000
#endif /* defined(S_IFCHR) */
#ifndef S_IFIFO
#define S_IFIFO 0x1000
#endif /* defined(S_IFIFO) */
#ifndef S_IFREG
#define S_IFREG 0x8000
#endif /* defined(S_IFREG) */
#ifndef S_IFBLK
#define S_IFBLK 0x6000
#endif /* defined(S_IFBLK) */
#ifndef S_IFLNK
#define S_IFLNK 0xA000
#endif /* defined(S_IFLNK) */
#else
#define _stat stat
#include <unistd.h>
#endif

/* stat the (fd) and set appropriate flags
 * ----------------------------------------------------------------------- */
int
fd_stat(struct fd* fd) {
  struct stat st;

  if(fd->mode & FD_TYPE)
    return 0;

  if(fstat(fd->n, &st) == -1)
    return 1;

  fd->dev = st.st_rdev;

  switch(st.st_mode & S_IFMT) {
  case S_IFREG: fd->mode |= FD_FILE; break;
  case S_IFDIR: fd->mode |= FD_DIR; break;
  case S_IFCHR: fd->mode |= FD_CHAR; break;
#ifdef S_IFBLK
  case S_IFBLK: fd->mode |= FD_BLOCK; break;
#endif
#ifdef S_IFIFO
  case S_IFIFO: fd->mode |= FD_PIPE; break;
#endif
#ifdef S_IFLNK
  case S_IFLNK: fd->mode |= FD_LINK; break;
#endif
#ifdef S_IFSOCK
  case S_IFSOCK: fd->mode |= FD_SOCKET; break;
#endif
  }

  return 0;
}
