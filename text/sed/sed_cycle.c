#include "sed_internal.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

struct sed_state*
sed_state_new(struct sed* prog,
              sed_read_fn read,
              sed_out_fn out,
              sed_wfile_fn wfile,
              sed_rfile_fn rfile,
              void* ctx) {
  struct sed_state* st = alloc(sizeof(*st));

  if(!st)
    return NULL;

  byte_zero(st, sizeof(*st));
  st->prog = prog;
  st->read = read;
  st->out = out;
  st->wfile = wfile;
  st->rfile = rfile;
  st->ctx = ctx;
  stralloc_init(&st->pattern);
  stralloc_init(&st->hold);
  stralloc_init(&st->nextbuf);
  st->pending_tail = &st->pending_head;
  return st;
}

void
sed_state_free(struct sed_state* st) {
  if(!st)
    return;

  sed_pending_clear(st);
  stralloc_free(&st->pattern);
  stralloc_free(&st->hold);
  stralloc_free(&st->nextbuf);
  alloc_free(st);
}

void
sed_pending_push(struct sed_state* st, int is_file, const char* data, size_t len) {
  struct sed_pending* node = alloc(sizeof(*node));

  if(!node)
    return; /* OOM: silently drop the queued a/r item */

  node->data = alloc(len + 1);

  if(!node->data) {
    alloc_free(node);
    return;
  }

  byte_copy(node->data, len, data);
  node->data[len] = 0;
  node->len = len;
  node->is_file = is_file;
  node->next = NULL;
  *st->pending_tail = node;
  st->pending_tail = &node->next;
}

static void
pending_drain(struct sed_state* st, int emit) {
  struct sed_pending* n = st->pending_head;

  while(n) {
    struct sed_pending* next = n->next;

    if(emit) {
      if(n->is_file) {
        if(st->rfile)
          st->rfile(st->ctx, n->data);
      } else {
        st->out(st->ctx, n->data, n->len);
        st->out(st->ctx, "\n", 1);
      }
    }

    alloc_free(n->data);
    alloc_free(n);
    n = next;
  }

  st->pending_head = NULL;
  st->pending_tail = &st->pending_head;
}

void
sed_pending_flush(struct sed_state* st) {
  pending_drain(st, 1);
}

void
sed_pending_clear(struct sed_state* st) {
  pending_drain(st, 0);
}

static void
sed_out_pattern(struct sed_state* st) {
  st->out(st->ctx, st->pattern.s ? st->pattern.s : "", st->pattern.len);

  if(st->cur_had_nl)
    st->out(st->ctx, "\n", 1);
}

static void
sed_out_lineno(struct sed_state* st) {
  char buf[32];
  size_t l = fmt_ulong(buf, st->lineno);

  st->out(st->ctx, buf, l);
  st->out(st->ctx, "\n", 1);
}

static void
sed_out_wfile_line(struct sed_state* st, int idx, const char* s, size_t n) {
  if(idx < 0 || !st->wfile)
    return;

  st->wfile(st->ctx, (size_t)idx, s, n);
  st->wfile(st->ctx, (size_t)idx, "\n", 1);
}

static size_t
find_nl(const char* s, size_t n) {
  size_t i;

  for(i = 0; i < n; i++)
    if(s[i] == '\n')
      return i;

  return n;
}

/* fetch_line: pulls one line into *dst via a one-line lookahead
 * buffer, so *is_last (POSIX '$') is known the moment a line becomes
 * available -- used both for the normal top-of-cycle read and for
 * 'n'/'N' mid-script. Returns 0 at true end of input. */
static int
fetch_line(struct sed_state* st, stralloc* dst, int* had_nl, int* is_last) {
  if(!st->next_valid) {
    const char* s;
    size_t n;
    int hn;
    int rc = st->read(st->ctx, &s, &n, &hn);

    if(rc > 0) {
      stralloc_copyb(&st->nextbuf, s, n);
      st->next_had_nl = hn;
      st->next_valid = 1;
    }
  }

  if(!st->next_valid)
    return 0;

  stralloc_copyb(dst, st->nextbuf.s, st->nextbuf.len);
  *had_nl = st->next_had_nl;
  st->next_valid = 0;

  {
    const char* s;
    size_t n;
    int hn;
    int rc = st->read(st->ctx, &s, &n, &hn);

    if(rc > 0) {
      stralloc_copyb(&st->nextbuf, s, n);
      st->next_had_nl = hn;
      st->next_valid = 1;
    }
  }

  *is_last = !st->next_valid;
  return 1;
}

/* run_one_cycle: executes the script once against the current
 * pattern space, restarting from the top (without reading new input)
 * whenever 'D' leaves something behind -- see its case below. Returns
 * 1 if q/Q was hit or n/N ran out of input (sed_run() should stop).
 * ----------------------------------------------------------------------- */
static int
run_one_cycle(struct sed_state* st) {
  size_t pc;

restart:
  pc = 0;
  st->suppress_print = 0;

  while(pc < st->prog->ncmd) {
    struct sed_cmd* c = &st->prog->cmds[pc];
    int match = sed_range_match(c, st);

    if(!match) {
      pc = (c->letter == '{') ? c->jump : pc + 1;
      continue;
    }

    switch(c->letter) {
      case '{':
      case ':': pc++; break;

      case '=':
        sed_out_lineno(st);
        pc++;
        break;

      case 'a':
        sed_pending_push(st, 0, c->u.text.s, c->u.text.len);
        pc++;
        break;

      case 'i':
        st->out(st->ctx, c->u.text.s, c->u.text.len);
        st->out(st->ctx, "\n", 1);
        pc++;
        break;

      case 'c': {
        /* naddr<2: fires on every matching line. naddr==2: fires only
           on the line that closes the range (in_range just went back
           to 0), or on every line with '!' (which drops the concept of
           a range's "closing" line entirely). */
        int fire = (c->naddr < 2) || c->negate || !c->in_range;

        if(fire) {
          st->out(st->ctx, c->u.text.s, c->u.text.len);
          st->out(st->ctx, "\n", 1);
        }

        stralloc_copys(&st->pattern, "");
        st->suppress_print = 1;
        goto end_cycle;
      }

      case 'd': st->suppress_print = 1; goto end_cycle;

      case 'D': {
        size_t nl = find_nl(st->pattern.s, st->pattern.len);

        if(nl == st->pattern.len) {
          st->suppress_print = 1;
          goto end_cycle;
        }

        {
          size_t rest = st->pattern.len - (nl + 1);

          byte_copy(st->pattern.s, rest, st->pattern.s + nl + 1);
          st->pattern.len = rest;
        }

        goto restart;
      }

      case 'g':
        stralloc_copy(&st->pattern, &st->hold);
        pc++;
        break;

      case 'G':
        stralloc_catc(&st->pattern, '\n');
        stralloc_catb(&st->pattern, st->hold.s ? st->hold.s : "", st->hold.len);
        pc++;
        break;

      case 'h':
        stralloc_copy(&st->hold, &st->pattern);
        pc++;
        break;

      case 'H':
        stralloc_catc(&st->hold, '\n');
        stralloc_catb(&st->hold, st->pattern.s ? st->pattern.s : "", st->pattern.len);
        pc++;
        break;

      case 'l':
        sed_do_list(st, st->pattern.s, st->pattern.len);
        pc++;
        break;

      case 'n': {
        int hn, il;

        if(!st->prog->autoprint_off)
          sed_out_pattern(st);

        sed_pending_flush(st);

        if(!fetch_line(st, &st->pattern, &hn, &il)) {
          st->quit = 1;
          st->quit_status = 0;
          st->suppress_print = 1; /* already printed above */
          goto end_cycle;
        }

        st->lineno++;
        st->cur_had_nl = hn;
        st->cur_is_last = il;
        pc++;
        break;
      }

      case 'N': {
        int hn, il;
        stralloc nl;

        stralloc_init(&nl);

        if(!fetch_line(st, &nl, &hn, &il)) {
          stralloc_free(&nl);
          st->quit = 1;
          st->quit_status = 0;
          st->suppress_print = 1; /* POSIX: N at EOF quits without printing, unlike n */
          goto end_cycle;
        }

        stralloc_catc(&st->pattern, '\n');
        stralloc_catb(&st->pattern, nl.s, nl.len);
        stralloc_free(&nl);
        st->lineno++;
        st->cur_had_nl = hn;
        st->cur_is_last = il;
        pc++;
        break;
      }

      case 'p':
        sed_out_pattern(st);
        pc++;
        break;

      case 'P': {
        size_t nl = find_nl(st->pattern.s, st->pattern.len);

        st->out(st->ctx, st->pattern.s, nl);
        st->out(st->ctx, "\n", 1);
        pc++;
        break;
      }

      case 'q':
        st->quit = 1;
        st->quit_status = c->u.qstatus;
        goto end_cycle;

      case 'r':
        sed_pending_push(st, 1, c->u.rfile, str_len(c->u.rfile));
        pc++;
        break;

      case 's': {
        if(sed_subst_exec(&c->u.s, st)) {
          if(c->u.s.print)
            sed_out_pattern(st);

          if(c->u.s.wfile >= 0)
            sed_out_wfile_line(st, c->u.s.wfile, st->pattern.s, st->pattern.len);
        }

        pc++;
        break;
      }

      case 't':
        if(st->tflag) {
          st->tflag = 0;
          pc = c->jump;
        } else {
          pc++;
        }

        break;

      case 'w':
        sed_out_wfile_line(st, c->u.wfile, st->pattern.s, st->pattern.len);
        pc++;
        break;

      case 'x': {
        stralloc tmp = st->pattern;

        st->pattern = st->hold;
        st->hold = tmp;
        pc++;
        break;
      }

      case 'y': {
        size_t i;

        for(i = 0; i < st->pattern.len; i++)
          st->pattern.s[i] = (char)c->u.y[(unsigned char)st->pattern.s[i]];

        pc++;
        break;
      }

      case 'b': pc = c->jump; break;

      default: pc++; break;
    }
  }

end_cycle:
  if(!st->suppress_print && !st->prog->autoprint_off)
    sed_out_pattern(st);

  sed_pending_flush(st);
  return st->quit;
}

void
sed_run(struct sed_state* st, int* exit_status) {
  int had_nl, is_last;

  for(;;) {
    if(!fetch_line(st, &st->pattern, &had_nl, &is_last))
      break;

    st->lineno++;
    st->cur_had_nl = had_nl;
    st->cur_is_last = is_last;
    st->tflag = 0;

    if(run_one_cycle(st))
      break;
  }

  if(exit_status)
    *exit_status = st->quit_status;
}
