#include "../../lib/alloc.h"
#include "../eval.h"
#include "../expand.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* evaluates backquoted command list, while writing stdout to a stralloc
 * ----------------------------------------------------------------------- */
union node*
expand_command(struct nargcmd* cmd, union node** nptr, int flags) {
  union node* n = *nptr;
  struct vartab vars;
  struct fd fd;
  struct fdstack fdst;
  struct eval en;
  int jmpret;
  stralloc sa;
  stralloc_init(&sa);

  /* do this in a new i/o context so we can redirect stdout */

  /* make the output buffer write to the stralloc */
  fdstack_push(&fdst);
  fd_push(&fd, STDOUT_FILENO, FD_WRITE);
  fd_subst(&fd, &sa);

  /* evaluate the command tree in a subshell */
  vartab_push(&vars, 0);

  eval_push(&en, E_ROOT);

  /* set up a long jump so we can exit the subshell and end up just
     after the setjmp call, which will return nonzero in this case */
  en.jump = 1;
  jmpret = setjmp(en.jumpbuf);

  if(jmpret) {
    en.exitcode = (jmpret >> 1);
  } else {
    eval_tree(&en, cmd->list, E_LIST);
  }

  sh->exitcode = eval_pop(&en);

  vartab_pop(&vars);

  fdstack_pop(&fdst);

  /* split trailing newlines */
  while(sa.len && sa.s[sa.len - 1] == '\n')
    sa.len--;

  /* expand the output of the command

     FIXME: we could do this much nicer by doing an
            expand_write() which is set as buffer op
            on the output fd.

            so we won't have to alloc all the stuff twice!
   */
  n = expand_cat(sa.s, sa.len, nptr, flags & (~(X_QUOTED)));
  stralloc_free(&sa);

  return n;
}
