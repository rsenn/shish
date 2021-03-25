#include "../windoze.h"
#include "../buffer.h"
#include "../fmt.h"
#include "../stralloc.h"

#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

extern ssize_t buffer_dummyread();
extern ssize_t buffer_dummyreadmmap();
extern int stralloc_write(int, const char*, size_t, buffer*);

void
buffer_dump(buffer* out, buffer* b) {
  char xlong[FMT_LONG + FMT_LONG + FMT_LONG];
  unsigned long n;
#define COLOR_DUMP
#ifndef COLOR_DUMP
#define RED ""
#define GREEN ""
#define YELLOW ""
#define CYAN ""
#define MAGENTA ""
#define NONE ""
#else
#define RED "\033[1;31m"
#define GREEN "\033[38;5;34m"
#define LIGHTGREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define DARKYELLOW "\033[38;5;220m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"
#define NONE "\033[0m"
#define ORANGE "\033[38;5;202m"
#define PURPLE "\033[38;5;93m"
#define DARKGRAY "\033[38;5;238m"
#endif

  buffer_puts(out, "[ ");
  if(b->p) {
    buffer_puts(out, "p" DARKGRAY "=" GREEN);
    buffer_putlong(out, b->p);
  }
  if(b->n) {
    buffer_puts(out, b->p ? NONE ", n" DARKGRAY "=" CYAN : "n" DARKGRAY "=" CYAN);
    buffer_putlong(out, b->n);
  }
  if(b->a) {
    buffer_puts(out, (b->p || b->n) ? NONE ", a" DARKGRAY "=" CYAN : "a" DARKGRAY "=" CYAN);
    buffer_putlong(out, b->a);
  }
  if(b->x && (b->p || b->n)) {

    buffer_puts(out, (b->p || b->n || b->a) ? NONE ", x" CYAN "@p" DARKGRAY "=" NONE : "x" CYAN "@p" DARKGRAY "=" CYAN);

    if(b->p > 6) {
      n = b->p;
      buffer_puts(out, "...");
    } else {
      n = 0;
    }
    buffer_puts(out, "\"");
    buffer_puts(out, "\"");
  }

  buffer_puts(out, (b->p || b->n || b->a || b->x) ? NONE ", fd" DARKGRAY "=" DARKYELLOW : "fd" DARKGRAY "=" DARKYELLOW);
  buffer_put(out, xlong, fmt_long(xlong, b->fd));
  buffer_puts(out, NONE ", op" DARKGRAY "=");

  if(b->op == (void*)&read) {
    buffer_puts(out, ORANGE "read" NONE);
  } else if(b->op == (void*)&write) {
    buffer_puts(out, ORANGE "write" NONE);
  } else if(b->op == (void*)&buffer_dummyread) {
    buffer_puts(out, ORANGE "buffer_dummyread" NONE);
  } else if(b->op == (void*)&buffer_dummyreadmmap) {
    buffer_puts(out, ORANGE "buffer_dummyreadmmap" NONE);
  } else if(b->op == (void*)&stralloc_write) {
    stralloc* sa = b->cookie;
    buffer_puts(out, ORANGE "stralloc_write" NONE " ");
    stralloc_dump(sa, out);
  } else if(b->op == (void*)NULL) {
    buffer_puts(out, ORANGE "0" NONE);
  } else {
    buffer_putptr(out, (void*)b->op);
  }
  buffer_puts(out, " ]");
  // buffer_flush(out);
}
