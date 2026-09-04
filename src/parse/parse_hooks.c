#include "../parse.h"

void (*parse_prompt_hook)(void) = 0;
void (*parse_exit_hook)(int) = 0;
