#ifndef BUILTIN_FILTER_H
#define BUILTIN_FILTER_H

#include "../lib/buffer.h"

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
 *   read    buffer_op_proto-shaped (fd is unused, always -1): fills
 *           up to len bytes of buf, returns the count, or 0 at EOF.
 *           A short return is fine -- the caller's own buffer layer
 *           already re-invokes read() as needed (lib/buffer.h).
 *   status  the builtin's real exit status, valid once read() has
 *           returned 0. Meaningless before that (a chain member that
 *           never reaches EOF -- e.g. its consumer stopped early --
 *           never gets a final status; see TODO.md Goal 13).
 *   close   releases ctx and anything it still owns (an open upstream
 *           file, a compiled pattern, ...). Always called exactly
 *           once, whether or not read() ever reached EOF.
 * ----------------------------------------------------------------------- */
struct filter_ops {
  void* (*open)(int argc, char* argv[], buffer* upstream);
  ssize_t (*read)(int fd, void* buf, size_t len, void* arg);
  int (*status)(void* ctx);
  void (*close)(void* ctx);
};

/* one indirection so struct builtin_cmd doesn't have to change shape
 * again if this ever needs more than a single ops pointer. */
struct builtin_filter {
  const struct filter_ops* ops;
};

#endif
