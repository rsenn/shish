#ifndef TERM_H
#define TERM_H

#include "buffer.h"
#include "stralloc.h"

#include "fd.h"

extern stralloc term_cmdline;
extern buffer term_input;
extern int term_insert;
extern int term_dumb;
extern unsigned long term_pos;
extern buffer* term_output;

extern struct termios term_tcattr;
extern struct winsize term_size;

int term_init(struct fd* input, struct fd* output);
void term_restore(buffer* input);
int term_attr(int fd, int set);
int term_read(int fd, char* buf, unsigned int len);

void term_winsize(void);

void term_insertc(char c);
void term_overwritec(char c);
void term_backspace(void);
void term_delete(void);
void term_home(void);
void term_end(void);
void term_left(unsigned long n);
void term_right(unsigned long n);
void term_newline(void);
void term_ansi(void);
void term_escape(unsigned long n, char type);

void term_setline(const char* s, unsigned long len);
char* term_getline(void);

#endif /* TERM_H */
