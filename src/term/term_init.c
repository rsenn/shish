#include <signal.h>
#include <termios.h>
#include "fdtable.h"
#include "term.h"

/* other shells seem to read char by char in interactive/terminal-mode.
   we buffer at least 64 bytes, so we can read ANSI-codes at once. */
static char    term_buffer[64];

stralloc       term_cmdline;
unsigned long  term_pos;
int            term_insert = 1;
int            term_dumb = 1;
buffer        *term_output;
buffer         term_input;

/* tries to get terminal attributes and if it succeeds it will make the src
 * buffer use the term_read() function which is, like bloaty readline(),
 * the interface to the whole terminal line editing stuff.
 * returns 1 on success
 * ----------------------------------------------------------------------- */
int term_init(struct fd *input, struct fd *output) {
  term_output = output->w;

  /* input and stderr must both be char devices */
  if((input->mode & output->mode & FD_CHAR) == 0)
    return 0;

  if(term_attr(input->r->fd, 0) == 0) {
    unsigned int i;

    /* set terminal flags on all fds that are char devices
       and have the same inode */
    fdtable_foreach(i) {
      if((fdtable[i]->mode & FD_CHAR) && fdtable[i]->dev == input->dev) {
        fdtable[i]->mode |= FD_TERM;
        fdtable[i]->mode &= ~FD_CHAR;
      }
    }
  }

  if((input->mode & output->mode & FD_TERM))
    return 0;

  /* intercept input buffer */
  buffer_init(&term_input, input->r->op, input->r->fd,
              term_buffer, sizeof(term_buffer));

  input->r->op = (ssize_t(*)())term_read;

  /* get window size */
  term_winsize();
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGWINCH, (void *)&term_winsize);

  /*    signal(SIGINT, (void *)&term_controlc);*/
  return 1;
}
