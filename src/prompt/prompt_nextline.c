#include "../prompt.h"
#include "../debug.h"

void
prompt_nextline(void) {
  if(prompt_number <= 1)
    prompt_number++;
}
