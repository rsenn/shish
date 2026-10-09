#include "../byte.h"
#include "../mmap.h"
#include "../path_internal.h"
#include "../windoze.h"

/* home directory of uid from /etc/passwd ("name:passwd:uid:gid:gecos:home:shell"),
 * in a static buffer; NULL if there is no such user or no file.
 * Lines without a trailing newline or with "\r\n" are fine; entries
 * whose home does not fit in PATH_MAX are skipped.
 * ----------------------------------------------------------------------- */
char*
path_gethome(int uid) {
  static char home[PATH_MAX + 1];
  const char *map, *p, *end;
  size_t size;

  if(!(map = mmap_read("/etc/passwd", &size)))
    return NULL;

  for(p = map, end = map + size; p < end;) {
    const char *line = p, *eol = p, *f[7];
    size_t i, nf = 0;
    unsigned long id = 0;

    while(eol < end && *eol != '\n')
      ++eol;
    p = eol + 1;

    if(eol > line && eol[-1] == '\r')
      --eol;

    /* split into fields; f[i] is the start of the i-th field (0-based) */
    f[nf++] = line;
    for(i = 0; line + i < eol && nf < 7; ++i)
      if(line[i] == ':')
        f[nf++] = line + i + 1;

    if(nf < 6)
      continue;

    /* the third field is the uid: digits only */
    if(f[3] - 1 == f[2])
      continue;
    for(i = 0; f[2] + i < f[3] - 1; ++i) {
      if(f[2][i] < '0' || f[2][i] > '9')
        break;
      id = id * 10 + (f[2][i] - '0');
    }
    if(f[2] + i != f[3] - 1 || id != (unsigned long)uid)
      continue;

    /* the sixth field is the home, up to the next colon or the end of the line */
    i = (nf > 6 ? f[6] - 1 : eol) - f[5];
    if(i > PATH_MAX)
      continue;

    byte_copy(home, i, f[5]);
    home[i] = '\0';
    mmap_unmap(map, size);
    return home;
  }

  mmap_unmap(map, size);
  return NULL;
}
