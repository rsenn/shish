/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef STRALLOC_H
#define STRALLOC_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __pure__
#define __pure__
#endif

/* stralloc is the internal data structure all functions are working on.
 * s is the string.
 * len is the used length of the string.
 * a is the allocated length of the string.
 */

typedef struct stralloc_s {
  char* s;
  size_t len;
  size_t a;
} stralloc;

/* stralloc_init will initialize a stralloc.
 * Previously allocated memory will not be freed; use stralloc_free for
 * that.  To assign an empty string, use stralloc_copys(sa, ""). */
void stralloc_init(stralloc* sa);

/* stralloc_ready makes sure that sa has enough space allocated to hold
 * len bytes: If sa is not allocated, stralloc_ready allocates at least
 * len bytes of space, and returns 1. If sa is already allocated, but
 * not enough to hold len bytes, stralloc_ready allocates at least len
 * bytes of space, copies the old string into the new space, frees the
 * old space, and returns 1. Note that this changes sa.s.  If the
 * allocation fails, stralloc_ready leaves sa alone and returns 0. */
int stralloc_ready(stralloc* sa, size_t len);

/* stralloc_readyplus is like stralloc_ready except that, if sa is
 * already allocated, stralloc_readyplus adds the current length of sa
 * to len. */
int stralloc_readyplus(stralloc* sa, size_t len);

/* stralloc_copyb copies the string buf[0], buf[1], ..., buf[len - 1] into
 * sa, allocating space if necessary, and returns 1. If it runs out of
 * memory, stralloc_copyb leaves sa alone and returns 0. */
int stralloc_copyb(stralloc* sa, const char* buf, size_t len);

/* stralloc_copys copies a \0 - terminated string from buf into sa,
 * without the \0. It is the same as
 * stralloc_copyb(&sa, buf, str_len(buf)). */
int stralloc_copys(stralloc* sa, const char* buf);

/* stralloc_copy copies the string stored in sa2 into sa. It is the same
 * as stralloc_copyb(&sa, sa2.s, sa2.len). sa2 must already be allocated. */
int stralloc_copy(stralloc* sa, const stralloc* sa2);

/* stralloc_catb adds the string buf[0], buf[1], ... buf[len - 1] to the
 * end of the string stored in sa, allocating space if necessary, and
 * returns 1. If sa is unallocated, stralloc_catb is the same as
 * stralloc_copyb. If it runs out of memory, stralloc_catb leaves sa
 * alone and returns 0. */
int stralloc_catb(stralloc* sa, const char* in, size_t len);

/* stralloc_cats is analogous to stralloc_copys */
int stralloc_cats(stralloc* sa, const char* in);
int stralloc_catc(stralloc* sa, const unsigned char c);

void stralloc_zero(stralloc* sa);
int stralloc_trunc(stralloc* sa, size_t n);

/* like stralloc_cats but can cat more than one string at once */
int stralloc_catm_internal(stralloc* sa, ...);

#if defined(__BORLANDC__) || defined(__LCC__)
#define stralloc_catm(sa, args) stralloc_catm_internal(sa, args, (char*)0)
#define stralloc_copym(sa, args) (stralloc_zero(sa), stralloc_catm_internal(sa, args, (char*)0))
#else
#define stralloc_catm(sa, ...) stralloc_catm_internal(sa, __VA_ARGS__, (char*)0)
#define stralloc_copym(sa, ...) (stralloc_zero(sa), stralloc_catm_internal(sa, __VA_ARGS__, (char*)0))
#endif

/* stralloc_cat is analogous to stralloc_copy */
int stralloc_cat(stralloc* sa, const stralloc* in);

/* stralloc_append adds one byte in[0] to the end of the string stored
 * in sa. It is the same as stralloc_catb(&sa, in, 1). */
int stralloc_append(stralloc* sa, const char* in); /* beware: this takes a pointer to 1 char */

/* stralloc_starts returns 1 if the \0 - terminated string in "in", without
 * the terminating \0, is a prefix of the string stored in sa. Otherwise
 * it returns 0. sa must already be allocated. */
int stralloc_starts(stralloc* sa, const char* in) __pure__;

/* stralloc_diffs returns negative, 0, or positive, depending on whether
 * a is lexicographically smaller than, equal to, or greater than the
 * string b[0], b[1], ..., b[n] == '\0'. */
int stralloc_diffs(const stralloc* a, const char* b) __pure__;

#define stralloc_equals(a, b) (!stralloc_diffs((a), (b)))

/* stralloc_0 appends \0 */
#define stralloc_0(sa) stralloc_append(sa, "")

int stralloc_nul(stralloc* sa);

/* stralloc_catulong0 appends a '0' padded ASCII representation of in */
size_t stralloc_catulong(stralloc* sa, unsigned long int ul);
int stralloc_catulong0(stralloc* sa, unsigned long int in, size_t n);

/* stralloc_catlong0 appends a '0' padded ASCII representation of in */
int stralloc_catlong0(stralloc* sa, signed long int in, size_t n);

/* stralloc_free frees the storage associated with sa */
void stralloc_free(stralloc* sa);

#define stralloc_catlong(sa, l) (stralloc_catlong0((sa), (l), 0))
#define stralloc_catuint0(sa, i, n) (stralloc_catulong0((sa), (i), (n)))
#define stralloc_catint0(sa, i, n) (stralloc_catlong0((sa), (i), (n)))
#define stralloc_catint(sa, i) (stralloc_catlong0((sa), (i), 0))

/* remove trailing "\r\n", "\n" or "\r".  Return number of removed chars (0, 1 or 2)
 */

void stralloc_trimr(stralloc* sa, const char* trimchars, unsigned int trimcharslen);

#ifdef BUFFER_H
/* write stralloc to buffer */
int buffer_putsa(buffer* b, const stralloc* sa);
/* write stralloc to buffer and flush */
int buffer_putsaflush(buffer* b, const stralloc* sa);

/* these "read token" functions return 0 if the token was complete or
 * EOF was hit or -1 on error.  In contrast to the non - stralloc token
 * functions, the separator is also put in the stralloc; use
 * stralloc_chop or stralloc_chomp to get rid of it. */

/* WARNING!  These token reading functions will not clear the stralloc!
 * They _append_ the token to the contents of the stralloc.  The idea is
 * that this way these functions can be used on non - blocking sockets;
 * when you get signalled EAGAIN, just call the functions again when new
 * data is available. */

/* read token from buffer to stralloc */
int buffer_get_token_sa(buffer* b, stralloc* sa, const char* charset, size_t setlen);
/* read line from buffer to stralloc */
int buffer_getline_sa(buffer* b, stralloc* sa);

/* same as buffer_get_token_sa but empty sa first */
int buffer_get_new_token_sa(buffer* b, stralloc* sa, const char* charset, size_t setlen);
/* same as buffer_getline_sa but empty sa first */
int buffer_getnewline_sa(buffer* b, stralloc* sa);

typedef int (*sa_predicate)(stralloc* sa, void*);

/* like buffer_get_token_sa but the token ends when your predicate says so */
int buffer_get_token_sa_pred(buffer* b, stralloc* sa, sa_predicate p, void*);
/* same, but clear sa first */
int buffer_get_new_token_sa_pred(buffer* b, stralloc* sa, sa_predicate p, void*);

/* make a buffer from a stralloc.
 * Do not change the stralloc after this! */
void buffer_fromsa(buffer* b, const stralloc* sa);

int stralloc_write(int, const char*, size_t, buffer*);
#endif

#ifdef __BORLANDC__
#define stralloc_length(sa) (sa)->len
#else
inline static size_t
stralloc_length(const stralloc* sa) {
  return sa->len;
}
#endif

#define stralloc_begin(sa) ((sa)->s)
#define stralloc_end(sa) ((sa)->s + (sa)->len)
#define stralloc_iterator_decrement(it) (--(it))
#define stralloc_iterator_dereference(it_ptr) (*(*(it_ptr)))
#define stralloc_iterator_distance(it1, it2) ((it2) - (it1))
#define stralloc_is_last(sa, ptr) ((sa)->len > 0 && ((sa)->s + (sa)->len - 1) == (ptr))

#ifdef BYTE_H
size_t byte_scan(const char* in, size_t in_len, stralloc* out, size_t (*scan_function)(const char*, char*));
#endif

int stralloc_insertb(stralloc* sa, const char* s, size_t pos, size_t n);

typedef size_t(stralloc_fmt_fn)(char*, unsigned int);

int stralloc_prepends(stralloc* sa, const char* s);

size_t stralloc_remove(stralloc*, size_t pos, size_t n);
void stralloc_replacec(stralloc*, char before, char after);

static inline char*
stralloc_take(stralloc* from, size_t* lenp, size_t* allocated) {
  char* s = from->s;
  if(lenp)
    *lenp = from->len;
  if(allocated)
    *allocated = from->a;
  stralloc_init(from);
  return s;
}
void stralloc_move(stralloc*, stralloc* from);

#ifdef DEBUG_ALLOC
void stralloc_freedebug(const char* file, unsigned int line, stralloc* sa);
int stralloc_readydebug(const char* file, unsigned int line, stralloc* sa, unsigned long int len);
int stralloc_readyplusdebug(const char* file, unsigned int line, stralloc* sa, unsigned long len);
int stralloc_truncdebug(const char* file, unsigned int line, stralloc* sa, unsigned long int n);
#endif

#ifdef BUFFER_H
void stralloc_dump(const stralloc*, buffer*);
#endif

#ifdef __cplusplus
}
#endif

#endif
