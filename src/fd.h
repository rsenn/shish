#ifndef FD_H
#define FD_H

/*#define __USE_LARGEFILE64*/
#include <fcntl.h>

#ifdef __TINYC__
#ifdef _WIN32
#define NO_OLDNAMES
#define pid_t _pid_t
#define dev_t _dev_t
#endif
#endif
#include <sys/types.h>
#undef NO_OLDNAMES

#include "../lib/buffer.h"
#include "../lib/shell.h"
#include "../lib/stralloc.h"
#include "../lib/windoze.h"
#if WINDOWS_NATIVE && !defined(__BORLANDC__) && !defined(__MINGW32__) && !defined(__TINYC__) && !defined(__LCC__)
#ifndef HAVE_DEV_T
typedef int dev_t;
#endif
#endif

#ifdef FD_SETSIZE
#define FD_MAX FD_SETSIZE
#else
#define FD_MAX 1024
#endif

#define FD_BUFSIZE 1024
#define FD_BUFSIZE2 (FD_BUFSIZE >> 1)

#ifndef STDSRC_FILENO
#define STDSRC_FILENO -1
#endif
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

struct fdtable;

#define FD_ISRD(fd) ((fd)->mode & FD_READ)
#define FD_ISWR(fd) ((fd)->mode & FD_WRITE)

#define FD_SIZE (sizeof(struct fd))

struct fd {
  int n;            /* virtual fd */
  int e;            /* effective fd */
  int mode;         /* FD_ modes */
  const char* name; /* name string */

  struct fd* next;
  struct fd** link;
  struct fd* parent;
  struct fd** pos;
  struct fd* dup;        /* this fd is a duplicate of ->dup */
  struct fdstack* stack; /* stack level */

  dev_t dev; /* device for terminal recognition */
  int fl;    /* posix fcntl() mode */

  buffer rb;
  buffer wb;
  buffer* r; /* pointer to effective read buffer */
  buffer* w; /* pointer to effective write buffer */
};

#define fd_ok(e) ((e) >= 0 && (e) < FD_MAX)

/* fd mode */
enum fd_mode {
  FD_READ = 0x0001,
  FD_WRITE = 0x0002,
  FD_APPEND = 0x0004,
  FD_EXCL = 0x0008,
  FD_TRUNC = 0x0010,

  /* types */

  FD_TYPE = 0x0007ff00,
  FD_FILE = 0x0100, /* a file that has been opened */
  FD_DIR = 0x0200,
  FD_LINK = 0x0400,
  FD_CHAR = 0x0800,
  FD_BLOCK = 0x1000,
  FD_SOCKET = 0x2000,
  FD_PIPE = 0x4000,       /* a pipe */
  FD_STRALLOC = 0x8000,   /* a stralloc */
  FD_STRING = 0x00010000, /* a nul-terminated string */
  FD_DUP = 0x00020000,    /* a clone of another file descriptor */
  FD_TERM = 0x00040000,   /* is a terminal */
  FD_NULL = 0x00080000,

  /* todo mode */

  FD_FLUSH = 0x01000000,    /* flush the buffer */
  FD_CLOSE = 0x02000000,    /* close the buffer */
  FD_FREENAME = 0x04000000, /* free the name */
  FD_DUPNAME = 0x08000000,  /* copy the name */
  FD_FREE = 0x10000000,     /* free the fd struct */
  FD_TMPBUF = 0x40000000,   /* buf alloca */
  FD_OPEN = 0x80000000
};

enum {
  FD_SUBST = (FD_STRALLOC | FD_WRITE),
  FD_HERE = (FD_STRALLOC | FD_READ),

  FD_READWRITE = (FD_READ | FD_WRITE),
  FD_DEFFILE = (FD_FILE | FD_CLOSE),
  FD_READFILE = (FD_DEFFILE | FD_READ),
  FD_WRITEFLUSH = (FD_FLUSH | FD_WRITE),
  FD_WRITEFILE = (FD_DEFFILE | FD_WRITEFLUSH),
  FD_RWFILE = (FD_READFILE | FD_WRITEFILE),
};

extern struct fd** const fdtable;

#define fd_foreach(i)                                                                                                                                          \
  for(i = fd_lo; i < fd_hi; i++)                                                                                                                               \
    if(fd_list[i])
#define fd_foreach_p(i, p)                                                                                                                                     \
  for(i = fd_lo; i < fd_hi; i++)                                                                                                                               \
    if((p = fd_list[i]))

/* current standard fds */
#define fd_src fdtable[STDSRC_FILENO]
#define fd_in fdtable[STDIN_FILENO]
#define fd_out fdtable[STDOUT_FILENO]
#define fd_err fdtable[STDERR_FILENO]

extern struct fd* fd_list[FD_MAX];
extern int fd_exp;
extern int fd_top;
extern int fd_lo;
extern int fd_hi;
extern struct fd fd_nullfd;

int fd_dup(struct fd* fd, int dfd);
int fd_error(int n, const char* msg);
int fd_exec(struct fd* fd);
int fd_getname(struct fd* fd);
int fd_mmap(struct fd* fd, const char* fname);
int fd_needbuf(struct fd* fd);
int fd_null(struct fd* fd);
int fd_pipe(struct fd* fd);
int fd_setfd(struct fd* fd, int e);
int fd_stat(struct fd* fd);
int fd_tempfile(struct fd* fd);
struct fd* fd_new(int fd, int mode);
struct fd* fd_push(struct fd* fd, int n, int mode);
struct fd* fd_reinit(struct fd* fd, int mode);
void fd_allocbuf(struct fd* fd, size_t n);
void fd_close(struct fd* fd);
void fd_dump(struct fd* fd, buffer* b);
void fd_dumplist(buffer* b);
void fd_free(struct fd* fd);
void fd_here(struct fd* fd, stralloc* sa);
void fd_init(struct fd* fd, int n, int mode);
void fd_open(struct fd* fd, const char* fname, long mode);
void fd_pop(struct fd* fd);
void fd_print(struct fd* fd, buffer* b);
void fd_setbuf(struct fd* fd, void* buf, size_t n);
void fd_string(struct fd* fd, const char* s, size_t len);
void fd_subst(struct fd* fd, stralloc* sa);

#define fd_malloc() ((struct fd*)shell_alloc(FD_SIZE))
#define fd_mallocb() ((struct fd*)shell_alloc(FD_SIZE + FD_BUFSIZE))
#define fd_alloca() ((struct fd*)alloca(FD_SIZE))
#define fd_allocab() ((struct fd*)alloca(FD_SIZE + FD_BUFSIZE))

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ALLOCA
#define fd_alloc fd_alloca
#define fd_allocb fd_allocab
#else
#define fd_alloc fd_malloc
#define fd_allocb fd_mallocb
#endif

#endif /* FD_H */
