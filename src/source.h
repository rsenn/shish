#ifndef SOURCE_H
#define SOURCE_H

#include "../lib/buffer.h"
#include "../lib/fmt.h"
#include "../lib/shell.h"

struct fd;

struct __attribute__((__packed__)) location {
  unsigned int line, column;
  size_t offset;
};

struct source {
  buffer* b;
  unsigned mode;
  struct source* parent;
  struct location position;
  /* fd this source frame owns and must fd_pop() when popped; NULL for
     sources (e.g. the top-level script/-c source) with no owned fd. */
  struct fd* fd;
};

#define SOURCE_IACTIVE 0x01
#define SOURCE_HERE 0x02

extern struct source* source;
extern int source_psn;

/* set (and cleared) by parse_squoted.c: tells source_skip.c/
 * source_peekn.c to stop treating "\<newline>" as a line continuation
 * to remove, since single quotes must preserve every character
 * literally. */
extern int source_squoted;
extern int source_comment;

void source_buffer(struct source*, struct fd*, const char* x, size_t n);
void source_pop(void);
void source_popfd(struct fd*);
void source_prompt(void);
int source_peek(char* c);
int source_get(char* c);
int source_next(char* c);
int source_peekn(char* c, unsigned int n);
void source_flush(void);
void source_msg(const struct location* pos);
int source_skip(void);
int source_skipn(int n);
int source_fork(buffer* child_source);
void source_exec(void);
void source_newline(void);
void source_push(struct source* in);
int source_peeknc(unsigned pos);
int source_peekc(void);

#define source_PEEKN(n) (char)source_peeknc(n);

#define FMT_LOC (FMT_ULONG * 2 + 1)

size_t fmt_loc(char* dest, const struct location* loc);
const char* location2str(const struct location loc);
size_t fmt_location(char*, const struct location);

#endif /* SOURCE_H */
