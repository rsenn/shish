#ifndef FILTER_H
#define FILTER_H

#include "../lib/buffer.h"

/* what a builtin declares to act as a chained filter, wired straight into a
 * struct fd's read buffer (fd_filter(), src/fd.h) so whatever reads fd_in->r
 * downstream never has to know it isn't a real fd.
 *
 * A typical filter fills in only the declarative half; the framework
 * (filter_open/filter_run/filter_close) does the rest:
 *
 *   struct cat { struct filter_in in; int number_lines; ... }; // in first 
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
 *   output  a file operand the result goes to instead of stdout (uniq's second
 *           operand); non-NULL result: filter_run writes there, filter_open
 *           declines (a chain has no such file). NULL: stdout
 *   each    with ctx->in.each set by setup(), filter_run does not step but calls
 *           each() per operand (once with NULL when there are none); it writes to
 *           ctx->in.sink. filter_open declines. (compress: file -> file.gz)
 *   err_status  exit status for a usage error (grep, sed: 2); 0 means 1
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
  const char* (*output)(void* ctx);
  int (*each)(void* ctx, const char* src); /* in.each: whole-operand work (src NULL: no operands); non-0 fails */
  int err_status;                          /* exit status of a usage error (0: 1) */
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
  unsigned silent : 1;                               /* an operand that cannot be opened is skipped without a message */
  unsigned spilling : 1;                             /* filter_in_line() is mid-line: keep spill across operands */
  unsigned keepempty : 1, empty : 1;                 /* keepempty: an empty operand is not skipped, ready() stops on it with empty set */
  unsigned each : 1;                                 /* setup(): filter_run calls ops->each() per operand, no step() */
  buffer* sink;                                      /* filter_run(): where the result goes; NULL in a chain */
  buffer* upstream;                                  /* what "-" reads */
  buffer* cur;                                       /* NULL: no operand open */
  buffer inb;
  char rbuf[4096];
  char** errargv; /* argv for builtin_error() */
  char* spill;    /* filter_in_line(): a line that straddles two reads */
  size_t spill_len, spill_cap;
  const char *err_arg, *err_msg; /* option()/setup() usage error: printed by filter_run as "cmd: err_arg: err_msg" */
};

/* initializes the input tracking structure.
 *
 *   char**   errargv    error argument vector
 *   char**   files      file operands list
 *   buffer*  upstream   default fallback input stream
 * ----------------------------------------------------------------------- */
void filter_in_init(struct filter_in* in, char** errargv, char** files, buffer* upstream);
int filter_in_ready(struct filter_in* in);
int filter_in_next(struct filter_in* in);
int filter_in_spill(struct filter_in* in, const char* p, size_t n);

/* returns the string name of the file operand currently being read,
 * or "-" if reading from standard input/upstream. */
const char* filter_in_name(const struct filter_in* in); /* operand being read */

/* pulls data from the active source, cycling file operands sequentially on EOF.
 * Returns the number of bytes read, 0 when all sources are done or negative on error. */
ssize_t filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims);

/* one whole line, without its '\n': *p stays valid until the next call, *had_nl says whether
 * a newline ended it (a last line may lack one). Never crosses operands.
 * Returns the length, or -1 when every source is exhausted. */
ssize_t filter_in_line(struct filter_in* in, const char** p, int* had_nl);

/* zero-copy: the longest prefix of the buffered bytes holding at most *lines
 * newlines (ending after the last of them), for a filter that takes N lines.
 * On return *lines is what is still wanted. Returns the length, 0 when every
 * source is exhausted; consume it with filter_in_skip(). */
ssize_t filter_in_peek_lines(struct filter_in* in, const char** p, unsigned long* lines);

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

/* a count operand such as the N of "head -n N": decimal digits only, no overflow.
 * 0 and *out set, or -1 */
int filter_opt_count(const char* s, unsigned long* out);

/* generates builtin_<name>() and <name>_filter from <name>_ops (needs fdtable.h) */
#define FILTER_BUILTIN(name) \
  const struct builtin_filter name##_filter = {&name##_ops}; \
  int builtin_##name(int argc, char* argv[]) { \
    return filter_run(&name##_ops, argc, argv, fd_out->w); \
  }

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
