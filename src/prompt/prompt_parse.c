#include <stdlib.h>
#include "var.h"
#include "debug.h"
#include "fdstack.h"
#include "fd.h"
#include "tree.h"
#include "parse.h"
#include "source.h"
#include "prompt.h"

static unsigned int prompt_hash;          /* hash of unexpanded PS1 */

/* hash a prompt value
 * ----------------------------------------------------------------------- */
static inline unsigned int prompt_hashstr(const char *s, unsigned long n) {
  unsigned int hash = 0x7fedcb95; /* some prime number */
  while(n) hash = (hash * 0x0123456b) ^ *s++, n--;
  return hash;
}

/* parses the prompt if necessary
 * ----------------------------------------------------------------------- */
void prompt_parse(void) {
  const char *value;
  unsigned int newhash;
  unsigned long n;
  struct parser p;
  struct fd fd;
  struct source src;

  /* parse PS1 only */
  if(prompt_number != 1)
    return;

  value = var_value(prompt_var, &n);
  newhash = prompt_hashstr(value, n);

  /* it was (most likely) the same prompt str last time */
  if(newhash == prompt_hash)
    return;

  /* now initialize input buffer from the prompt str and parse */
  fd_push(&fd, STDSRC_FILENO, FD_MODE_READ);
  fd_string(&fd, value, n);
  source_push(&src);
  parse_init(&p, P_DEFAULT);
  parse_dquoted(&p);
  parse_string(&p, 0);
  
  /* if we have a word tree then free any previous word tree
     and set the new one */
  if(prompt_node)
    tree_free(prompt_node);
  
  prompt_node = parse_getarg(&p);
  
  /* now leave the context in which the prompt was parsed */
  source_pop();
  fd_pop(&fd);
  
#ifdef DEBUG
//  debug_list(prompt_node, 0);
#endif /* DEBUG */
}


