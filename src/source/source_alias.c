#include "../fd.h"
#include "../source.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

int source_alias_blank;
const void* source_alias_popped[8];
unsigned long source_alias_poppedat[8];
int source_alias_npopped;
unsigned long source_tokskips;
int source_tokskips_set;

struct alias_frame {
  struct source src; /* first: a frame is freed through its source pointer */
  struct fd fd;
  char text[];
};

/* pushes alias text as the current input */
void
source_alias_push(const void* alias, const char* code, size_t n) {
  struct alias_frame* f = alloc(sizeof(struct alias_frame) + n + 1);

  byte_copy(f->text, n, code);
  f->text[n] = '\0'; /* source_peekn() looks one byte past a trailing backslash */
  source_buffer(&f->src, &f->fd, f->text, n);
  f->src.mode |= SOURCE_ALIAS;
  f->src.alias = alias;
  source_bs = 0;
}

/* pops the used-up alias frame on top; done from source_peekn() */
void
source_alias_pop(void) {
  struct source* s = source;
  const char* last = s->b->x + s->b->n;

  if(s->b->n > 0 && (last[-1] == ' ' || last[-1] == '\t'))
    source_alias_blank = 1;

  if(source_alias_npopped < 8) {
    source_alias_popped[source_alias_npopped] = s->alias;
    source_alias_poppedat[source_alias_npopped++] = source_skips;
  }

  source_bs = 0;
  s->parent->mode |= s->mode & SOURCE_HERE; /* a here-document body running out of the text goes on behind it */
  source_popfd(s->fd);
  alloc_free(s);
}

/* is this alias being read right now (or was, since the current token began)? */
int
source_alias_active(const void* alias) {
  struct source* s;
  int i;

  for(s = source; s; s = s->parent)
    if((s->mode & SOURCE_ALIAS) && s->alias == alias)
      return 1;

  /* popped before the word's first character was read = not part of this word */
  for(i = 0; i < source_alias_npopped; i++)
    if(source_alias_popped[i] == alias && source_alias_poppedat[i] > source_tokskips)
      return 1;

  return 0;
}

void
source_alias_reset(void) {
  source_alias_npopped = 0;
  source_tokskips_set = 0;
}
