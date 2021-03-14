#include "../debug.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../prompt.h"
#include "../source.h"
#include "../tree.h"
#include "../var.h"
#include <stdlib.h>

static unsigned int prompt_hash; /* hash of unexpanded PS1 */
static const char prompt_default[] = "\\u@\\h:\\w \\$ ";

/* hash a prompt value
 * ----------------------------------------------------------------------- */
static inline unsigned int
prompt_hashstr(const char* s, unsigned long n) {
  unsigned int hash = 0x7fedcb95; /* some prime number */
  while(n) hash = (hash * 0x0123456b) ^ *s++, n--;
  return hash;
}

/* parses the prompt if necessary
 * ----------------------------------------------------------------------- */
void
prompt_parse(void) {
  const char* value;
  unsigned int newhash;
  size_t n;
  struct parser p;
  struct fd fd;
  struct source src;

  /* parse PS1 only */
  if(prompt_number != 1)
    return;

  value = var_vdefault(prompt_var, prompt_default, &n);
  newhash = prompt_hashstr(value, n);

  /* it was (most likely) the same prompt str last time */
  if(newhash == prompt_hash)
    return;

  /* now initialize input buffer from the prompt str and parse */
  source_buffer(&src, &fd, value, n);

  parse_init(&p, P_DEFAULT);
  parse_dquoted(&p);
  parse_string(&p, 0);

  /* if we have a word tree then free any previous word tree
     and set the new one */
  if(prompt_node)
    tree_free(prompt_node);

  prompt_node = parse_getarg(&p);

  /* now leave the context in which the prompt was parsed */
  source_popfd(&fd);

#ifdef DEBUG_OUTPUT_
/*  debug_list(prompt_node, 0);*/
#endif /* DEBUG_OUTPUT */
}
