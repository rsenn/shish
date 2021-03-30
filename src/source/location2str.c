#include "../source.h"
#include "../str.h"
#include "../fdtable.h"
#include "../fd.h"

const char*
location2str(const struct location loc) {
  char buf[FMT_LOC];
  const char* name;
  size_t pos = 0;
  if((name = fdtable[-1]->name)) {
    pos += str_copy(buf, name);
    buf[pos++] = ':';
  }
  pos += fmt_loc(&buf[pos], &loc);
  buf[pos] = '\0';
  return shell_strdup(buf);
}
