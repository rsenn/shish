#include "../parse.h"
#include "../tree.h"
#include "../../lib/byte.h"

/* is the word just lexed the plain text s (len bytes), unquoted? */
static int
word_is(struct parser* p, const char* s, size_t len) {
  union node* n = p->tree;

  return n && n->id == N_ARGSTR && !n->next && n->nargstr.len == len && !byte_diff(n->nargstr.s, len, s);
}

/* 3.9.2 - parse a pipeline
 * ----------------------------------------------------------------------- */
union node*
parse_pipeline(struct parser* p) {
  int negate = 0, timed = 0, posix = 0;
  union node *node, *pipeline, **cmdptr;
  enum tok_flag tok;

  /* on T_NOT toggle negate, on the word "time" [-p] request timing */
  for(;;) {
    tok = parse_gettok(p, P_DEFAULT);

    if(tok == T_NOT)
      negate = !negate;
    else if(tok == T_NAME && !timed && word_is(p, "time", 4)) {
      timed = 1;

      if((tok = parse_gettok(p, P_DEFAULT)) & (T_NAME | T_WORD) && word_is(p, "-p", 2))
        posix = 1;
      else
        p->pushback++;
    } else
      break;
  }

  p->pushback++;

  if((node = parse_command(p, P_DEFAULT)) == NULL) {
    /* a lone "time" is an error, an empty line is not */
    if(timed)
      parse_error(p, T_NAME | T_WORD);

    return NULL;
  }

  /* on a T_PIPE, create a new pipeline */
  if((tok = parse_gettok(p, P_DEFAULT)) == T_PIPE) {
    /* create new pipeline node */
    pipeline = tree_newnode(N_PIPELINE);
    pipeline->npipe.bgnd = 0;

    /* create a command list inside the pipeline */
    pipeline->npipe.cmds = node;
    pipeline->npipe.ncmd = 1;
    cmdptr = &node->next;

    /* parse commands and add them to the pipeline
       as long as there are pipe tokens */
    do {
      node = parse_command(p, P_SKIPNL);

      /* a "|" must be followed by another command -- report it here
         rather than just propagating NULL, since sh_loop()'s generic
         fallback treats a bare T_EOF as not an error. */
      if(node == NULL) {
        parse_error(p, T_NAME | T_WORD);
        tree_free(pipeline);
        return NULL;
      }

      tree_link(node, cmdptr);
      pipeline->npipe.ncmd++;
    } while(parse_gettok(p, P_DEFAULT) == T_PIPE);

    /* set command to the pipeline */
    node = pipeline;
  }

  p->pushback++;

  /* link in a N_TIME node for "time" */
  if(timed) {
    union node* tm;
    tm = tree_newnode(N_TIME);
    tm->ntime.pipeline = node;
    tm->ntime.posix = posix;
    node = tm;
  }

  /* link in a N_NOT node if requested */
  if(negate) {
    union node* neg;
    neg = tree_newnode(N_NOT);
    neg->nnot.pipeline = node;
    node = neg;
  }

  return node;
}
