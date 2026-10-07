#include "../source.h"
#include "../sh.h"
#include "../fdtable.h"
#include "../../lib/stralloc.h"

/* "set -v": the line being read, written to stderr once its newline is consumed */
static stralloc source_verbose_line;

/* is the text being read echoed: the script, stdin or -c string itself, and what "." and eval read */
static int
source_verbose_on(void) {
  return sh->opts.verbose && (!source->parent || (source->mode & SOURCE_VERBOSE)) && !(source->mode & (SOURCE_ALIAS | SOURCE_HERE));
}

void
source_verbose(char c) {
  if(!source_verbose_on())
    return;

  stralloc_catc(&source_verbose_line, c);

  if(c == '\n') {
    buffer_put(fd_err->w, source_verbose_line.s, source_verbose_line.len);
    buffer_flush(fd_err->w);
    source_verbose_line.len = 0;
  }
}

/* the last line has no newline: echo it now, with one */
void
source_verbose_flush(void) {
  if(source_verbose_line.len && source_verbose_on()) {
    stralloc_catc(&source_verbose_line, '\n');
    buffer_put(fd_err->w, source_verbose_line.s, source_verbose_line.len);
    buffer_flush(fd_err->w);
    source_verbose_line.len = 0;
  }
}

int source_squoted = 0;
int source_comment = 0;
int source_bs = 0;
unsigned long source_skips = 0;
struct alias_scan alias_scan;

/* ----------------------------------------------------------------------- */
int
source_skip(void) {
  buffer* b;
  char c;

  /* a continuation at the front is not a character to skip; used-up
     alias text is popped there too, which changes source->b */
  source_peekn(0, 0);
  b = source->b;

  if(b->p < b->n) {
    c = b->x[b->p];

#ifdef DEBUG_OUTPUT_
    debug_char("source_skip", c);
#endif

    b->p++;
    source_skips++;
    source_verbose(c);

    if(c == '\\' && !source_bs && !source_squoted && !source_comment) {
      source_bs = 1;

      /* the peek may pop used-up alias text: the newline then belongs to
         whatever follows the alias name, not to the buffer b was taken from */
      if(source_peek(&c) > 0 && c == '\n') {
        source->b->p++;
        source_verbose(c);
        source_bs = 0;
      } else {
        c = '\\';
      }
    } else {
      source_bs = 0;
    }

    if(c == '\n') {
      source_newline();
    } else {
      source->position.column++;
      source->position.offset++;
    }

    return 1;
  }

  return 0;
}

/* ----------------------------------------------------------------------- */
int
source_skipn(int n) {
  while(n-- >= 0)
    if(!source_skip())
      break;

  return n;
}
