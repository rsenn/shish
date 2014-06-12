#ifndef SOURCE_H
#define SOURCE_H

#include <buffer.h>

struct fd;

struct source
{
  buffer       *b;
  int           mode;
  unsigned int  line;
  struct source *parent;
};

#define SOURCE_IACTIVE 0x01
#define SOURCE_HERE    0x02

extern struct source *source;
extern int           source_psn;

int source_string(const char *s, unsigned long len);
void source_pop(void);  
void source_prompt(void);
int source_peek(char *c);
int source_get(char *c);
int source_next(char *c);
int source_peekn(char *c, unsigned long n);
void source_flush(void);
void source_msg(void);
void source_skip(void);
int source_fork(buffer *child_source);
void source_exec(void);
void source_newline(void);
void source_push(struct source *in);
  
#endif /* SOURCE_H */
