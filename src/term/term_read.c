#include "byte.h"
#include "prompt.h"
#include "term.h"
#include <termios.h>

static char term_inbuf[BUFFER_INSIZE];
buffer term_input = BUFFER_INIT(0, 0, term_inbuf, sizeof(term_inbuf));

/* ----------------------------------------------------------------------- */
int
term_read(int fd, char* buf, unsigned int len) {
  char c;
  int ret;
  static unsigned long remain;

  if(remain) {
  check_remain:
    if(len > remain)
      len = remain;
    byte_copy(buf, len, &term_cmdline.s[term_cmdline.len - remain]);
    remain -= len;
    if(!remain) {
      tcsetattr(fd, TCSANOW, &term_tcattr);
      stralloc_zero(&term_cmdline);
    }
    return len;
  }

  /*  tcsetattr(fd, TCSANOW, &term_tcattr);*/
  term_attr(term_input.fd, 1);

  prompt_show();

  while((ret = buffer_getc(&term_input, &c)) > 0) {
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
      /* printable chars */
      case '\t': c = ' ';
      default:
        if(term_insert)
          term_insertc(c);
        else
          term_overwritec(c);
        break;
    }
  }
fail:
  return ret;
}
