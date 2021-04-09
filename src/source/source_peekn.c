#include "../source.h"

/* gets more data from buffer (at least n + 1 chars)
 * doesn't advance buffer pointer, use input_skipcn() for that
 * ----------------------------------------------------------------------- */
int
source_peekn(char* c, unsigned n) {
  buffer* b = source->b;
  int ret = buffer_LEN(b);
  unsigned lookahead = n;

  for(;;) {
    char* x;

    /* no data available, try to get some */
    if((unsigned)ret <= lookahead)
      if((ret = buffer_prefetch(b, lookahead + 1)) <= 0)
        return ret;
#ifdef DEBUG_OUTPUT_
    debug_ulong("source_peekn", ret);
#endif
    x = buffer_PEEK(b);

    if(x[0] == '\\') {
      if(lookahead == n) {
        lookahead++;
        continue;
      } else {
        if(x[1] == '\n') {
          b->p += 2;
          lookahead = n;
          continue;
        }
      }
    }
    break;
  }
  /* got data, peek the char */
  if(c) {

    *c = b->x[b->p + n];
  }

  return ret;
}

char
source_peeknc(unsigned pos) {
  return *source_peeks(pos);
}
