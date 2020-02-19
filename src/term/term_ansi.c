#include "../history.h"
#include "../parse.h"
#include "../../lib/scan.h"
#include "../term.h"
#include "../tree.h"

void
term_ansi(void) {
  char c;
  unsigned long num;
  char buf[16];
  unsigned long i;

  /* when an ANSI code is received, then disable dumb terminal mode */
  /*  term_dumb = 0;*/

  if(buffer_getc(&term_input, &c) <= 0)
    return;

  if(c != '[')
    return;

  buf[(i = 0)] = '\0';
  do {
    if(buffer_getc(&term_input, &c) <= 0)
      return;

    if(i < sizeof(buf))
      buf[i++] = c;
  } while(parse_isdigit(c) || c == ';');

  num = 1;
  scan_ulong(buf, &num);

  switch(c) {
  case 'A': history_prev(); break;

  case 'B': history_next(); break;

  case 'C': term_right(num); break;

  case 'D': term_left(num); break;

  case '~':
    if(num == 2)
      term_insert = !term_insert;
    else if(num == 3)
      term_delete();
    else if(num == 7)
      term_home();
    else if(num == 8)
      term_end();
    break;
  }
}
