#include "../eval.h"
#include "../tree.h"
#include "../sh.h"

/* evaluate while/until-loop (3.9.4.5 - 3.9.4.6)
 * ----------------------------------------------------------------------- */
int
eval_loop(struct eval* e, struct nloop* nloop) {
  struct eval en;
  int retcode = (nloop->id == N_WHILE) ? 0 : 1;
  int ret = 0;
  int ran = 0;
  int status = 0;

  eval_push(&en, E_LOOP);

  en.jump = 1;

  /* POSIX 2.9.4: a while/until loop's own exit status is that of the
     last command run in its *body* (0 if the body never ran, e.g. the
     test failed immediately) -- not "ret" (the *test*'s own status,
     the only thing this function ever tracked) and not eval_pop(&en)'s
     always-0 "en.exitcode" (never assigned, same bug as eval_for()'s).
     e->exitcode already holds exactly that value, since the body runs
     against the caller's "e" -- both the "break" jump target below and
     the normal loop-exit path now return it instead, also syncing the
     global sh->exitcode immediately (exactly like eval_subshell() does
     for "(...)"): otherwise a "$?" expanded by a *later* command still
     sharing this same top-level list read whatever the *test*'s or the
     body's own last command had left sh->exitcode at directly, rather
     than this compound command's own, just-computed aggregate result. */
  if(setjmp(en.jumpbuf) & 1) {
    eval = e;
    return sh->exitcode = e->exitcode;
  }

  for(;;) {
    /* the controlling list of a while/until is exempt from "set -e"
       in its entirety -- see errexit_suppress's own comment (eval.h). */
    errexit_suppress++;
    ret = eval_tree(e, nloop->test, E_LIST);
    errexit_suppress--;

    if(ret != retcode)
      break;

    /* evaluate loop body */
    eval_tree(e, nloop->cmds, E_LIST);
    ran = 1;
    /* snapshot right away -- the *next* iteration's test re-run
       (above) also executes against this same "e" and overwrites
       e->exitcode with its own (by definition different, or the loop
       would never terminate) result before we get back around to
       checking "ran" below, otherwise leaving the loop's reported
       status as that of its final, loop-ending *test* instead of its
       last real body command. */
    status = e->exitcode;
  }

  if(ran)
    en.exitcode = status;

  return sh->exitcode = eval_pop(&en);
}
