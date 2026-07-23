#include "../fdtable.h"
#include "../fd.h"
#include "../term.h"
#include <signal.h>
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#include <unistd.h>
#else
#include <io.h>
#endif

/* other shells seem to read char by char in interactive/terminal-mode.
   we buffer at least 64 bytes, so we can read ANSI-codes at once. */
static char term_buffer[64];

stralloc term_cmdline;
unsigned long term_pos;
int term_insert = 1;
int term_dumb = 1;
char term_obuf[128];
static buffer term_default_output = BUFFER_INIT(&write, 1, term_obuf, sizeof(term_obuf));

buffer* term_output = 0; //&term_default_output;

/* tries to get terminal attributes and if it succeeds it will make the src
 * buffer use the term_read() function which is, like bloaty readline(),
 * the interface to the whole terminal line editing stuff.
 * returns 1 on success
 * ----------------------------------------------------------------------- */
int
term_init(struct fd* input, struct fd* output) {
  term_output = output->w;

  /* input and stderr must both be char devices */
  if((input->mode & output->mode & FD_CHAR) == 0)
    return 0;

  if(term_attr(input->r->fd, 0, &term_tcattr) == 0) {
    int i;

    /* set terminal flags on all fds that are char devices
       and have the same inode */
    fdtable_foreach(i) {
      if((fdtable[i]->mode & FD_CHAR) && fdtable[i]->dev == input->dev) {
        fdtable[i]->mode |= FD_TERM;
        fdtable[i]->mode &= ~FD_CHAR;
      }
    }
  }

  if((input->mode & output->mode & FD_TERM) == 0)
    return 0;

  /* intercept input buffer */
  buffer_init(&term_input, input->r->op, input->r->fd, term_buffer, sizeof(term_buffer));
  // term_output = output->w;

  input->r->op = (ssize_t (*)())(void*)&term_read;

  /* get window size */
  term_winsize();
#ifdef SIGTTIN
  signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
  signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, (void*)&term_winsize);
#endif

  return 1;
}
