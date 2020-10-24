#include "../prompt.h"
#include "../debug.h"

void
prompt_nextline(void) {
#ifdef DEBUG_OUTPUT
 debug_ulong("prompt_nextline", prompt_number, 0);
  debug_nl();
  #endif
 if(prompt_number <= 1)
    prompt_number++;
}
