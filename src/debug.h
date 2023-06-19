#ifndef DEBUG_H
#define DEBUG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H*/

#include "../lib/uint64.h"
#include "../lib/buffer.h"
#include "../lib/str.h"
#include "../lib/stralloc.h"
#include "../lib/open.h"
#include "fd.h"
#include <stdlib.h>

extern buffer debug_buffer, *debug_output;
extern int debug_nindent;
extern char debug_quote;

/* some ansi colors
 * ----------------------------------------------------------------------- */
#ifndef COLOR_DEBUG
#define COLOR_DEBUG 0
#endif

#if COLOR_DEBUG && !DEBUG_NOCOLOR
#define COLOR_CYAN "\033[36;1m"
#define COLOR_PURPLE "\033[38;5;89m"
#define COLOR_VIOLET "\033[38;5;55m"
#define COLOR_MAGENTA "\033[38;5;199m"
#define COLOR_DARKGREEN "\033[0;32m"
#define COLOR_GREEN "\033[32;1m"
#define COLOR_RED "\033[31;1m"
#define COLOR_YELLOW "\033[33;1m"
#define COLOR_BLUE "\033[34;1m"
#define COLOR_LIGHTBLUE "\033[38;5;33m"
#define COLOR_DARKBLUE "\033[0;34m"
#define COLOR_WHITE "\033[37;1m"
#define COLOR_NONE "\033[0m"
#define COLOR_DARKGRAY "\033[38;5;238m"

#else
#define COLOR_CYAN ""
#define COLOR_PURPLE ""
#define COLOR_VIOLET ""
#define COLOR_MAGENTA ""
#define COLOR_DARKGREEN ""
#define COLOR_GREEN ""
#define COLOR_RED ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_LIGHTBLUE ""
#define COLOR_DARKBLUE ""
#define COLOR_WHITE ""
#define COLOR_NONE ""
#define COLOR_DARKGRAY ""
#endif /* COLOR_DEBUG */

#define CURSOR_FORWARD(n) "\x1b[" #n "C"
#define CURSOR_HORIZONTAL_ABSOLUTE(n) "\x1b[" #n "G"

#define DEBUG_EQU ": "
#define DEBUG_SEP ","
#define DEBUG_BEGIN "["
#define DEBUG_END "]"
#define DEBUG_SPACE 2

union node;
struct location;

void debug_begin(const char* s, int depth);
void debug_end(int depth);
void debug_str(const char* msg, const char* s, int depth, char quote);
void debug_stralloc(const char* msg, stralloc* s, int depth, char quote);
void debug_ulong(const char* msg, uint64 i, int depth);
void debug_xlong(const char* msg, uint64 i, int depth);
void debug_ptr(const char* msg, void* ptr, int depth);
void debug_char(const char* msg, char c);
void debug_sublist(const char* s, union node* node, int depth);
void debug_subnode(const char* s, union node* node, int depth);
void debug_list(union node* node, int depth);
void debug_unquoted(const char* msg, const char* s, int depth);
void debug_space(int count, int newline);
void debug_node(union node* node, int depth);
void debug_redir(const char* msg, int flags, int depth);
void debug_subst(const char* msg, int flags);
void debug_position(const char* msg, const struct location* pos, int depth);
void debug_location(const char* msg, const struct location* pos, int depth);

size_t debug_argv(char** argv, buffer* out);
void debug_squoted(const char*, size_t n, buffer* out);

#ifdef DEBUG_OUTPUT

#define debug_to(buf) debug_output = buf;
#define debug_s(str) buffer_puts(debug_output, str)
#define debug_n(num) buffer_putlonglong(debug_output, num)
#define debug_xn(num) buffer_putxlonglong(debug_output, num)
#define debug_c(chr) \
  buffer_putc(debug_output, (unsigned int)(unsigned char)(chr))
#define debug_b(buf, len) buffer_put(debug_output, (buf), (len))
#define debug_ws(str) debug_c(' ')
#define debug_nl() debug_c('\n') //
#define debug_fl() buffer_flush(debug_output)
#define debug_nl_fl() buffer_putnlflush(debug_output) //(debug_nl(), debug_fl())
#define debug_fn() (debug_s(__func__), debug_s("()"))
#define debug_fn_ws() (debug_fn(), debug_ws())
#define debug_fn_nl() (debug_fn(), debug_nl())
#define debug_fn_nf() (debug_fn(), debug_nl_fl())

#else

#define debug_s(str)
#define debug_n(num)
#define debug_xn(num)
#define debug_c(chr)
#define debug_b(buf, len)
#define debug_ws(str)
#define debug_nl()
#define debug_fl()
#define debug_nl_fl()
#define debug_fn()
#define debug_fn_ws()
#define debug_fn_nl()
#define debug_fn_nf()

#endif /* DEBUG_OUTPUT */

#if DEBUG_ALLOC

struct chunk {
  struct chunk* next;
  struct chunk** pos;
  unsigned long size;
  const char* file;
  unsigned int line;
};

extern struct chunk* debug_heap;
extern struct chunk** debug_pos;

void* debug_alloc(const char* file, unsigned int line, unsigned long size);
void* debug_realloc(const char* file,
                    unsigned int line,
                    void* ptr,
                    unsigned long size);
void debug_free(const char* file, unsigned int line, void* ptr);
void debug_error(const char* file, unsigned int line, const char* s);

void debug_memory(void);
#endif

static inline int
dump_flags(buffer* b, int bits, const char* const names[], int pad) {
  size_t i, n = 0;
  for(i = 0; i < sizeof(bits) * 8; i++) {
    if(bits & (1 << i)) {
      size_t len = str_len(names[i]);
      if(n) {
        n++;
        buffer_puts(b, COLOR_CYAN "|" COLOR_NONE);
      }
      buffer_put(b, names[i], len);
      n += len;
    }
  }
  if(n < (size_t)pad)
    buffer_putnspace(b, pad - n);
  return n;
}

static inline int
debug_flags(int bits, const char* const names[]) {
  return dump_flags(debug_output, bits, names, 0);
}

static inline void
debug_open() {
  if(debug_buffer.fd == -1)
    debug_buffer.fd = open_trunc("debug.log");
}

static inline void
debug_indent(int depth) {
  depth *= debug_nindent;
  while(depth-- > 0)
    debug_c(' ');
}

static inline void
debug_newline(int depth) {
  if(depth >= 0 && debug_nindent > 0) {
    debug_c('\n');
    debug_indent(depth);
  } else {
    debug_c(' ');
  }
}

static inline void
debug_field(const char* s, int depth) {
  if(s[str_chr(", ", *s)]) {
    if(*s == ',')
      debug_c(',');

    if(depth >= 0)
      debug_newline(depth);
    else
      debug_c(' ');

    if(*s == ',')
      s++;
    if(*s == ' ')
      s++;
  }
  debug_c('"');
  debug_s(s);
  debug_c('"');
  debug_s(COLOR_CYAN ": ");
}

#endif /* DEBUG_H */
