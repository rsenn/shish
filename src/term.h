#ifndef TERM_H
#define TERM_H

#include "../lib/buffer.h"
#include "../lib/stralloc.h"
#include "../lib/windoze.h"

#if WINDOWS_NATIVE
__attribute__((packed)) struct termios {
  int c_iflag, c_oflag, c_cflag, c_lflag;
  int __dummy[12];
};
#else
#include <termios.h>
#endif

struct fd;

extern stralloc term_cmdline;
extern buffer term_input;
extern int term_insert;
extern int term_dumb;
extern unsigned long term_pos;
extern buffer* term_output;
/* set only while term_read() is actually blocked mid-line waiting on
 * more terminal input (i.e. we're genuinely sitting at the prompt) --
 * checked by sh_onsig()'s SIGCHLD handler before it erases/redraws
 * the "current line": a job finishing while we're busy *executing* a
 * command (any command, not just this specific one -- includes every
 * child a command substitution's own pipeline forks) isn't a redraw
 * situation, there's no in-progress prompt line to protect */
extern volatile int term_reading;

extern struct termios term_tcattr;
extern struct winsize term_size;

int term_init(struct fd* input, struct fd* output);
void term_restore(int fd, const struct termios*);
int term_attr(int fd, int set, struct termios*);
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
void term_erase(void);
void term_escape(buffer*, long n, char type);

void term_setline(const char* s, unsigned long len);
char* term_getline(void);
void term_complete(void);

#endif /* TERM_H */
