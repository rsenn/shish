#include "prompt.h"
#include "term.h"
#include "var.h"

char prompt_var[4] = "PS0";
int prompt_number = 0;

/* shows the commandline prompt after expanding if necessary
 * ----------------------------------------------------------------------- */
void
prompt_show(void) {
  const char* prompt;
  size_t plen;

  /* increment prompt number if smaller than PS2 */
  if(prompt_number <= 1)
    prompt_number++;

  /* get prompt variable name */
  prompt_var[2] = '0' + prompt_number;

  /* parse and expand PS1 if necessary */
  prompt_parse();
  prompt_expand();

  /* use expansion of PS1? */
  if(prompt_number == 1) {
    prompt = prompt_expansion.s;
    plen = prompt_expansion.len;
  } else {
    prompt = var_vdefault(prompt_var, ">", &plen);
  }

  /* put the prompt to the terminal */
  if(prompt) {
    buffer_put(term_output, prompt, plen);
    buffer_flush(term_output);
  }
}
