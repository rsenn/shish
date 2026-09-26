#include "../trace.h"

#ifdef DEBUG_OUTPUT
#include "../eval.h"
#include "../sh.h"
#include "../../lib/byte.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"
#include <errno.h>
#include <stdlib.h>

#if WINDOWS_NATIVE
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define TRACE_LINE 8192
#define TRACE_FD_MIN 200

static const char* const trace_names[TRACE_NMODULES] = {
    "exec", "builtin", "fd", "fdstack", "fdtable", "eval", "redir", "var", "sh", "job", "sig",
};

static char trace_buf[TRACE_LINE];
static size_t trace_len;
static const char *trace_close, *trace_module_name;
static int trace_first, trace_saved_errno, trace_fd = -2, trace_init_done;
static unsigned int trace_mask;

/* "exec,fd,-job" -> bit mask; "all" selects everything */
static void
trace_parse(const char* s) {
  while(*s) {
    size_t n = 0, i;
    int neg = 0;

    while(*s == ',')
      s++;

    if(*s == '-') {
      neg = 1;
      s++;
    }

    while(s[n] && s[n] != ',')
      n++;

    if(n == 3 && !str_diffn(s, "all", 3)) {
      trace_mask = neg ? 0 : ~0u;
    } else {
      for(i = 0; i < TRACE_NMODULES; i++)
        if(str_len(trace_names[i]) == n && !str_diffn(s, trace_names[i], n)) {
          if(neg)
            trace_mask &= ~(1u << i);
          else
            trace_mask |= 1u << i;
        }
    }

    s += n;
  }
}

static void
trace_open(void) {
  const char *sel = getenv("SHISH_TRACE"), *file = getenv("SHISH_TRACE_FILE");

  trace_init_done = 1;

  if(!sel || !*sel)
    return;

  trace_parse(sel);

  if(file && !str_diff(file, "-")) {
    trace_fd = 2;
  } else {
    int fd = open(file && *file ? file : "trace.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

#if !WINDOWS_NATIVE
    /* move it out of the way of the descriptors the shell hands out,
       and don't leak it into exec'd programs */
    if(fd >= 0) {
      int hi = fcntl(fd, F_DUPFD_CLOEXEC, TRACE_FD_MIN);

      if(hi >= 0) {
        close(fd);
        fd = hi;
      }
    }
#endif
    trace_fd = fd;
  }
}

int
trace_begin(enum trace_module mod, const char* event, const char* open, const char* close) {
  char num[32];
  size_t n;

  trace_saved_errno = errno;

  if(!trace_init_done)
    trace_open();

  if(trace_fd < 0 || !(trace_mask & (1u << mod)))
    return 0;

  trace_len = 0;
  trace_first = 1;
  trace_close = close;
  trace_module_name = trace_names[mod];

  trace_put("[", 1);
  #if WINDOWS_NATIVE
  n = fmt_ulong(num, (unsigned long)sh_pid);
#else
  n = fmt_ulong(num, (unsigned long)getpid());
#endif
  trace_put(num, n);
  trace_put(":", 1);
  n = fmt_ulong(num, eval_depth());
  trace_put(num, n);
  trace_put("] ", 2);
  trace_put(trace_module_name, str_len(trace_module_name));
  trace_put(".", 1);
  trace_put(event, str_len(event));
  trace_put(open, str_len(open));
  trace_first = 1;
  return 1;
}

void
trace_end(void) {
  trace_put(trace_close, str_len(trace_close));

  if(trace_len < TRACE_LINE)
    trace_buf[trace_len++] = '\n';
  else
    trace_buf[TRACE_LINE - 1] = '\n';

  {
    ssize_t r = write(trace_fd, trace_buf, trace_len);
    (void)r;
  }

  errno = trace_saved_errno;
}

/* used by trace_value.c: append text to the line being built */
void
trace_put(const char* s, size_t n) {
  if(trace_len + n >= TRACE_LINE - 4) {
    if(trace_len < TRACE_LINE - 4) {
      size_t room = TRACE_LINE - 4 - trace_len;
      byte_copy(trace_buf + trace_len, room, s);
      trace_len += room;
    }
    /* ellipsis, at most once */
    if(trace_len < TRACE_LINE - 1 && trace_buf[trace_len - 1] != '~')
      trace_buf[trace_len++] = '~';
    return;
  }

  byte_copy(trace_buf + trace_len, n, s);
  trace_len += n;
}

/* separator + key= before a value; NULL key writes the bare value */
void
trace_key(const char* key) {
  if(!trace_first)
    trace_put(", ", 2);

  trace_first = 0;

  if(key) {
    trace_put(key, str_len(key));
    trace_put("=", 1);
  }
}

/* is `fd` the descriptor the trace itself writes to? */
int
trace_is_fd(int fd) {
  return fd == trace_fd;
}
#endif /* DEBUG_OUTPUT */
