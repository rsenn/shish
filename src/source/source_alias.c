#include "../fd.h"
#include "../parse.h"
#include "../source.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "builtin_config.h"

#if BUILTIN_ALIAS
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
  struct alias_popped* pp = alloc(sizeof(struct alias_popped));

  if(s->b->n > 0 && (last[-1] == ' ' || last[-1] == '\t'))
    alias_scan.blank = 1;

  pp->alias = s->alias;
  pp->at = source_skips;
  pp->next = alias_scan.popped;
  alias_scan.popped = pp;

  source_bs = 0;
  s->parent->mode |= s->mode & SOURCE_HERE; /* a here-document body running out of the text goes on behind it */
  source_popfd(s->fd);
  alloc_free(s);
}

/* is this alias being read right now (or was, since the current token began)? */
int
source_alias_active(const void* alias) {
  struct source* s;
  struct alias_popped* pp;

  for(s = source; s; s = s->parent)
    if((s->mode & SOURCE_ALIAS) && s->alias == alias)
      return 1;

  /* popped before the word's first character was read = not part of this word */
  for(pp = alias_scan.popped; pp; pp = pp->next)
    if(pp->alias == alias && pp->at > alias_scan.tokskips)
      return 1;

  return 0;
}

void
source_alias_reset(void) {
  struct alias_popped* pp;

  while((pp = alias_scan.popped)) {
    alias_scan.popped = pp->next;
    alloc_free(pp);
  }

  alias_scan.tokskips_set = 0;
}

/* a subshell works on a copy of the alias list; restore drops the copy
 * ----------------------------------------------------------------------- */
struct alias*
alias_scan_save(void) {
  struct alias *saved = alias_scan.list, **tail = &alias_scan.list, *a;

  alias_scan.list = NULL;

  for(a = saved; a; a = a->next) {
    size_t n = sizeof(struct alias) + a->namelen + 1 + a->codelen + 1;
    struct alias* c = alloc(n);

    byte_copy(c, n, a);
    c->next = NULL;
    *tail = c;
    tail = &c->next;
  }

  return saved;
}

void
alias_scan_restore(struct alias* saved) {
  struct alias* a;

  while((a = alias_scan.list)) {
    alias_scan.list = a->next;
    alloc_free(a);
  }

  alias_scan.list = saved;
}
#endif
