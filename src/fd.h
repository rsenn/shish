#ifndef FD_H
#define FD_H

/*#define __USE_LARGEFILE64*/
#include <fcntl.h>

#include "buffer.h"
#include "shell.h"
#include "stralloc.h"
#include <sys/stat.h>

#ifdef D_SETSIZE
#define D_MAX D_SETSIZE
#else
#define D_MAX 1024
#endif

#define D_BUFSIZE 1024
#define D_BUFSIZE2 (D_BUFSIZE >> 1)

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

#define D_ISRD(i) ((i)->mode & D_READ)
#define D_ISWR(i) ((i)->mode & D_WRITE)

#define D_SIZE (sizeof(struct fd))

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
  int fl;    /* posix fcntl() flags */

  buffer rb;
  buffer wb;
  buffer* r; /* pointer to effective read buffer */
  buffer* w; /* pointer to effective write buffer */
};

#define fd_ok(e) ((e) >= 0 && (e) < D_MAX)

/* fd mode */
#define D_READ 0x00000001
#define D_WRITE 0x00000002
#define D_APPEND 0x00000004
#define D_EXCL 0x00000008
#define D_TRUNC 0x00000010

/* types */
#define D_TYPE 0x0007ff00

#define D_FILE 0x00000100 /* a file that has been opened */
#define D_DIR 0x00000200
#define D_LINK 0x00000400
#define D_CHAR 0x00000800
#define D_BLOCK 0x00001000
#define D_SOCKET 0x00002000
#define D_PIPE 0x00004000     /* a pipe */
#define D_STRALLOC 0x00008000 /* a stralloc */
#define D_STRING 0x00010000   /* a nul-terminated string */
#define D_DUP 0x00020000      /* a clone of another file descriptor */
#define D_TERM 0x00040000     /* is a terminal */
#define D_NULL 0x00080000

#define D_SUBST (D_STRALLOC | D_WRITE)
#define D_HERE (D_STRALLOC | D_READ)

#define D_READWRITE (D_READ | D_WRITE)
#define D_DEFFILE (D_FILE | D_CLOSE)
#define D_READFILE (D_DEFFILE | D_READ)
#define D_WRITEFLUSH (D_FLUSH | D_WRITE)
#define D_WRITEFILE (D_DEFFILE | D_WRITEFLUSH)
#define D_RWFILE (D_READFILE | D_WRITEFILE)

/* todo mode */
#define D_FLUSH 0x01000000    /* flush the buffer */
#define D_CLOSE 0x02000000    /* close the buffer */
#define D_FREENAME 0x04000000 /* free the name */
#define D_DUPNAME 0x08000000  /* copy the name */
#define D_FREE 0x10000000     /* free the fd struct */
#define D_TMPBUF 0x40000000   /* buf alloca */
#define D_OPEN 0x80000000

extern struct fd** const fdtable;

#define fd_foreach(i)                                                                                                  \
  for(i = fd_lo; i < fd_hi; i++)                                                                                       \
    if(fd_list[i])
#define fd_foreach_p(i, p)                                                                                             \
  for(i = fd_lo; i < fd_hi; i++)                                                                                       \
    if((p = fd_list[i]))

/* current standard fds */
#define fd_src fdtable[STDSRC_FILENO]
#define fd_in fdtable[STDIN_FILENO]
#define fd_out fdtable[STDOUT_FILENO]
#define fd_err fdtable[STDERR_FILENO]

extern struct fd* fd_list[D_MAX];
extern unsigned int fd_exp;
extern unsigned int fd_top;
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
struct fd* fd_reinit(struct fd* fd, int flags);
void fd_allocbuf(struct fd* fd, unsigned long n);
void fd_close(struct fd* fd);
void fd_dump(struct fd* fd);
void fd_dumplist(void);
void fd_free(struct fd* fd);
void fd_here(struct fd* fd, stralloc* sa);
void fd_init(struct fd* fd, int n, int flags);
void fd_open(struct fd* fd, const char* fname, long mode);
void fd_pop(struct fd* fd);
void fd_print(struct fd* fd);
void fd_setbuf(struct fd* fd, void* buf, unsigned long n);
void fd_string(struct fd* fd, const char* s, unsigned long len);
void fd_subst(struct fd* fd, stralloc* sa);

#define fd_malloc(io) io = shell_alloc(D_SIZE);
#define fd_mallocb(io) io = shell_alloc(D_SIZE + D_BUFSIZE);
#define fd_alloca(io) io = alloca(D_SIZE);
#define fd_allocab(io) io = alloca(D_SIZE + D_BUFSIZE);

#endif /* FD_H */
