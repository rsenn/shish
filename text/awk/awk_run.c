/* awk_run(): interpreter setup/teardown, BEGIN/END, and the main
 * per-record loop over the compiled program's rules. */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"

extern char** environ; /* not declared by every libc's headers uniformly
                           (glibc/musl/dietlibc all provide the symbol
                           itself); ENVIRON is simply empty if this
                           doesn't resolve on some exotic target. */

static void
set_default_str(awk_cell* c, const char* s) {
  awk_cell_set_str(c, s, str_len(s), 0);
}

static void
setup_argv(struct awk_state* st, char* const* operands) {
  hashmap* argv = awk_array_of(st, awk_global(st, SP_ARGV));
  char key[32];
  size_t klen, i;
  awk_cell* c;

  klen = fmt_ulong(key, 0);
  c = alloc_zero(sizeof(awk_cell));
  awk_cell_set_str(c, "awk", 3, 0);
  hashmap_put2(argv, key, klen, c);

  for(i = 0; operands && operands[i]; i++) {
    klen = fmt_ulong(key, (unsigned long)(i + 1));
    c = alloc_zero(sizeof(awk_cell));
    awk_cell_set_str(c, operands[i], str_len(operands[i]), 0);
    hashmap_put2(argv, key, klen, c);
  }

  awk_cell_set_num(awk_global(st, SP_ARGC), (double)(i + 1));
}

static void
setup_environ(struct awk_state* st) {
  hashmap* env = awk_array_of(st, awk_global(st, SP_ENVIRON));
  size_t i;

  for(i = 0; environ && environ[i]; i++) {
    const char* e = environ[i];
    size_t eq = 0;
    awk_cell* c;
    double v;
    int numeric;

    while(e[eq] && e[eq] != '=')
      eq++;

    if(e[eq] != '=')
      continue;

    c = alloc_zero(sizeof(awk_cell));
    numeric = awk_looks_numeric(e + eq + 1, str_len(e + eq + 1), &v);
    awk_cell_set_str(c, e + eq + 1, str_len(e + eq + 1), numeric);

    if(numeric)
      c->num = v;

    hashmap_put2(env, e, eq, c);
  }
}

static int
run_list(struct awk_state* st, struct anode* list) {
  struct anode* b;
  int rc = CF_NORMAL;

  for(b = list; b; b = b->next) {
    rc = awk_exec(st, b);

    if(rc != CF_NORMAL || st->unwind)
      break;
  }

  return rc;
}

static int
rule_matches(struct awk_state* st, struct awk_rule* ru, unsigned char* active) {
  if(ru->kind == 0)
    return 1;

  if(ru->kind == 1) {
    awk_cell c = awk_eval(st, ru->pat1);

    return awk_tobool(st, &c);
  }

  /* range pattern */
  if(!*active) {
    awk_cell c1 = awk_eval(st, ru->pat1);

    if(!awk_tobool(st, &c1))
      return 0;

    *active = 1;
  }

  {
    awk_cell c2 = awk_eval(st, ru->pat2);

    if(awk_tobool(st, &c2))
      *active = 0;
  }

  return 1;
}

static void
close_current_file(struct awk_state* st) {
  if(!st->cur_open)
    return;

  if(st->cur.h && st->io->close_read)
    st->io->close_read(st->io->ctx, st->cur.h);

  alloc_free(st->cur.name);
  st->cur_open = 0;
}

int
awk_run(struct awk_prog* prog, const struct awk_io* io, char* const* assigns,
        char* const* operands, const char* fs) {
  struct awk_state st;
  size_t i;
  int rc = CF_NORMAL;
  int begin_stopped;

  byte_zero(&st, sizeof(st));
  st.prog = prog;
  st.io = io;
  arena_init(&st.tmp, &arena_heap, 0);
  st.globals = alloc_zero(prog->nglobals * sizeof(awk_cell));
  awk_rec_init(&st.rec);
  st.range_active = prog->nrules ? alloc_zero(prog->nrules) : NULL;

  set_default_str(&st.globals[SP_FS], fs ? fs : " ");
  set_default_str(&st.globals[SP_OFS], " ");
  set_default_str(&st.globals[SP_ORS], "\n");
  set_default_str(&st.globals[SP_RS], "\n");
  set_default_str(&st.globals[SP_SUBSEP], "\034");
  set_default_str(&st.globals[SP_CONVFMT], "%.6g");
  set_default_str(&st.globals[SP_OFMT], "%.6g");
  set_default_str(&st.globals[SP_FILENAME], "");
  awk_cell_set_num(&st.globals[SP_RLENGTH], -1);

  setup_argv(&st, operands);
  setup_environ(&st);
  st.cur_argi = 1; /* ARGV[0] is the program name, not an operand */

  for(i = 0; assigns && assigns[i]; i++)
    awk_apply_assignment(&st, assigns[i]);

  rc = run_list(&st, prog->begin);
  begin_stopped = (rc == CF_EXIT) || st.unwind;

  if(!begin_stopped && prog->uses_main_input) {
    for(;;) {
      int gr = awk_getrecord(&st);

      if(gr <= 0 || st.unwind)
        break;

      for(i = 0; i < prog->nrules; i++) {
        struct awk_rule* ru = &prog->rules[i];
        int matched = rule_matches(&st, ru, &st.range_active[i]);

        if(st.unwind)
          break;

        if(!matched)
          continue;

        if(ru->action) {
          rc = awk_exec(&st, ru->action);
        } else {
          awk_cell f0 = *awk_rec_field(&st, 0);
          awk_cell* ors = awk_global(&st, SP_ORS);

          awk_output(&st, f0.str ? f0.str : "", f0.str ? str_len(f0.str) : 0);
          awk_output(&st, ors->str ? ors->str : "\n", ors->str ? str_len(ors->str) : 1);
          rc = CF_NORMAL;
        }

        if(st.unwind || rc == CF_EXIT)
          goto main_loop_done;

        if(rc == CF_NEXT)
          break;

        if(rc == CF_NEXTFILE) {
          close_current_file(&st);
          break;
        }
      }
    }
  }

main_loop_done:

  if(st.unwind != UNWIND_ERROR)
    run_list(&st, prog->end);

  close_current_file(&st);
  awk_streams_close_all(&st);
  awk_streams_flush_all(&st);

  for(i = 0; i < prog->nglobals; i++)
    awk_cell_free(&st.globals[i]);

  alloc_free(st.globals);
  alloc_free(st.range_active);
  awk_rec_free(&st.rec);
  arena_free(&st.tmp);

  return st.exit_status;
}
