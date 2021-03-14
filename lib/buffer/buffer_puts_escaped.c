#include "../buffer.h"
#include "../str.h"
#include "../fmt.h"

int
buffer_puts_escaped_args(buffer* b, const char* x, size_t (*escape)(char*, void*, void*, void*, void*), void* args[]) {
  if(escape == NULL)
    escape = (size_t(*)(char*, void*, void*, void*, void*)) & fmt_escapecharshell;
  return buffer_putfmt_args(b, x, str_len(x), (size_t(*)(char*, int, void*, void*, void*, void*))escape, args);
}

int
buffer_puts_escaped(buffer* b, const char* x, size_t (*escape)(char*, int)) {
  void* args[4] = {0, 0, 0, 0};
  return buffer_putfmt_args(b, x, str_len(x), (size_t(*)(char*, int, void*, void*, void*, void*))escape, args);
}
