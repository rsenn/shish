#include "sed_internal.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

int
sed_compile(struct sed** out, const char* script, size_t len, unsigned flags) {
  struct sed* prog;
  int rc;

  *out = NULL;
  prog = alloc(sizeof(*prog));

  if(!prog)
    return SED_ENOMEM;

  byte_zero(prog, sizeof(*prog));
  prog->flags = flags;

  rc = sed_parse(prog, script, len, flags);

  if(rc != SED_OK) {
    /* sed_parse frees everything it built locally on failure; only
       the wfile table (interned directly into *prog as parsing went)
       needs sed_free's help here. */
    sed_free(prog);
    return rc;
  }

  *out = prog;
  return SED_OK;
}
