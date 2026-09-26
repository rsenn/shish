#include "../trace.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

static const char* const trace_fd_modes[32] = {
    "READ",   "WRITE", "APPEND",   "EXCL",    "TRUNC", 0,        0,        0,
    "FILE",   "DIR",   "LINK",     "CHAR",    "BLOCK", "SOCKET", "PIPE",   "STRALLOC",
    "STRING", "DUP",   "TERM",     "NULL",    0,       0,        0,        0,
    "FLUSH",  "CLOSE", "FREENAME", "DUPNAME", "FREE",  0,        "TMPBUF", "OPEN",
};

/* {n=1, name="pipe", level=0, e=1, mode=WRITE|PIPE|TMPBUF, rfd=-1, wfd=1} */
void
trace_fd(const char* key, struct fd* d) {
  if(!d) {
    trace_str(key, NULL);
    return;
  }

  trace_open(key, "{");
  trace_int("n", d->n);
  trace_str("name", d->name);
  trace_int("level", d->stack ? (long)d->stack->level : -1);
  trace_int("e", d->e);
  trace_flags("mode", (unsigned long)d->mode, trace_fd_modes, 32);
  trace_int("rfd", d->rb.fd);
  trace_int("wfd", d->wb.fd);

  if((d->mode & FD_DUP) && d->dup)
    trace_int("dup", d->dup->n);

  trace_close("}");
}

/* fdtable.<event>(vfd=, shadow=, fd={...}) for every fd in the table, innermost first */
void
trace_fdtable(const char* event) {
  int i;

  fdtable_foreach(i) {
    struct fd* d;
    int depth = 0;

    for(d = fdtable[i]; d; d = d->parent, depth++)
      TRACE(TRACE_FDTABLE, event, trace_int("vfd", i), trace_int("shadow", depth), trace_fd("fd", d));
  }
}
#endif
