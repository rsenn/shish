/* this header file comes from libowfat,  http://www.fefe.de/libowfat/ */
#ifndef BUFFER_H
#define BUFFER_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#define BUFFER_INSIZE 65535
#define BUFFER_OUTSIZE 32768

void buffer_init(buffer*, buffer_op_proto*, fd_t fd, char* y, size_t ylen);
void buffer_init_free(buffer*, buffer_op_proto*, fd_t fd, char* y, size_t ylen);
void buffer_free(void* buf);
void buffer_munmap(void* buf);
int buffer_mmapread(buffer*, const char* filename);
int buffer_mmapread_fd(buffer*, fd_t fd);
int buffer_mmapprivate(buffer*, const char* filename);
int buffer_mmapprivate_fd(buffer*, fd_t fd);
int buffer_mmapshared(buffer*, const char* filename);
int buffer_mmapshared_fd(buffer*, fd_t fd);
void buffer_close(buffer* b);

/* reading from an fd... if it is a regular file,  then  buffer_mmapread_fd is called,
   otherwise  buffer_init(&b,  read,  fd,  malloc(8192),  8192) */
int buffer_read_fd(buffer*, fd_t fd);

int buffer_flush(buffer* b);
int buffer_put(buffer*, const char* x, size_t len);
int buffer_putalign(buffer*, const char* x, size_t len);
ssize_t buffer_putflush(buffer*, const char* x, size_t len);
int buffer_puts(buffer*, const char* x);
int buffer_putsalign(buffer*, const char* x);
ssize_t buffer_putsflush(buffer*, const char* x);

#if defined(__GNUC__) && !defined(__LIBOWFAT_INTERNAL) && !defined(__dietlibc__) && !defined(NO_BUILTINS)
/* as a little gcc-specific hack,  if somebody calls buffer_puts with a
 * constant string,  where we know its length at compile-time,  call
 * buffer_put with the known length instead */
#define buffer_puts(b, s) (__builtin_constant_p(s) ? buffer_put(b, s, __builtin_strlen(s)) : buffer_puts(b, s))
#define buffer_putsflush(b, s)                                                                                         \
  (__builtin_constant_p(s) ? buffer_putflush(b, s, __builtin_strlen(s)) : buffer_putsflush(b, s))
#endif

int buffer_putm_internal(buffer* b, ...);
int buffer_putm_internal_flush(buffer* b, ...);

#ifdef __BORLANDC__
#define buffer_putm(b, args) buffer_putm_internal(b, args, (char*)0)
#define buffer_putmflush(b, args) buffer_putm_internal_flush(b, args, (char*)0)
#else
#define buffer_putm(...) buffer_putm_internal(__VA_ARGS__, (char*)0)
#define buffer_putmflush(b, ...) buffer_putm_internal_flush(b, __VA_ARGS__, (char*)0)
#endif
#define buffer_putm_2(b, a1, a2) buffer_putm_internal(b, a1, a2, (char*)0)
#define buffer_putm_3(b, a1, a2, a3) buffer_putm_internal(b, a1, a2, a3, (char*)0)
#define buffer_putm_4(b, a1, a2, a3, a4) buffer_putm_internal(b, a1, a2, a3, a4, (char*)0)
#define buffer_putm_5(b, a1, a2, a3, a4, a5) buffer_putm_internal(b, a1, a2, a3, a4, a5, (char*)0)
#define buffer_putm_6(b, a1, a2, a3, a4, a5, a6) buffer_putm_internal(b, a1, a2, a3, a4, a5, a6, (char*)0)
#define buffer_putm_7(b, a1, a2, a3, a4, a5, a6, a7) buffer_putm_internal(b, a1, a2, a3, a4, a5, a6, a7, (char*)0)

int buffer_putspace(buffer* b);
ssize_t buffer_putnlflush(buffer* b); /* put \n and flush */

#define buffer_PUTC(s, c) (((s)->a != (s)->p) ? ((s)->x[(s)->p++] = (c), 0) : buffer_putc((s), (c)))

ssize_t buffer_get(buffer*, char* x, size_t len);
ssize_t buffer_feed(buffer* b);
ssize_t buffer_getc(buffer*, char* x);
ssize_t buffer_getn(buffer*, char* x, size_t len);

/* read bytes until the destination buffer is full (len bytes),  end of
 * file is reached or the read char is in charset (setlen bytes).  An
 * empty line when looking for \n will write '\n' to x and return 0.  If
 * EOF is reached,  \0 is written to the buffer */
ssize_t buffer_get_token(buffer*, char* x, size_t len, const char* charset, size_t setlen);
ssize_t buffer_getline(buffer*, char* x, size_t len);
int buffer_skip_until(buffer*, const char* charset, size_t setlen);

/* this predicate is given the string as currently read from the buffer
 * and is supposed to return 1 if the token is complete,  0 if not. */
typedef int (*string_predicate)(const char* x, size_t len, void* arg);

/* like buffer_get_token but the token ends when your predicate says so */
ssize_t buffer_get_token_pred(buffer*, char* x, size_t len, string_predicate p, void*);

char* buffer_peek(buffer* b);
int buffer_peekc(buffer*, char* c);
void buffer_seek(buffer*, size_t len);

int buffer_skipc(buffer* b);
int buffer_skipn(buffer*, size_t n);

int buffer_prefetch(buffer*, size_t n);

#define buffer_PEEK(s) ((s)->x + (s)->p)
#define buffer_SEEK(s, len) ((s)->p += (len))

#define buffer_GETC(s, c)                                                                                              \
  (((s)->p < (s)->n) ? (*(c) = *buffer_PEEK(s), buffer_SEEK((s), 1), 1) : buffer_get((s), (c), 1))

int buffer_putulong(buffer*, unsigned long int l);
int buffer_put8long(buffer*, unsigned long int l);
int buffer_putxlong(buffer*, unsigned long int l);
int buffer_putlong(buffer*, signed long int l);

int buffer_putdouble(buffer*, double d, int prec);

int buffer_puterror(buffer* b);
int buffer_puterror2(buffer*, int errnum);

extern buffer* buffer_0;
extern buffer* buffer_0small;
extern buffer* buffer_1;
extern buffer* buffer_1small;
extern buffer* buffer_2;

#ifdef STRALLOC_H
/* write stralloc to buffer */
int buffer_putsa(buffer*, const stralloc* sa);
/* write stralloc to buffer and flush */
int buffer_putsaflush(buffer*, const stralloc* sa);

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
int buffer_get_token_sa(buffer*, stralloc* sa, const char* charset, size_t setlen);
/* read line from buffer to stralloc */
int buffer_getline_sa(buffer*, stralloc* sa);

/* same as buffer_get_token_sa but empty sa first */
int buffer_get_new_token_sa(buffer*, stralloc* sa, const char* charset, size_t setlen);
/* same as buffer_getline_sa but empty sa first */
int buffer_getnewline_sa(buffer*, stralloc* sa);

typedef int (*sa_predicate)(stralloc* sa, void*);

/* like buffer_get_token_sa but the token ends when your predicate says so */
int buffer_get_token_sa_pred(buffer*, stralloc* sa, sa_predicate p, void*);
/* same,  but clear sa first */
int buffer_get_new_token_sa_pred(buffer*, stralloc* sa, sa_predicate p, void*);

/* make a buffer from a stralloc.
 * Do not change the stralloc after this! */
void buffer_fromsa(buffer*, const stralloc* sa); /* read from sa */
int buffer_tosa(buffer* b, stralloc* sa);        /* write to sa,  auto-growing it */

int buffer_gettok_sa(buffer*, stralloc* sa, const char* charset, size_t setlen);
#endif

void buffer_frombuf(buffer*, const char* x, size_t l); /* buffer reads from static buffer */

#ifdef ARRAY_H
void buffer_fromarray(buffer*, array* a); /* buffer reads from array */
#endif
void buffer_dump(buffer* out, buffer* b);

int buffer_putc(buffer*, char c);
int buffer_putnspace(buffer*, int n);

int buffer_putptr(buffer*, void* ptr);
int buffer_putulong0(buffer*, unsigned long l, int pad);
int buffer_putlong0(buffer*, long l, int pad);
int buffer_putxlong0(buffer*, unsigned long l, int pad);

int buffer_skipspace(buffer* b);
int buffer_skip_pred(buffer*, int (*pred)(int));

int buffer_put_escaped(buffer*, const char* x, size_t len);
int buffer_puts_escaped(buffer*, const char* x);

int buffer_freshen(buffer* b);

int buffer_truncfile(buffer*, const char* fn);

int buffer_lzma(buffer*, buffer*, int compress);
int buffer_bz2(buffer*, buffer*, int compress);

int buffer_putnc(buffer*, char c, int ntimes);
int buffer_putns(buffer*, const char* s, int ntimes);

int buffer_putspad(buffer*, const char* x, size_t pad);

int buffer_deflate(buffer*, buffer* out, int level);
int buffer_inflate(buffer*, buffer* in);

int buffer_gunzip(buffer*, const char* filename);
int buffer_gunzip_fd(buffer*, fd_t fd);
int buffer_gzip(buffer*, const char* filename, int level);
int buffer_gzip_fd(buffer*, fd_t fd, int level);
int buffer_bunzip(buffer*, const char* filename);
int buffer_bunzip_fd(buffer*, fd_t fd);
int buffer_bzip(buffer*, const char* filename, int level);
int buffer_bzip_fd(buffer*, fd_t fd, int level);

int buffer_get_until(buffer*, char* x, size_t len, const char* charset, size_t setlen);

int buffer_write_fd(buffer*, fd_t fd);

#ifdef UINT64_H
int buffer_putlonglong(buffer*, int64 l);
int buffer_putulonglong(buffer*, uint64 l);
int buffer_putxlonglong(buffer*, uint64 l);
int buffer_putulonglong(buffer*, uint64 i);
int buffer_putlonglong(buffer*, int64 i);
int buffer_putxlonglong0(buffer*, uint64 l, int pad);
#endif

#ifdef TAI_H
int buffer_puttai(buffer*, const struct tai*);
#endif

#ifdef __cplusplus
}
#endif

#endif
