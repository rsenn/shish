#ifndef HISTORY_H
#define HISTORY_H

#include "../lib/buffer.h"
#include "../lib/stralloc.h"
#include <stdlib.h>

#define DEFAULT_HISTSIZE 128

#define HISTORY (&history_name[5])

extern char** history_array;
extern unsigned int history_size;
extern unsigned int history_count;
extern unsigned int history_offset;
extern unsigned int history_mapped;
extern const char history_name[];
extern const char* history_files[];
extern buffer history_buffer;

unsigned long history_cmdlen(const char* b);

void history_resize(void);
void history_clear(void);
void history_print(void);
void history_advance(void);
void history_set(char* s);
void history_free(unsigned int index);

void history_next(void);
void history_prev(void);

void history_load(void);
void history_save(void);

#endif /* HISTORY_H */
