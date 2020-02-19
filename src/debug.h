#ifndef DEBUG_H
#define DEBUG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H*/

#ifdef DEBUG_OUTPUT

#include "../lib/buffer.h"
#include "../lib/stralloc.h"
#include <stdlib.h>

/* some ansi colors
 * ----------------------------------------------------------------------- */
#ifndef COLOR_DEBUG
#define COLOR_DEBUG 0
#endif

#if COLOR_DEBUG
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
#else
#define COLOR_CYAN ""
#define COLOR_GREEN ""
#define COLOR_RED ""
#define COLOR_YELLOW ""
#define COLOR_NONE ""
#endif /* COLOR_DEBUG */

#define DEBUG_EQU " "
#define DEBUG_SEP ","
#define DEBUG_BEGIN " ["
#define DEBUG_END "] "
#define DEBUG_SPACE 2

union node;

void debug_begin(const char* s, int depth);
void debug_end(int depth);
void debug_str(const char* msg, const char* s, int depth, char quote);
void debug_stralloc(const char* msg, stralloc* s, int depth, char quote);
void debug_ulong(const char* msg, unsigned long i, int depth);
void debug_ptr(const char* msg, void* ptr, int depth);
void debug_char(const char* msg, char c, int depth);
void debug_sublist(const char* s, union node* node, int depth);
void debug_subnode(const char* s, union node* node, int depth);
void debug_list(union node* node, int depth);
void debug_unquoted(const char* msg, const char* s, int depth);
void debug_space(int count, int newline);
void debug_node(union node* node, int depth);
void debug_redir(const char* msg, int flags, int depth);
void debug_subst(const char* msg, int flags, int depth);

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
void* debug_realloc(const char* file, unsigned int line, void* ptr, unsigned long size);
void debug_free(const char* file, unsigned int line, void* ptr);
void debug_error(const char* file, unsigned int line, const char* s);

void debug_memory(void);
#endif

#endif /* DEBUG_H */
