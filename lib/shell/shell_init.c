#include "buffer.h"

buffer     *shell_buff;
const char *shell_name;

/* set the output buffer and the basename for error messages */
void shell_init(buffer *b, const char *n)
{
  shell_buff = b;
  shell_name = n;
}
