#include "../windoze.h"

#if WINDOWS_NATIVE
#include <io.h>
#ifndef __LCC__
#define close _close
#endif
#else
#include <unistd.h>
#endif

#include "../buffer.h"

typedef void deinit_fn_t(buffer*);
void
buffer_close(buffer* b) {
  if(b->deinit)
    ((deinit_fn_t*)b->deinit)(b);
  if(b->fd > 2)
    close(b->fd);
}
