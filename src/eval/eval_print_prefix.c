#include "../fd.h"
#include "../eval.h"
#include "../expand.h"
#include "../parse.h"
#include "../source.h"
#include "../sh.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/stralloc.h"

/* the "set -x" line prefix: $PS4 expanded as a double-quoted word, or "+ " (one '+' per
 * subshell level) when PS4 is unset. Tracing is off while it is expanded.
 * ----------------------------------------------------------------------- */
void
eval_print_prefix(struct eval* e, buffer* b) {
  size_t len;
  const char* ps4 = var_value("PS4", &len);

  if(ps4 && len) {
    stralloc txt, out;
    struct fd fd;
    struct source src;
    struct parser p;
    int xtrace = sh->opts.xtrace, flags = e->flags;
    size_t i;

    stralloc_init(&txt);
    stralloc_init(&out);
    stralloc_catc(&txt, '"');

    for(i = 0; i < len; i++) {
      if(ps4[i] == '"')
        stralloc_catc(&txt, '\\');

      stralloc_catc(&txt, ps4[i]);
    }

    stralloc_catc(&txt, '"');

    sh->opts.xtrace = 0;
    e->flags &= ~E_PRINT;

    source_buffer(&src, &fd, txt.s, txt.len);
    parse_init(&p, P_DEFAULT);

    if(parse_gettok(&p, P_NOKEYWD) & (T_WORD | T_NAME)) {
      union node* arg = parse_getarg(&p);

      if(arg) {
        wordlist wl;
        arena_pos pos = arena_tell(&expand_arena);
        int n;
        char** v;

        wordlist_init(&wl, &expand_arena, var_vdefault("IFS", IFS_DEFAULT, NULL));
        expand_args(arg, &wl, 0);
        v = wordlist_argv(&wl, &n);

        if(n)
          stralloc_copys(&out, v[0]);

        wordlist_free(&wl);
        arena_rewind(&expand_arena, pos);
        tree_free(arg);
      }
    }

    source_popfd(&fd);
    stralloc_free(&p.sa);
    sh->opts.xtrace = xtrace;
    e->flags = flags;

    buffer_put(b, out.s, out.len);
    stralloc_free(&out);
    stralloc_free(&txt);
    return;
  }

  buffer_putnc(b, '+', eval_depth());
  buffer_putspace(b);
}
