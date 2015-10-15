#include "stralloc.h"
#include "byte.h"
#include "str.h"

#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
# ifndef HAVE_LSTAT
#  define lstat stat
# endif
#endif

/* canonicalizes a <path> and puts it into <sa>
 * 
 * <path>, without trailing '\0', should not be longer than PATH_MAX or it
 * is truncated!
 * 
 * if symbolic is zero then it reads symlinks and puts the physical 
 * path into the destination buffer
 * 
 * returns zero on error and 1 if the whole path has no symlink,
 * so the return value - 1 is the count of symlinks
 * 
 * the <path> should be absolute though it will work on relative paths,
 * but they should be relative to the current dir and shell_canonicalize()
 * will again return a relative path.
 * because of that the behaviour of this function differs from usual path 
 * canonicalizing functions like realpath() in libc, but there is a 
 * shell_realpath() function which provides similar behaviour and will
 * resolve relative paths to absolute ones.
 * ----------------------------------------------------------------------- */
int shell_canonicalize(const char *path, stralloc *sa, int symbolic) {
  unsigned long n;
  struct stat st;
  int ret = 1;
  int (*stat_fn)() = stat;
#ifdef HAVE_LSTAT
  char buf[PATH_MAX + 1];
  
#if defined(HAVE_LSTAT ) && !defined(__MINGW32__)
  if(symbolic)
   stat_fn = lstat;
#endif

start:
#endif
  /* loop once for every /path/component/
     we canonicalize absolute paths, so we must always have a '/' here */
  while(*path) {
    while(*path == '/') path++;
    
    /* check for various relative directory parts beginning with '.' */
    if(path[0] == '.') {
      /* strip any "./" inside the path or a trailing "." */
      if(path[1] == '/' || path[1] == '\0') {
        path++;
        continue;
      }
      
      /* if we have ".." we have to truncate the resulting path */
      if(path[1] == '.' && (path[2] == '/' || path[2] == '\0')) {
        sa->len = byte_rchr(sa->s, sa->len, '/');
        path += 2;
        continue;
      }
    }
    
    /* exit now if we're done */
    if(*path == '\0')
      break;

    /* begin a new path component */
    stralloc_catc(sa, '/');

    /* look for the next path separator and then copy the component */
    n = str_chr(path, '/');
    stralloc_catb(sa, path, n);
    stralloc_nul(sa);

    path += n;

    /* now stat() the thing to verify it */
    if(stat_fn(sa->s, &st) == -1)
      return 0;

#ifdef HAVE_LSTAT
    /* is it a symbolic link? */
    if(S_ISLNK(st.st_mode)) {
      ret++;

      /* read the link, return if failed and then nul-terminate the buffer */
      if((n = readlink(sa->s, buf, PATH_MAX)) == -1)
        return 0;

      buf[n] = '\0';

      /* if the symlink is absolute we clear the stralloc, 
         set the path to buf and repeat the whole procedure */
      if(buf[0] == '/') {
        stralloc_zero(sa);

        path = buf;
        goto start;
      }
      /* if the symlink is relative we remove the symlink path 
         component and recurse */
      else {
        sa->len = byte_rchr(sa->s, sa->len, '/');

        if(!shell_canonicalize(buf, sa, symbolic))
          return 0;
      }
    }
 #endif
    
    /* it isn't a directory :( */
    if(!S_ISDIR(st.st_mode)) {
      errno = ENOTDIR;
      return 0;
    }
  }
  
  if(sa->len == 0)
    stralloc_catc(sa, '/');

  return ret;
}

