#include "term.h"

/* ----------------------------------------------------------------------- */
void term_home(void) {
  if(!term_pos)
    return;

  term_left(term_pos);
}

