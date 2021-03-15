#ifndef SOURCE_H
#define SOURCE_H

#include "../lib/buffer.h"

struct fd;

struct __attribute__((__packed__)) position {
  unsigned int line, column;
};

struct source {
  buffer* b;
  int mode;
  struct position pos;
  struct source* parent;
};

#define SOURCE_IACTIVE 0x01
#define SOURCE_HERE 0x02

extern struct source* source;
extern int source_psn;

void source_buffer(struct source*, struct fd*, const char* x, size_t n);
void source_pop(void);
void source_popfd(struct fd*);
void source_prompt(void);
int source_peek(char* c);
int source_get(char* c);
int source_next(char* c);
int source_peekn(char* c, unsigned long n);
void source_flush(void);
void source_msg(void);
void source_skip(void);
int source_fork(buffer* child_source);
void source_exec(void);
void source_newline(void);
void source_push(struct source* in);

#endif /* SOURCE_H */
