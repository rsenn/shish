#include "../source.h"
#include "../debug.h"
#include "../prompt.h"
#include "builtin_config.h"

struct source* source = 0;

/* gets more data from buffer (at least n + 1 chars)
 * doesn't advance buffer pointer, use input_skipcn() for that
 * ----------------------------------------------------------------------- */

int
source_peekn(char* c, unsigned n) {
  buffer* b;
  int ret;
  unsigned pi = 0, k = 0; /* physical index, logical index */
  int esc;                /* x[pi] is escaped by a backslash before it */

  /* used-up alias text: carry on with whatever follows the alias name */
#if BUILTIN_ALIAS
  while((source->mode & SOURCE_ALIAS) && source->parent && buffer_LEN(source->b) <= 0)
    source_alias_pop();
#endif

  b = source->b;
  ret = buffer_LEN(b);
  esc = source_bs;

  /* quoted/comment text has no continuations */
  if(source_squoted || source_comment) {
    if((unsigned)ret <= n && (ret = buffer_prefetch(b, n + 1)) <= 0)
      return ret;

    if(c)
      *c = b->x[b->p + n];

    return ret;
  }

  for(;;) {
    char* x;

    /* need the char at pi itself; the one after it (to tell "\\\n" apart
       from a lone "\\") is only fetched once x[pi] actually is a
       backslash -- fetching it unconditionally here forced a blocking
       read for one phantom byte past *every* character peeked, so an
       interactive line's own trailing newline (nothing typed after it
       yet) could never be classified without first blocking for more
       input (source-peekn-forces-extra-byte-for-continuation-check). */
    if((unsigned)ret <= pi && (ret = buffer_prefetch(b, pi + 1)) <= 0) {
      if(ret < 0 || (unsigned)buffer_LEN(b) <= pi)
        return ret;

      ret = buffer_LEN(b);
    }

    x = buffer_PEEK(b);

    if(!esc && x[pi] == '\\') {
      if((unsigned)ret <= pi + 1 && (ret = buffer_prefetch(b, pi + 2)) <= 0) {
        if(ret < 0)
          return ret;

        ret = buffer_LEN(b);
      }

      x = buffer_PEEK(b); /* buffer_prefetch may have moved/grown it */

      if((unsigned)buffer_LEN(b) > pi + 1 && x[pi + 1] == '\n') {
        /* a continuation at the very front is consumed for good */
        if(pi == 0) {
          b->p += 2;
          source_verbose('\\');
          source_verbose('\n');
          source_newline();
          ret = buffer_LEN(b);
        } else
          pi += 2;

        continue;
      }
    }

    if(k == n)
      break;

    esc = !esc && x[pi] == '\\';
    pi++;
    k++;
  }

  /* got data, peek the char */
  if(c)
    *c = buffer_PEEK(b)[pi];

  return ret;
}

int
source_peeknc(unsigned pos) {
  char c;

  if(source_peekn(&c, pos) <= 0)
    return -1;

  return (unsigned int)(unsigned char)c;
}
