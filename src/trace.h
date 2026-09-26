#ifndef TRACE_H
#define TRACE_H

/* Evaluator trace: one event per line, written with a single write(2).
 *
 *   [pid:depth] mod.event(k=v, ...)       TRACE()         a call / operation
 *   [pid:depth] mod.event => v            TRACE_RET()     a result
 *   [pid:depth] mod.event { k=v, ... }    TRACE_STRUCT()  a state snapshot
 *
 * Runtime selection (read once, from the process environment):
 *
 *   SHISH_TRACE=exec,fd   modules to trace; "all", or "-name" to exclude
 *   SHISH_TRACE_FILE=f    default "trace.log" (O_APPEND); "-" is stderr
 *
 * Without DEBUG_OUTPUT every macro below expands to nothing.
 * ----------------------------------------------------------------------- */

enum trace_module {
  TRACE_EXEC,
  TRACE_BUILTIN,
  TRACE_FD,
  TRACE_FDSTACK,
  TRACE_FDTABLE,
  TRACE_EVAL,
  TRACE_REDIR,
  TRACE_VAR,
  TRACE_SH,
  TRACE_JOB,
  TRACE_SIG,
  TRACE_NMODULES
};

#ifdef DEBUG_OUTPUT

/* begin an event; returns 0 when the module is not selected. `open` is one of
 * TRACE_CALL_OPEN / TRACE_RET_OPEN / TRACE_STRUCT_OPEN, `close` the matching
 * TRACE_*_CLOSE. errno is preserved across begin..end. */
#define TRACE_CALL_OPEN "("
#define TRACE_CALL_CLOSE ")"
#define TRACE_RET_OPEN " => "
#define TRACE_RET_CLOSE ""
#define TRACE_STRUCT_OPEN " { "
#define TRACE_STRUCT_CLOSE " }"

int trace_begin(enum trace_module mod, const char* event, const char* open, const char* close);
void trace_end(void);

/* line builder internals, used by the value writers */
void trace_put(const char* s, unsigned long n);
void trace_key(const char* key);
int trace_is_fd(int fd);

/* value writers; `key` may be NULL (bare value) */
void trace_str(const char* key, const char* val);
void trace_int(const char* key, long val);
void trace_hex(const char* key, unsigned long val);
void trace_raw(const char* key, const char* text);
void trace_argv(const char* key, char* const* argv);
void trace_flags(const char* key, unsigned long bits, const char* const* names, unsigned int n);

/* fdtable.exec { 0="/dev/null", 1="pipe:[123]", ... }: the real fds of this process */
void trace_fdmap(const char* event);

#define TRACE(mod, ev, ...) \
  do { \
    if(trace_begin(mod, ev, TRACE_CALL_OPEN, TRACE_CALL_CLOSE)) { \
      __VA_ARGS__; \
      trace_end(); \
    } \
  } while(0)

#define TRACE_RET(mod, ev, ...) \
  do { \
    if(trace_begin(mod, ev, TRACE_RET_OPEN, TRACE_RET_CLOSE)) { \
      __VA_ARGS__; \
      trace_end(); \
    } \
  } while(0)

#define TRACE_STRUCT(mod, ev, ...) \
  do { \
    if(trace_begin(mod, ev, TRACE_STRUCT_OPEN, TRACE_STRUCT_CLOSE)) { \
      __VA_ARGS__; \
      trace_end(); \
    } \
  } while(0)

#else

#define TRACE(mod, ev, ...) ((void)0)
#define TRACE_RET(mod, ev, ...) ((void)0)
#define TRACE_STRUCT(mod, ev, ...) ((void)0)
#define trace_fdmap(ev) ((void)0)

#endif /* DEBUG_OUTPUT */

#endif /* TRACE_H */
