/* runtime error reporting, program stdout/stderr, the table of open
 * redirection/getline targets (> >> | and getline<file / cmd|getline),
 * and the main-input loop (ARGV/ARGC, "name=value" operands, "-"). */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"

void
awk_runtime_error(struct awk_state* st, const char* msg) {
  if(st->unwind == UNWIND_ERROR)
    return; /* keep the first error's message */

  awk_output_err(st, "awk: ", 5);
  awk_output_err(st, msg, str_len(msg));
  awk_output_err(st, "\n", 1);
  st->unwind = UNWIND_ERROR;
  st->exit_status = 2;
}

void
awk_output(struct awk_state* st, const char* s, size_t n) {
  if(st->io->write)
    st->io->write(st->io->ctx, st->io->out, s, n);
}

void
awk_output_err(struct awk_state* st, const char* s, size_t n) {
  if(st->io->write)
    st->io->write(st->io->ctx, st->io->err, s, n);
}

/* ---- the buffer glue: every input stream (main input, getline<file,
   cmd|getline) reads through a buffer on top of io->read() ---------- */

static ssize_t
stream_read_op(int fd, void* buf, size_t len, void* arg) {
  buffer* b = arg;
  struct awk_stream* s = b->cookie;

  (void)fd;
  return (ssize_t)s->st->io->read(s->st->io->ctx, s->h, buf, len);
}

static void
stream_init_reader(struct awk_state* st, struct awk_stream* strm) {
  strm->st = st;
  buffer_init(&strm->rb, &stream_read_op, -1, strm->rbuf, sizeof(strm->rbuf));
  strm->rb.cookie = strm;
}

/* reads one record (RS's first byte as delimiter; default "\n") from
   an already-opened stream. 1 = got one (possibly empty; out holds it,
   delimiter stripped), 0 = clean EOF, -1 = stream never opened. */
static int
stream_getline(struct awk_stream* strm, stralloc* out) {
  struct awk_state* st = strm->st;
  const char* rsv = awk_global(st, SP_RS)->str;
  char rs = (rsv && rsv[0]) ? rsv[0] : '\n';
  char chunk[512];
  int got = 0;

  if(!strm->h)
    return -1;

  if(strm->eof)
    return 0;

  stralloc_zero(out);

  for(;;) {
    int r = buffer_get_until(&strm->rb, chunk, sizeof(chunk), &rs, 1);

    if(r <= 0) {
      strm->eof = 1;
      break;
    }

    got = 1;

    if(chunk[r - 1] == rs) {
      stralloc_catb(out, chunk, (size_t)(r - 1));
      return 1;
    }

    stralloc_catb(out, chunk, (size_t)r);

    if((size_t)r < sizeof(chunk)) {
      strm->eof = 1; /* short read with no delimiter seen: end of stream */
      break;
    }
  }

  return got;
}

/* ---- redirection/getline target table ----------------------------- */

static struct awk_stream*
find_stream(struct awk_state* st, const char* name) {
  size_t i;

  for(i = 0; i < st->nstreams; i++)
    if(!str_diff(st->streams[i].name, name))
      return &st->streams[i];

  return NULL;
}

static struct awk_stream*
new_stream(struct awk_state* st) {
  if(st->nstreams == st->streamcap) {
    st->streamcap = st->streamcap ? st->streamcap * 2 : 8;
    st->streams = alloc_re(st->streams, st->streamcap * sizeof(struct awk_stream));
  }

  byte_zero(&st->streams[st->nstreams], sizeof(struct awk_stream));
  return &st->streams[st->nstreams++];
}

struct awk_stream*
awk_stream_for_write(struct awk_state* st, const char* name, int append, int is_cmd) {
  struct awk_stream* strm = find_stream(st, name);

  if(strm && strm->is_write)
    return strm;

  if(strm) {
    awk_runtime_error(st, "target is already open for reading");
    return NULL;
  }

  strm = new_stream(st);
  strm->name = str_ndup(name, str_len(name));
  strm->is_write = 1;
  strm->is_cmd = is_cmd;

  if(is_cmd) {
    if(!st->io->run_shell) {
      awk_runtime_error(st, "'| cmd' is not supported in this build");
      return NULL;
    }

    if(st->io->run_shell(st->io->ctx, name, 2, &strm->h, NULL) != 0 || !strm->h) {
      awk_runtime_error(st, "can't run command");
      return NULL;
    }
  } else {
    strm->h = st->io->open_write ? st->io->open_write(st->io->ctx, name, append) : NULL;

    if(!strm->h) {
      awk_runtime_error(st, "can't open file for output");
      return NULL;
    }
  }

  return strm;
}

struct awk_stream*
awk_stream_for_read(struct awk_state* st, const char* name, int is_cmd) {
  struct awk_stream* strm = find_stream(st, name);

  if(strm && !strm->is_write)
    return strm;

  if(strm) {
    awk_runtime_error(st, "target is already open for writing");
    return NULL;
  }

  strm = new_stream(st);
  strm->name = str_ndup(name, str_len(name));
  strm->is_cmd = is_cmd;

  if(is_cmd) {
    if(!st->io->run_shell) {
      awk_runtime_error(st, "'cmd | getline' is not supported in this build");
      return NULL;
    }

    awk_streams_flush_all(st); /* keep output order sane before running a command */

    if(st->io->run_shell(st->io->ctx, name, 1, &strm->h, NULL) != 0)
      strm->h = NULL;
  } else {
    strm->h = st->io->open_read ? st->io->open_read(st->io->ctx, name) : NULL;
  }

  if(strm->h)
    stream_init_reader(st, strm);

  return strm; /* strm->h == NULL: getline reports -1, not fatal */
}

int
awk_stream_close(struct awk_state* st, const char* name) {
  size_t i;

  for(i = 0; i < st->nstreams; i++) {
    struct awk_stream* strm = &st->streams[i];

    if(str_diff(strm->name, name))
      continue;

    if(strm->is_write) {
      if(strm->is_cmd) {
        if(st->io->run_shell)
          st->io->run_shell(st->io->ctx, NULL, 2, &strm->h, NULL);
      } else if(st->io->close_write) {
        st->io->close_write(st->io->ctx, strm->h);
      }
    } else {
      if(strm->is_cmd) {
        if(st->io->run_shell)
          st->io->run_shell(st->io->ctx, NULL, 1, &strm->h, NULL);
      } else if(st->io->close_read) {
        st->io->close_read(st->io->ctx, strm->h);
      }
    }

    alloc_free(strm->name);
    st->streams[i] = st->streams[st->nstreams - 1];
    st->nstreams--;
    return 1;
  }

  return 0;
}

void
awk_streams_close_all(struct awk_state* st) {
  while(st->nstreams)
    awk_stream_close(st, st->streams[0].name);

  alloc_free(st->streams);
  st->streams = NULL;
  st->streamcap = 0;
}

void
awk_streams_flush_all(struct awk_state* st) {
  size_t i;

  awk_output(st, "", 0);
  awk_output_err(st, "", 0);

  for(i = 0; i < st->nstreams; i++)
    if(st->streams[i].is_write && st->io->write)
      st->io->write(st->io->ctx, st->streams[i].h, "", 0);
}

/* ---- NAME=VALUE (-v and file-operand-position assignments) --------- */

static long
find_global_by_name(struct awk_prog* prog, const char* s, size_t len) {
  size_t i;

  for(i = 0; i < prog->nglobals; i++)
    if(str_len(prog->globalnames[i]) == len && !byte_diff(prog->globalnames[i], len, s))
      return (long)i;

  return -1;
}

static int
is_name_start(char c) {
  return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int
is_name_char(char c) {
  return is_name_start(c) || (c >= '0' && c <= '9');
}

int
awk_apply_assignment(struct awk_state* st, const char* s) {
  size_t i;
  long idx;
  char* val;
  size_t vlen;

  if(!s[0] || !is_name_start(s[0]))
    return 0;

  for(i = 1; s[i] && s[i] != '='; i++)
    if(!is_name_char(s[i]))
      return 0;

  if(s[i] != '=')
    return 0;

  idx = find_global_by_name(st->prog, s, i);

  if(idx < 0)
    return 1; /* well-formed, but names nothing the program reads: consume, do nothing */

  val = awk_unescape(&st->tmp, s + i + 1, str_len(s + i + 1), &vlen);

  {
    double v;
    int numeric = awk_looks_numeric(val, vlen, &v);
    awk_cell* c = awk_global(st, idx);

    awk_cell_set_str(c, val, vlen, numeric);

    if(numeric)
      c->num = v;
  }

  return 1;
}

/* ---- main input: ARGV/ARGC, "-", operand assignments --------------- */

static int
open_argv_operand(struct awk_state* st, const char* name) {
  byte_zero(&st->cur, sizeof(st->cur));
  st->cur.name = str_ndup(name, str_len(name));
  st->cur.h = st->io->open_read ? st->io->open_read(st->io->ctx, name) : NULL;

  if(!st->cur.h) {
    static const char msg[] = "awk: can't open ";

    awk_output_err(st, msg, sizeof(msg) - 1);
    awk_output_err(st, name, str_len(name));
    awk_output_err(st, "\n", 1);
    st->exit_status = 2; /* non-fatal: keep going with the next operand */
    alloc_free(st->cur.name);
    st->cur.name = NULL;
    return 0;
  }

  stream_init_reader(st, &st->cur);
  st->cur_open = 1;
  awk_cell_set_str(awk_global(st, SP_FILENAME), name, str_len(name), 0);
  awk_cell_set_num(awk_global(st, SP_FNR), 0);
  return 1;
}

static int
open_next_operand(struct awk_state* st) {
  hashmap* argv = awk_array_of(st, awk_global(st, SP_ARGV));

  if(!argv)
    return 0;

  for(;;) {
    char key[32];
    size_t klen;
    awk_cell* c;
    double argc = awk_tonum(st, awk_global(st, SP_ARGC));

    if((double)st->cur_argi >= argc) {
      if(!st->any_input_used) {
        st->any_input_used = 1;
        return open_argv_operand(st, "-");
      }

      return 0;
    }

    klen = fmt_ulong(key, (unsigned long)st->cur_argi);
    st->cur_argi++;
    c = hashmap_get2(argv, key, klen);

    if(!c || !c->str || !c->str[0])
      continue;

    if(awk_apply_assignment(st, c->str))
      continue;

    st->any_input_used = 1;

    if(open_argv_operand(st, c->str))
      return 1;
    /* couldn't open it: diagnostic already printed, try the next one */
  }
}

static int
main_input_nextline(struct awk_state* st, stralloc* out) {
  for(;;) {
    if(st->cur_open) {
      int r = stream_getline(&st->cur, out);

      if(r == 1) {
        awk_cell_set_num(awk_global(st, SP_NR), awk_tonum(st, awk_global(st, SP_NR)) + 1);
        awk_cell_set_num(awk_global(st, SP_FNR), awk_tonum(st, awk_global(st, SP_FNR)) + 1);
        return 1;
      }

      if(st->cur.h && st->io->close_read)
        st->io->close_read(st->io->ctx, st->cur.h);

      alloc_free(st->cur.name);
      st->cur_open = 0;
      continue;
    }

    if(!open_next_operand(st))
      return 0;
  }
}

int
awk_getrecord(struct awk_state* st) {
  stralloc line;
  int r;

  stralloc_init(&line);
  r = main_input_nextline(st, &line);

  if(r == 1)
    awk_rec_setline(st, line.s ? line.s : "", line.len);

  stralloc_free(&line);
  return r;
}

int
awk_getline_from(struct awk_state* st, struct anode* n, awk_cell** setvarp) {
  int has_var = (n->idx & GL_VAR) != 0;
  int is_file = (n->idx & GL_FILE) != 0;
  int is_cmd = (n->idx & GL_CMD) != 0;
  struct awk_lvalue lv;
  stralloc line;
  int r;

  (void)setvarp;

  if(has_var)
    lv = awk_lvalue(st, n->a);

  if(is_cmd || is_file) {
    awk_cell tv = awk_eval(st, n->b);
    const char* target = awk_tostr(st, &tv, 0);
    struct awk_stream* strm = awk_stream_for_read(st, target, is_cmd);

    if(!strm)
      return -1;

    stralloc_init(&line);
    r = stream_getline(strm, &line);

    if(r == 1) {
      if(has_var)
        awk_lvalue_set_str(st, &lv, line.s ? line.s : "", line.len,
                            awk_looks_numeric(line.s ? line.s : "", line.len, NULL));
      else
        awk_rec_setline(st, line.s ? line.s : "", line.len);

      if(is_cmd)
        awk_cell_set_num(awk_global(st, SP_NR), awk_tonum(st, awk_global(st, SP_NR)) + 1);
    }

    stralloc_free(&line);
    return r;
  }

  if(!has_var)
    return awk_getrecord(st);

  stralloc_init(&line);
  r = main_input_nextline(st, &line);

  if(r == 1)
    awk_lvalue_set_str(st, &lv, line.s ? line.s : "", line.len,
                        awk_looks_numeric(line.s ? line.s : "", line.len, NULL));

  stralloc_free(&line);
  return r;
}
