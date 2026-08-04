#include "../../lib/byte.h"
#include "../job.h"
#include "../prompt.h"
#include "../term.h"
#include "../debug.h"
#include "builtin_config.h"
#include "../../lib/windoze.h"

#if BUILTIN_TRAP
void trap_run_pending(void);
#endif
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#endif
#if !WINDOWS_NATIVE
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#endif

static char term_inbuf[BUFFER_INSIZE];
buffer term_input = BUFFER_INIT(0, 0, term_inbuf, sizeof(term_inbuf));
volatile int term_reading;

#if !WINDOWS_NATIVE
/* runs the work sh_onsig()'s SIGCHLD handler used to do inline (see
 * BUGS: sh-onsig-async-unsafe, fixed as fixes/87) -- called from
 * ordinary context once term_jobwait() below sees job_sigfd[0] wake
 * up, never from the signal handler itself. */
static void
term_jobnotify(void) {
  if(term_output && term_reading) {
    term_erase();
    term_restore(term_input.fd, &term_tcattr);
  }

  /* prints "Done"/"Stopped" banners for whatever job_signal() (run
     from signal context) recorded since the last call */
  job_update();

#if BUILTIN_TRAP
  /* a real-signal trap firing while shish is blocked here waiting for
     terminal input (interactive mode) wakes this select() loop the
     same way SIGCHLD does -- see trap_relay()'s own comment for why
     that byte gets written at all. */
  trap_run_pending();
#endif

  if(term_output && term_reading) {
    term_attr(term_input.fd, 1, &term_tcattr);
    prompt_number--;
    prompt_show();
    buffer_putsa(term_output, &term_cmdline);
    term_escape(term_output, term_cmdline.len - term_pos, 'D');
    buffer_flush(term_output);
  }
}

/* blocks until term_input.fd has data to read, servicing job_sigfd
 * wakeups (one per SIGCHLD) along the way instead of depending on
 * EINTR -- every signal here installs via sig_action() with
 * SA_RESTART unconditionally set, so a bare blocking read() would
 * never even notice the signal arrived. Only called when term_input's
 * buffer is actually empty, i.e. right before buffer_getc() would
 * otherwise block in read() itself. */
static void
term_jobwait(void) {
  for(;;) {
    fd_set rfds;
    int maxfd = term_input.fd;

    FD_ZERO(&rfds);
    FD_SET(term_input.fd, &rfds);

    if(job_sigfd[0] >= 0) {
      FD_SET(job_sigfd[0], &rfds);

      if(job_sigfd[0] > maxfd)
        maxfd = job_sigfd[0];
    }

    if(select(maxfd + 1, &rfds, 0, 0, 0) == -1) {
      if(errno == EINTR)
        continue;

      return;
    }

    if(job_sigfd[0] >= 0 && FD_ISSET(job_sigfd[0], &rfds)) {
      char drain[64];

      while(read(job_sigfd[0], drain, sizeof(drain)) > 0)
        ;

      term_jobnotify();

      if(!FD_ISSET(term_input.fd, &rfds))
        continue;
    }

    return;
  }
}
#endif

/* ----------------------------------------------------------------------- */
ssize_t
term_read(int fd, void* vbuf, size_t len, void* arg) {
  /* the signature above matches buffer_op_proto (lib/buffer.h) exactly
   * -- this function is only ever used as a buffer's read callback
   * (term_init.c), assigned to a buffer_op_proto* field, so unlike an
   * incompatible-signature cast (the previous "int(int,char*,
   * unsigned int)"), calling through that pointer is fully defined
   * rather than UB (found via -fsanitize=function). "arg" is unused,
   * same as it always was under its old, differently-named parameter
   * list -- buffer's read callbacks just don't get one here. */
  char* buf = vbuf;
  char c;
  ssize_t ret;
  static unsigned long remain;
  (void)arg;

  if(remain) {
  check_remain:
    if(len > remain)
      len = remain;
    byte_copy(buf, len, &term_cmdline.s[term_cmdline.len - remain]);
    remain -= len;

    if(!remain) {
#ifdef TCSANOW
      tcsetattr(fd, TCSANOW, &term_tcattr);
#endif
      stralloc_zero(&term_cmdline);
    }
    ret = len;
    goto fail;
  }

  /*  tcsetattr(fd, TCSANOW, &term_tcattr);*/
  term_attr(term_input.fd, 1, &term_tcattr);

  prompt_show();

  term_reading = 1;

  for(;;) {
#if !WINDOWS_NATIVE
    /* only wait when the buffer's actually empty -- that's the only
       point buffer_getc() below would otherwise block in read()
       itself */
    if(term_input.p == term_input.n)
      term_jobwait();
#endif

    if((ret = buffer_getc(&term_input, &c)) <= 0)
      break;

#ifdef DEBUG_OUTPUT_
    debug_char("term_read.c", c);
    debug_nl();
#endif
    switch(c) {
      /* control-c discards the current line */
      case 3: stralloc_zero(&term_cmdline);
      /* newline */
      case '\n':
        term_newline();
        remain = term_cmdline.len;
        goto check_remain;
      /* control-a is HOME */
      case 1: term_home(); break;
      /* control-e is END */
      case 5: term_end(); break;
      /* control-d is EOF */
      case 4:
        if(!term_cmdline.len) {
          buffer_puts(term_output, "EOF");
          buffer_putnlflush(term_output);
          ret = 0;
          goto fail;
        }

        break;
      /* do the ANSI codes */
      case '\033': term_ansi(); break;
      /* backspace */
      case 127:
      case '\b': term_backspace(); break;
      /* tab triggers filename completion */
      case '\t': term_complete(); break;
      /* printable chars */
      default:
        if(term_insert)
          term_insertc(c);
        else
          term_overwritec(c);
        break;
    }
  }
fail:
  term_reading = 0;
#ifdef DEBUG_OUTPUT_
  debug_ulong("term_read.ret", ret, 0);
  debug_nl();
#endif
  return ret;
}
