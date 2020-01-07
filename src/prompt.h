#ifndef PROMPT_H
#define PROMPT_H

#include "stralloc.h"

extern int prompt_number;
extern union node* prompt_node;
extern char prompt_var[4];
extern stralloc prompt_expansion;

void prompt_escape(const char* s, stralloc* sa);
void prompt_parse(void);
void prompt_expand(void);
void prompt_reset(void);
void prompt_show(void);

#endif /* PROMPT_H */
