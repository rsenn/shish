#ifndef TERM_H
#define TERM_H

#include "../lib/buffer.h"
#include "../lib/stralloc.h"
#include "../lib/windoze.h"
#include "builtin_config.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

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
/* set only while term_read() is blocked at the prompt: sh_onsig()'s SIGCHLD handler redraws the
 * line only then, not while a command (or a command substitution's child) is running */
extern volatile int term_reading;

extern struct termios term_tcattr;
extern struct winsize term_size;
extern int term_vi_cmd; /* 1 in vi command mode */

int term_init(struct fd* input, struct fd* output);
void term_restore(int fd, const struct termios*);
int term_attr(int fd, int set, struct termios*);
ssize_t term_read(int fd, void* buf, size_t len, void* arg);

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
void term_complete_redraw(void);
#if BUILTIN_HISTORY
void term_search(void);
#else
#define term_search() ((void)0)
#endif
int term_vi_escape(void);
void term_vimode(char c);

#endif /* TERM_H */
