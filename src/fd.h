#ifndef FD_H
#define FD_H

/*#define __USE_LARGEFILE64*/
#include <fcntl.h>

#include "shell.h"
#include "buffer.h"
#include "stralloc.h"
#include <sys/stat.h>

#ifdef FD_SETSIZE
#define FD_MAX FD_SETSIZE
#else
#define FD_MAX 1024
#endif

#define FD_BUFSIZE   1024
#define FD_BUFSIZE2  (FD_BUFSIZE >> 1)

#ifndef STDSRC_FILENO 
#define STDSRC_FILENO -1
#endif
#ifndef STDIN_FILENO 
#define STDIN_FILENO   0
#endif
#ifndef STDOUT_FILENO 
#define STDOUT_FILENO  1
#endif
#ifndef STDERR_FILENO 
#define STDERR_FILENO  2
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

struct fdtable;

#define FD_ISRD(i) ((i)->mode & FD_READ)
#define FD_ISWR(i) ((i)->mode & FD_WRITE)

#define FD_SIZE (sizeof(struct fd))

struct fd {
  int             n;      /* virtual fd */
  int             e;      /* effective fd */
  int             mode;   /* FD_ modes */
  const char     *name;   /* name string */

  struct fd    *next;
  struct fd   **link;
  struct fd    *parent; 
  struct fd   **pos;
  struct fd    *dup;    /* this fd is a duplicate of ->dup */
  struct fdstack *stack;  /* stack level */

  dev_t           dev;    /* device for terminal recognition */
  int             fl;     /* posix fcntl() flags */

  buffer          rb;
  buffer          wb;
  buffer         *r;     /* pointer to effective read buffer */
  buffer         *w;     /* pointer to effective write buffer */
};

#define fd_ok(e)      ((e) >= 0 && (e) < FD_MAX)

/* fd mode */
#define FD_READ       0x00000001
#define FD_WRITE      0x00000002
#define FD_APPEND     0x00000004
#define FD_EXCL       0x00000008
#define FD_TRUNC      0x00000010

/* types */
#define FD_TYPE       0x0007ff00

#define FD_FILE       0x00000100  /* a file that has been opened */
#define FD_DIR        0x00000200 
#define FD_LINK       0x00000400 
#define FD_CHAR       0x00000800 
#define FD_BLOCK      0x00001000
#define FD_SOCKET     0x00002000
#define FD_PIPE       0x00004000  /* a pipe */
#define FD_STRALLOC   0x00008000  /* a stralloc */
#define FD_STRING     0x00010000  /* a nul-terminated string */
#define FD_DUP        0x00020000  /* a clone of another file descriptor */
#define FD_TERM       0x00040000  /* is a terminal */
#define FD_NULL       0x00080000

#define FD_SUBST      (FD_STRALLOC|FD_WRITE)
#define FD_HERE       (FD_STRALLOC|FD_READ)

#define FD_READWRITE  (FD_READ|FD_WRITE)
#define FD_DEFFILE    (FD_FILE|FD_CLOSE)
#define FD_READFILE   (FD_DEFFILE|FD_READ)
#define FD_WRITEFLUSH (FD_FLUSH|FD_WRITE)
#define FD_WRITEFILE  (FD_DEFFILE|FD_WRITEFLUSH)
#define FD_RWFILE     (FD_READFILE|FD_WRITEFILE)

/* todo mode */
#define FD_FLUSH      0x01000000  /* flush the buffer */
#define FD_CLOSE      0x02000000  /* close the buffer */
#define FD_FREENAME   0x04000000  /* free the name */
#define FD_DUPNAME    0x08000000  /* copy the name */
#define FD_FREE       0x10000000  /* free the fd struct */
#define FD_TMPBUF     0x40000000  /* buf alloca */
#define FD_OPEN       0x80000000

extern struct fd **const fdtable;

#define fd_foreach(i)      for(i = fd_lo; i < fd_hi; i++) if(fd_list[i])
#define fd_foreach_p(i, p) for(i = fd_lo; i < fd_hi; i++) if((p = fd_list[i]))

/* current standard fds */
#define fd_src fdtable[STDSRC_FILENO]
#define fd_in  fdtable[STDIN_FILENO]
#define fd_out fdtable[STDOUT_FILENO]
#define fd_err fdtable[STDERR_FILENO]

extern struct fd   *fd_list[FD_MAX];
extern unsigned int fd_exp;
extern unsigned int fd_top;
extern int          fd_lo;
extern int          fd_hi;
extern struct fd    fd_nullfd;

int fd_dup(struct fd *fd, int dfd);
int fd_error(int n, const char *msg);
int fd_exec(struct fd *fd);
int fd_getname(struct fd *fd);
int fd_mmap(struct fd *fd, const char *fname);
int fd_needbuf(struct fd *fd);
int fd_null(struct fd *fd);
int fd_pipe(struct fd *fd);
int fd_setfd(struct fd *fd, int e);
int fd_stat(struct fd *fd);
int fd_tempfile(struct fd *fd);
struct fd *fd_new(int fd, int mode);
struct fd *fd_push(struct fd *fd, int n, int mode);
struct fd *fd_reinit(struct fd *fd, int flags);
void fd_allocbuf(struct fd *fd, unsigned long n);
void fd_close(struct fd *fd);
void fd_dump(struct fd *fd);
void fd_dumplist(void);
void fd_free(struct fd *fd);
void fd_here(struct fd *fd, stralloc *sa);
void fd_init(struct fd *fd, int n, int flags);
void fd_open(struct fd *fd, const char *fname, long mode);
void fd_pop(struct fd *fd);
void fd_print(struct fd *fd);
void fd_setbuf(struct fd *fd, void *buf, unsigned long n);
void fd_string(struct fd *fd, const char *s, unsigned long len);
void fd_subst(struct fd *fd, stralloc *sa);

#define fd_malloc(io) io = shell_alloc(FD_SIZE);
#define fd_mallocb(io) io = shell_alloc(FD_SIZE + FD_BUFSIZE);
#define fd_alloca(io) io = alloca(FD_SIZE);
#define fd_allocab(io) io = alloca(FD_SIZE + FD_BUFSIZE);

#endif /* FD_H */
