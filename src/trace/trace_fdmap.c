#include "../trace.h"
#include "../../lib/windoze.h"

#if defined(DEBUG_OUTPUT) && WINDOWS_NATIVE
void
trace_fdmap(const char* event) {
  (void)event;
}
#elif defined(DEBUG_OUTPUT)
#include "../../lib/fmt.h"
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

/* every open real fd of this process, as "N=target" (Linux: readlink /proc/self/fd/N) */
void
trace_fdmap(const char* event) {
  int i;

  if(!trace_begin(TRACE_FDTABLE, event, TRACE_STRUCT_OPEN, TRACE_STRUCT_CLOSE))
    return;

  for(i = 0; i < 256; i++) {
    char key[16], path[32] = "/proc/self/fd/", target[PATH_MAX];
    ssize_t r;

    if(trace_is_fd(i) || fcntl(i, F_GETFD) == -1)
      continue;

    key[fmt_ulong(key, i)] = 0;
    path[14 + fmt_ulong(path + 14, i)] = 0;
    r = readlink(path, target, sizeof(target) - 1);

    if(r > 0) {
      target[r] = 0;
      trace_str(key, target);
    } else {
      trace_str(key, "?");
    }
  }

  trace_end();
}
#endif
