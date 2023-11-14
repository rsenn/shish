#include "../parse.h"
#include "../tree.h"

/* 3.9.2 - parse a pipeline
 * ----------------------------------------------------------------------- */
union node*
parse_pipeline(struct parser* p) {
  int negate = 0;
  union node *node, *pipeline, **cmdptr;
  enum tok_flag tok;

  /* on T_NOT toggle negate */
  while((tok = parse_gettok(p, P_DEFAULT)) == T_NOT)
    negate = !negate;

  p->pushback++;

  if((node = parse_command(p, P_DEFAULT)) == NULL)
    return NULL;

  /* on a T_PIPE, create a new pipeline */
  if((tok = parse_gettok(p, P_DEFAULT)) == T_PIPE) {
    /* create new pipeline node */
    pipeline = tree_newnode(N_PIPELINE);
    pipeline->npipe.bgnd = 0;

    /* create a command list inside the pipeline */
    pipeline->npipe.cmds = node;
    pipeline->npipe.ncmd = 1;
    // cmdptr = &pipeline->npipe.cmds;
    cmdptr = &node->next;

    /* parse commands and add them to the pipeline
       as long as there are pipe tokens */
    do {
      node = parse_command(p, P_SKIPNL);
      tree_link(node, cmdptr);
      // tree_unshift(node, cmdptr);
      pipeline->npipe.ncmd++;
    } while(parse_gettok(p, P_DEFAULT) == T_PIPE);

    /* set command to the pipeline */
    node = pipeline;
  }

  p->pushback++;

  /* link in a N_NOT node if requested */
  if(negate) {
    union node* neg;
    neg = tree_newnode(N_NOT);
    neg->nnot.pipeline = node;
    node = neg;
  }

  return node;
}
