#include "../../builtin.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"

/* id_name appends the name of id from a passwd or group file ("name:x:ID:...")
   to out; returns 1 if found, 0 if not */
int
id_name(const char* file, unsigned long id, stralloc* out) {
  stralloc sa = {0};
  const char* p;
  unsigned long v;
  int fd, r = 0;

  if((fd = open_read(file)) == -1)
    return 0;

  if(readclose_append(fd, &sa, 4096) < 0) {
    stralloc_free(&sa);
    return 0;
  }

  stralloc_0(&sa);

  for(p = sa.s; *p;) {
    size_t n = str_chr(p, ':');
    const char* q = p + n;

    if(*q && *(q += 1 + str_chr(q + 1, ':')) && scan_ulong(q + 1, &v) && v == id) {
      stralloc_catb(out, p, n);
      r = 1;
      break;
    }

    p += str_chr(p, '\n');

    if(*p)
      p++;
  }

  stralloc_free(&sa);
  return r;
}
