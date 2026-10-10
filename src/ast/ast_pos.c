#include "../ast.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../source.h"
#include "../../lib/str.h"
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

/* ----------------------------------------------------------------------- */
void
ast_pos(struct ast* a, const struct location* loc, size_t len) {
  if(a->no_position)
    return;

  if(a->loc) {
    /* "file:line:col": fmt_location() copies the whole file name, so size for it */
    const char* name = fdtable[-1]->name;
    char* buf = alloca((name ? str_len(name) : 0) + FMT_LOC + 2);

    json_key(&a->j, "loc");
    json_str(&a->j, buf, fmt_location(buf, *loc));
  }

  if(a->range)
    json_kpair(&a->j, "range", loc->offset, loc->offset + len);
}
