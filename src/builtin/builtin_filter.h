#ifndef BUILTIN_FILTER_H
#define BUILTIN_FILTER_H

#include "../../lib/buffer.h"

/* the interface a builtin exposes to act as a chained filter, wired
 * straight into a struct fd's read buffer (fd_filter(), src/fd.h) so
 * whatever reads fd_in->r downstream never has to know it isn't a
 * real fd -- see TODO.md, Goal 13.
 *
 *   open    parses argv itself (argc/argv exactly as the builtin's
 *           own main entry point would receive them) and decides
 *           whether *this* invocation streams. Returns an opaque ctx
 *           on success. Returns NULL only for a side-effect-free
 *           reason (unsupported flags for this builtin, e.g. grep -c/
 *           -q) -- the caller then falls back to fork()+pipe() and
 *           re-runs the same argv through the builtin's normal entry
 *           point, so open() must never have printed anything or
 *           produced output before returning NULL. A genuine usage
 *           error (bad pattern, missing script, ...) is NOT reported
 *           by returning NULL: it is reported once, from here, and
 *           carried inside the returned ctx instead (read() reports
 *           immediate EOF, status() reports the real failure code) --
 *           see each builtin's *_filter_open() for the exact cutoff.
 *   step    yields the next unit as a pointer that stays valid until
 *           the next step() call (see filter_step_fn). the consumer's
 *           buffer points straight at the unit: no copy. preferred.
 *   read    only when there is no step: buffer_op_proto-shaped (fd is
 *           unused, always -1), fills up to len bytes of buf, returns the
 *           count, or 0 at EOF. A short return is fine -- the caller's own
 *           buffer layer re-invokes read() as needed (lib/buffer.h).
 *           leave NULL when step is set.
 *   status  the builtin's real exit status, valid once read() has
 *           returned 0. Meaningless before that (a chain member that
 *           never reaches EOF -- e.g. its consumer stopped early --
 *           never gets a final status; see TODO.md Goal 13).
 *   close   releases ctx and anything it still owns (an open upstream
 *           file, a compiled pattern, ...). Always called exactly
 *           once, whether or not read() ever reached EOF.
 * ----------------------------------------------------------------------- */
typedef int filter_step_fn(void* ctx, const char** unit, size_t* len);

struct filter_ops {
  void* (*open)(int argc, char* argv[], buffer* upstream);
  ssize_t (*read)(int fd, void* buf, size_t len, void* arg);
  int (*status)(void* ctx);
  void (*close)(void* ctx);
  filter_step_fn* step;
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

/* runs step to completion, writing and flushing every unit to out: the
 * direct (non-chained) run of a builtin that also offers a filter. */
void filter_drain(filter_step_fn* step, void* ctx, buffer* out);

#endif
