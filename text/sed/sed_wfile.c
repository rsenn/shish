#include "sed_internal.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* sed_wfile_intern: dedupes 'w'/'s///w' targets by exact name so
 * several commands writing to the same file share one caller-side
 * open/truncate (POSIX: each wfile is created once, before processing
 * begins). Returns the (stable) index, or -1 on allocation failure.
 * ----------------------------------------------------------------------- */
int
sed_wfile_intern(struct sed* prog, const char* name, size_t len) {
  size_t i;
  struct sed_wfile_entry* ne;
  char* copy;

  for(i = 0; i < prog->nwfiles; i++) {
    if(prog->wfiles[i].len == len && byte_diff(prog->wfiles[i].name, len, name) == 0)
      return (int)i;
  }

  ne = alloc_re(prog->wfiles, (prog->nwfiles + 1) * sizeof(*ne));

  if(!ne)
    return -1;

  prog->wfiles = ne;

  copy = alloc(len + 1);

  if(!copy)
    return -1;

  byte_copy(copy, len, name);
  copy[len] = 0;

  prog->wfiles[prog->nwfiles].name = copy;
  prog->wfiles[prog->nwfiles].len = len;
  return (int)prog->nwfiles++;
}

size_t
sed_wfile_count(const struct sed* prog) {
  return prog->nwfiles;
}

const char*
sed_wfile_name(const struct sed* prog, size_t i) {
  return i < prog->nwfiles ? prog->wfiles[i].name : NULL;
}
