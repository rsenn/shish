/* awk_compile()/awk_free()/awk_error(): ties the lexer and parser
 * together into text/awk.h's public entry point. */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"

struct awk_func*
awk_func_find(struct awk_prog* prog, const char* name, size_t len) {
  size_t i;

  for(i = 0; i < prog->nfuncs; i++)
    if(str_len(prog->funcs[i].name) == len && !byte_diff(prog->funcs[i].name, len, name))
      return &prog->funcs[i];

  return NULL;
}

const char*
awk_error(int code) {
  switch(code) {
    case AWK_OK: return "no error";
    case AWK_ENOMEM: return "out of memory";
    case AWK_ESYNTAX: return "syntax error";
    case AWK_EREGEX: return "invalid regular expression";
    default: return "unknown error";
  }
}

int
awk_compile(struct awk_prog** out, const char* src, size_t len, unsigned long* errline) {
  struct awk_prog* prog;
  struct awk_parser p;
  char* copy;
  int rc;

  *out = NULL;

  prog = alloc_zero(sizeof(*prog));
  arena_init(&prog->a, &arena_heap, 0);

  /* the lexer/strtod need a NUL-terminated buffer to scan safely past
     the last token; awk_compile()'s own contract allows non-terminated
     input, so make an owned copy once here. */
  copy = arena_alloc(&prog->a, len + 1, 1);

  if(!copy) {
    arena_free(&prog->a);
    alloc_free(prog);
    return AWK_ENOMEM;
  }

  byte_copy(copy, len, src);
  copy[len] = 0;

  byte_zero(&p, sizeof(p));
  awk_lex_init(&p.lx, copy, len);
  p.a = &prog->a;
  p.prog = prog;

  rc = awk_parse_program(&p);

  if(rc != AWK_OK || p.err) {
    if(errline)
      *errline = p.errline ? p.errline : p.lx.line;

    awk_free(prog);
    return p.err ? p.err : rc;
  }

  *out = prog;
  return AWK_OK;
}

void
awk_free(struct awk_prog* prog) {
  size_t i;

  if(!prog)
    return;

  /* struct dfa's own buffers are alloc()'d (heap), not arena, even
     though the struct dfa header itself lives in the arena */
  for(i = 0; i < prog->nregexes; i++)
    dfa_free(prog->regexes[i]);

  arena_free(&prog->a);
  alloc_free(prog);
}
