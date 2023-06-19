/* this header file comes from libowfat,  http://www.fefe.de/libowfat/ */
#ifndef BUFFER_H
#define BUFFER_H

#include "typedefs.h"

typedef ssize_t(buffer_op_sys)(int fd, void* buf, size_t len);
typedef ssize_t(buffer_op_proto)(int fd, void* buf, size_t len, void* arg);
typedef ssize_t(buffer_op_fn)(/*int fd, void* buf, size_t len, void* arg*/);
typedef buffer_op_fn* buffer_op_ptr;

struct buffer;

typedef struct buffer {
  char* x;             /* actual buffer space */
  size_t p;            /* current position */
  size_t n;            /* current size of string in buffer */
  size_t a;            /* allocated buffer size */
  buffer_op_proto* op; /* use read(2) or write(2) */
  void* cookie; /* used internally by the to-stralloc buffers,  and for buffer
                   chaini(ng */
  void (*deinit)(struct buffer*); /* called to munmap/free cleanup,  with a
                              pointer to the buffer as argument */
  fd_t fd;                        /* passed as first argument to op */
} buffer;

#define BUFFER_INIT(op, fd, buf, len) \
  { (buf), 0, 0, (len), (buffer_op_proto*)(void*)(op), NULL, NULL, (fd) }
#define BUFFER_INIT_FREE(op, fd, buf, len) \
  { (buf), 0, 0, (len), (buffer_op_proto*)(void*)(op), NULL, buffer_free, (fd) }
#define BUFFER_INIT_READ(op, fd, buf, len) \
  BUFFER_INIT(op, fd, buf, len) /*obsolete*/
#define BUFFER_INSIZE 65535
#define BUFFER_OUTSIZE 32768

void buffer_init(buffer*, buffer_op_proto*, int fd, char* y, size_t ylen);
void buffer_free(buffer* buf);
void buffer_munmap(buffer* buf);
int buffer_mmapread(buffer*, const char* filename);
int buffer_mmapread_fd(buffer*, int fd);
void buffer_close(buffer* b);

/* reading from an fd... if it is a regular file,  then  buffer_mmapread_fd is
   called, otherwise  buffer_init(&b,  read,  fd,  malloc(8192),  8192) */

int buffer_flush(buffer* b);
int buffer_put(buffer*, const char* x, size_t len);
ssize_t buffer_putflush(buffer*, const char* x, size_t len);
int buffer_puts(buffer*, const char* x);
ssize_t buffer_putsflush(buffer*, const char* x);

#if defined(__GNUC__) && !defined(__LIBOWFAT_INTERNAL) && \
    !defined(__dietlibc__) && !defined(NO_BUILTINS)
/* as a little gcc-specific hack,  if somebody calls buffer_puts with a
 * constant string,  where we know its length at compile-time,  call
 * buffer_put with the known length instead */
#define buffer_puts(b, s) \
  (__builtin_constant_p(s) ? buffer_put(b, s, __builtin_strlen(s)) \
                           : buffer_puts(b, s))
#define buffer_putsflush(b, s) \
  (__builtin_constant_p(s) ? buffer_putflush(b, s, __builtin_strlen(s)) \
                           : buffer_putsflush(b, s))
#endif

int buffer_putm_internal(buffer* b, ...);

#if defined(__BORLANDC__) || defined(__LCC__)
#else
#endif

int buffer_putspace(buffer* b);
ssize_t buffer_putnlflush(buffer* b); /* put \n and flush */

#define buffer_PUTC(s, c) \
  (((s)->a != (s)->p) ? ((s)->x[(s)->p++] = (c), 0) : buffer_putc((s), (c)))

ssize_t buffer_feed(buffer* b);
ssize_t buffer_getc(buffer*, char* x);

ssize_t buffer_get_token(
    buffer*, char* x, size_t len, const char* charset, size_t setlen);
ssize_t buffer_getline(buffer*, char* x, size_t len);

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
#define buffer_LEN(b) ((b)->n - (b)->p)
#define buffer_SEEK(s, len) ((s)->p += (len))

#define buffer_BEGIN(b) buffer_PEEK(b)
#define buffer_END(b) ((b)->x + (b)->n)

int buffer_putulong(buffer*, unsigned long int l);
int buffer_putlong(buffer*, signed long int l);

extern buffer* buffer_1;
extern buffer* buffer_2;

#ifdef STRALLOC_H
/* write stralloc to buffer */
int buffer_putsa(buffer*, const stralloc* sa);

typedef int (*sa_predicate)(stralloc* sa, void*);

/* like buffer_get_token_sa but the token ends when your predicate says so */
int buffer_get_token_sa_pred(buffer*, stralloc* sa, sa_predicate p, void*);
/* same,  but clear sa first */
int buffer_get_new_token_sa_pred(buffer*, stralloc* sa, sa_predicate p, void*);

/* make a buffer from a stralloc.
 * Do not change the stralloc after this! */
void buffer_fromsa(buffer*, const stralloc* sa); /* read from sa */
int buffer_tosa(buffer* b, stralloc* sa); /* write to sa,  auto-growing it */

#endif

void buffer_default(buffer* b, buffer_op_fn* op);
ssize_t buffer_dummyread_fromstr(int, void*, size_t, void*);
ssize_t buffer_dummyread(int, void*, size_t, void*);
ssize_t buffer_dummyreadmmap(int, void*, size_t, void*);
void buffer_dump(buffer* out, buffer* b);
void buffer_frombuf(buffer* b, const char* x, size_t l);
void buffer_fromstr(buffer* b, char* s, size_t len);
int buffer_get_until(
    buffer* b, char* x, size_t len, const char* charset, size_t setlen);
int buffer_putc(buffer* b, char c);

int buffer_putnspace(buffer* b, int n);
int buffer_putptr(buffer* b, void* ptr);
int buffer_putlong0(buffer* b, long l, int pad);
int buffer_putulong0(buffer* b, unsigned long l, int pad);

ssize_t buffer_stubborn(
    buffer_op_proto* op, int fd, const char* buf, size_t len, void* ptr);
ssize_t buffer_stubborn_read(
    buffer_op_proto* op, int fd, const void* buf, size_t len, void* ptr);
int buffer_truncfile(buffer* b, const char* fn);
int buffer_putnc(buffer*, char c, int ntimes);
int buffer_putns(buffer*, const char*, int ntimes);
int buffer_putspad(buffer*, const char* x, size_t pad);

#ifdef UINT64_H
int buffer_putlonglong(buffer*, int64);
int buffer_putulonglong(buffer*, uint64);
int buffer_putxlonglong0(buffer*, uint64, int pad);
int buffer_putxlonglong(buffer*, uint64);
#endif

#endif
