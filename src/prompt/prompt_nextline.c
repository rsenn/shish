#include "../prompt.h"
#include "../debug.h"

void
prompt_nextline(void) {
#ifdef DEBUG_OUTPUT
  debug_fn_ws();
   debug_n(prompt_number);
  debug_nl();
#endif
  if(prompt_number <= 1)
    prompt_number++;
}
