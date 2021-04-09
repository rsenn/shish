#include "../source.h"
#include "../fdtable.h"
#include "../fd.h"
#include "../../lib/str.h"

size_t
fmt_location(char* buf, const struct location loc) {
  const char* name;
  size_t pos = 0;
  if((name = fdtable[-1]->name)) {
    pos += str_copy(buf, name);
    buf[pos++] = ':';
  }
  pos += fmt_loc(&buf[pos], &loc);
  buf[pos] = '\0';
  return pos;
}

const char*
location2str(const struct location loc) {
  char buf[FMT_LOC];
  size_t len = fmt_location(buf, loc);
  buf[len] = '\0';
  return shell_strdup(buf);
}
