#ifndef BUILTIN_FILTER_H
#define BUILTIN_FILTER_H

#include "../../lib/buffer.h"

/* what a builtin declares to act as a chained filter, wired straight into a
 * struct fd's read buffer (fd_filter(), src/fd.h) so whatever reads fd_in->r
 * downstream never has to know it isn't a real fd -- see TODO.md, Goal 13.
 *
 * A typical filter fills in only the declarative half; the framework
 * (filter_open/filter_run/filter_close) does the rest:
 *
 *   struct cat { struct filter_in in; int number_lines; ... };  // in first
 *   const struct filter_ops cat_ops = {.opts = "nb", .size = sizeof(struct cat),
 *                                      .option = cat_option, .step = cat_step};
 *   int builtin_cat(int argc, char* argv[]) { return filter_run(&cat_ops, argc, argv, fd_out->w); }
 *
 * declarative half (ctx is zeroed, `size` bytes, starting with a struct filter_in):
 *
 *   opts    shell_getopt() string; NULL: no options
 *   size    sizeof(ctx)
 *   option  one parsed option; -1 rejects it (usage error)
 *   setup   after options and operands are known (ctx->in is initialised;
 *           an operand it consumes, e.g. a pattern, is taken off ctx->in.files).
 *           0 ok, 1 valid but not streamable (grep -c: filter_open declines,
 *           filter_run still runs), -1 usage error. it must not print or read
 *           input when it returns 1 or -1: a declining chain falls back to
 *           fork()+pipe() and re-runs the same argv. a data error it finds
 *           itself (unreadable input) is printed there and reported via status.
 *   step    yields the next unit as a pointer that stays valid until the next
 *           step() call (filter_step_fn). the consumer's buffer points straight
 *           at the unit: no copy.
 *   status  exit status once step() has returned 0; NULL: ctx->in.had_error
 *   finish  releases what setup() / step() allocated; NULL: nothing
 * ----------------------------------------------------------------------- */
typedef int filter_step_fn(void* ctx, const char** unit, size_t* len);

struct filter_ops {
  const char* opts;
  size_t size;
  int (*option)(void* ctx, int ch);
  int (*setup)(void* ctx);
  filter_step_fn* step;
  int (*status)(void* ctx);
  void (*finish)(void* ctx);
};

/* one indirection so struct builtin_cmd doesn't have to change shape
 * again if this ever needs more than a single ops pointer. */
struct builtin_filter {
  const struct filter_ops* ops;
};

/* ---- helpers shared by the filter-capable builtins (cat, grep, sed, ...) ---- */

/* opens path for reading into b: mmap when possible, else plain read(2)
 * over rbuf (FIFOs and devices cannot be mapped). 0 on success, -1 on error */
int filter_open_file(buffer* b, char* rbuf, size_t rlen, const char* path);

/* feeds the whole content of path to sink(); -1 if it cannot be read */
typedef void filter_sink_fn(void* ctx, const char* s, size_t n);
int filter_copy(const char* path, filter_sink_fn* sink, void* ctx);

/* input side: the file operands in order, or only stdin/upstream ("-")
 * when there are none. an unreadable operand is reported and skipped. */
struct filter_in {
  char** files; /* operand list, NULL: only "-" */
  int i;
  unsigned done_any : 1, had_error : 1, newfile : 1; /* set when an operand was just opened; the user clears it */
  buffer* upstream;                                  /* what "-" reads */
  buffer* cur;                                       /* NULL: no operand open */
  buffer inb;
  char rbuf[4096];
  char** errargv; /* argv for builtin_error() */
};

/* initializes the input tracking structure
 *
 *  errargv      error argument vector
 *  files        file operands list
 *  upstream     default fallback input stream
 */
void filter_in_init(struct filter_in* in, char** errargv, char** files, buffer* upstream);

/* returns the string name of the file operand currently being read,
 * or "-" if reading from standard input/upstream. */
const char* filter_in_name(const struct filter_in* in); /* operand being read */

/* pulls data from the active source, cycling file operands sequentially on EOF.
 * Returns the number of bytes read, 0 when all sources are done or negative on error. */
ssize_t filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims);

/* zero-copy variant of filter_in_get(): points *p at the bytes buffered in the
 * active source and returns how many (0 when all sources are done, <0 on error).
 * They stay valid until filter_in_skip(), which consumes n of them. */
ssize_t filter_in_peek(struct filter_in* in, const char** p);
void filter_in_skip(struct filter_in* in, size_t n);

/* frees up input tracking structure */
void filter_in_close(struct filter_in* in);

/* runs step to completion, writing every unit to out (flushed when the next
 * read would wait): the direct (non-chained) run of a builtin that also
 * offers a filter. ctx starts with a struct filter_in. */
void filter_drain(filter_step_fn* step, void* ctx, buffer* out);

/* the framework behind a filter_ops (see its comment) */

/* parses argv into a fresh ctx reading from upstream; NULL declines (bad
 * option, setup() said not streamable, ...) without having printed anything */
void* filter_open(const struct filter_ops* ops, int argc, char* argv[], buffer* upstream);

/* like filter_open() for a ctx the caller owns (e.g. on its stack), zeroed
 * here. 0 ok, 1 valid but not streamable, -1 usage error. */
int filter_init(const struct filter_ops* ops, void* ctx, int argc, char* argv[], buffer* upstream);

int filter_status(const struct filter_ops* ops, void* ctx);
void filter_close(const struct filter_ops* ops, void* ctx);

/* the whole builtin: init on stdin, drain step() into out, report, release.
 * a usage error prints "invalid option" and returns 1. */
int filter_run(const struct filter_ops* ops, int argc, char* argv[], buffer* out);

#endif
