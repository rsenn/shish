/* this header file comes from libowfat,  http://www.fefe.de/libowfat/ */
#ifndef BUFFER_H
#define BUFFER_H

#include "typedefs.h"


typedef ssize_t(buffer_op_sys)(fd_t fd, void* buf, size_t len);
typedef ssize_t(buffer_op_proto)(fd_t fd, void* buf, size_t len, void* arg);
typedef ssize_t(buffer_op_fn)(/*fd_t fd, void* buf, size_t len, void* arg*/);
typedef buffer_op_fn* buffer_op_ptr;

typedef struct buffer {
  char* x;             /* actual buffer space */
  size_t p;            /* current position */
  size_t n;            /* current size of string in buffer */
  size_t a;            /* allocated buffer size */
  buffer_op_proto* op; /* use read(2) or write(2) */
  void* cookie;        /* used internally by the to-stralloc buffers,  and for buffer chaini(ng */
  void (*deinit)();    /* called to munmap/free cleanup,  with a pointer to the buffer as argument */
  fd_t fd;             /* passed as first argument to op */
} buffer;

#define BUFFER_INIT(op, fd, buf, len)                                                                                  \
  { (buf), 0, 0, (len), (buffer_op_proto*)(void*)(op), NULL, NULL, (fd) }
#define BUFFER_INIT_FREE(op, fd, buf, len)                                                                             \
  { (buf), 0, 0, (len), (buffer_op_proto*)(void*)(op), NULL, buffer_free, (fd) }
#define BUFFER_INIT_READ(op, fd, buf, len) BUFFER_INIT(op, fd, buf, len) /*obsolete*/
#define BUFFER_INSIZE 8192
#define BUFFER_OUTSIZE 8192

void buffer_init(buffer*, buffer_op_proto*, fd_t fd, char* y, size_t ylen);
void buffer_free(void* buf);
void buffer_munmap(void* buf);
int buffer_mmapread(buffer*, const char* filename);
int buffer_mmapread_fd(buffer*, fd_t fd);
int buffer_mmapprivate(buffer*, const char* filename);
void buffer_close(buffer* b);

/* reading from an fd... if it is a regular file,  then  buffer_mmapread_fd is called,
   otherwise  buffer_init(&b,  read,  fd,  malloc(8192),  8192) */

int buffer_flush(buffer* b);
int buffer_put(buffer*, const char* x, size_t len);
ssize_t buffer_putflush(buffer*, const char* x, size_t len);
int buffer_puts(buffer*, const char* x);
ssize_t buffer_putsflush(buffer*, const char* x);

#if defined(__GNUC__) && !defined(__LIBOWFAT_INTERNAL) && !defined(__dietlibc__)
/* as a little gcc-specific hack,  if somebody calls buffer_puts with a
 * constant string,  where we know its length at compile-time,  call
 * buffer_put with the known length instead */
#define buffer_puts(b, s) (__builtin_constant_p(s) ? buffer_put(b, s, __builtin_strlen(s)) : buffer_puts(b, s))
#define buffer_putsflush(b, s)                                                                                         \
  (__builtin_constant_p(s) ? buffer_putflush(b, s, __builtin_strlen(s)) : buffer_putsflush(b, s))
#endif

int buffer_putm_internal(buffer* b, ...);

#ifdef __BORLANDC__
#else
#endif

int buffer_putspace(buffer* b);
ssize_t buffer_putnlflush(buffer* b); /* put \n and flush */

ssize_t buffer_get(buffer*, char* x, size_t len);
ssize_t buffer_feed(buffer* b);
ssize_t buffer_getc(buffer*, char* x);

/* read bytes until the destination buffer is full (len bytes),  end of
 * file is reached or the read char is in charset (setlen bytes).  An
 * empty line when looking for \n will write '\n' to x and return 0.  If
 * EOF is reached,  \0 is written to the buffer */
int buffer_skip_until(buffer*, const char* charset, size_t setlen);

/* this predicate is given the string as currently read from the buffer
 * and is supposed to return 1 if the token is complete,  0 if not. */
typedef int (*string_predicate)(const char* x, size_t len, void* arg);

/* like buffer_get_token but the token ends when your predicate says so */

int buffer_prefetch(buffer*, size_t n);

#define buffer_PEEK(s) ((s)->x + (s)->p)
#define buffer_SEEK(s, len) ((s)->p += (len))

#define buffer_GETC(s, c)                                                                                              \
  (((s)->p < (s)->n) ? (*(c) = *buffer_PEEK(s), buffer_SEEK((s), 1), 1) : buffer_get((s), (c), 1))

int buffer_putulong(buffer*, unsigned long int l);
int buffer_putxlong(buffer*, unsigned long int l);

extern buffer* buffer_0;
extern buffer* buffer_0small;
extern buffer* buffer_1;
extern buffer* buffer_1small;
extern buffer* buffer_2;

#ifdef STRALLOC_H
/* write stralloc to buffer */
int buffer_putsa(buffer*, const stralloc* sa);
/* write stralloc to buffer and flush */

/* these "read token" functions return 0 if the token was complete or
 * EOF was hit or -1 on error.  In contrast to the non-stralloc token
 * functions,  the separator is also put in the stralloc; use
 * stralloc_chop or stralloc_chomp to get rid of it. */

/* WARNING!  These token reading functions will not clear the stralloc!
 * They _append_ the token to the contents of the stralloc.  The idea is
 * that this way these functions can be used on non-blocking sockets;
 * when you get signalled EAGAIN,  just call the functions again when new
 * data is available. */

/* read token from buffer to stralloc */
/* read line from buffer to stralloc */

/* same as buffer_get_token_sa but empty sa first */
/* same as buffer_getline_sa but empty sa first */

typedef int (*sa_predicate)(stralloc* sa, void*);

/* like buffer_get_token_sa but the token ends when your predicate says so */
/* same,  but clear sa first */

/* make a buffer from a stralloc.
 * Do not change the stralloc after this! */
void buffer_fromsa(buffer*, const stralloc* sa); /* read from sa */
int buffer_tosa(buffer* b, stralloc* sa);        /* write to sa,  auto-growing it */

#endif

void buffer_frombuf(buffer*, const char* x, size_t l); /* buffer reads from static buffer */

#ifdef ARRAY_H
#endif
void buffer_dump(buffer* out, buffer* b);

int buffer_putc(buffer*, char c);
int buffer_putnspace(buffer*, int n);

int buffer_putptr(buffer*, void* ptr);
int buffer_putulong0(buffer*, unsigned long l, int pad);
int buffer_putxlong0(buffer*, unsigned long l, int pad);

int buffer_put_escaped(buffer*, const char* x, size_t len);
int buffer_truncfile(buffer* b, const char* fn);

int buffer_putnc(buffer*, char c, int ntimes);
int buffer_get_until(buffer*, char* x, size_t len, const char* charset, size_t setlen);

#ifdef UINT64_H
int buffer_putlonglong(buffer*, int64 l);
int buffer_putulonglong(buffer*, uint64 l);
int buffer_putxlonglong(buffer*, uint64 l);
int buffer_putulonglong(buffer*, uint64 i);
int buffer_putlonglong(buffer*, int64 i);
int buffer_putxlonglong0(buffer*, uint64 l, int pad);
#endif

#ifdef TAI_H
#endif

void buffer_default(buffer*, buffer_op_fn* op);
ssize_t buffer_dummyread_fromstr(void);
void buffer_fromstr(buffer*, char* s, size_t len);


#endif
