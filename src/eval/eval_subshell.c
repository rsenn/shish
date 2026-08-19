#include "../fd.h"
#include "../eval.h"
#include "../exec.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../tree.h"
#include "../vartab.h"
#include "builtin_config.h"

#if BUILTIN_TRAP
void* trap_snapshot_save(void);
void trap_snapshot_restore(void*);
#endif

/* evaluate subshell or grouping (3.9.4.1)
 * ----------------------------------------------------------------------- */
int
eval_subshell(struct eval* e, struct ngrp* ngrp) {
  int ret, jmpret;
  struct eval en;
  struct env she;
  struct fdstack io;
  struct fd_state fdst;
  struct vartab vars;
  struct func_snapshot funcs;
#if BUILTIN_TRAP
  void* traps_snap;
#endif

  fdstack_push(&io);
  /* fdstack_push()/fdstack_pop() already scope the struct fd entries
     themselves, but not the real-kernel-fd bookkeeping (fd_expected,
     fd_list[], ...) a *persistent* ("exec") redirection inside this
     subshell mutates -- shish never fork()s for "(...)", so nothing
     else bounds that mutation to the subshell's own lifetime the way
     it would in a real forked child. Save/restore it here, the same
     way vartab_push()/vartab_pop() right below already do for
     variables. See TODO.md, Goal 4, for what this does and does not
     fix. */
  fd_state_save(&fdst);
  vartab_push(&vars, 0);
  sh_push(&she);
  /* function definitions are stored in a process-global list rather than
     scoped to env frames, so subshells leak them into the parent unless we
     snapshot/restore around the eval. See exec_functions_save. */
  exec_functions_save(&funcs);

#if BUILTIN_TRAP
  /* traps are likewise a process-global list -- see trap_snapshot_save
     in builtin_trap.c for why this needs the same treatment. */
  traps_snap = trap_snapshot_save();
#endif

  eval_push(&en, E_ROOT);

  /* set up a long jump so we can exit the subshell and end up just
     after the setjmp call, which will return nonzero in this case */
  en.jump = 1;
  jmpret = setjmp(en.jumpbuf);

  if(jmpret) {
    en.exitcode = (jmpret >> 1);
  } else {
    eval_tree(&en, ngrp->cmds, E_LIST);

    /* an explicit "exit" inside the subshell reaches here via
       eval_exit()'s longjmp, which already runs the subshell's own
       EXIT trap (en.destructor) before jumping back -- see the
       jmpret branch above and eval_exit.c. Falling off the end of the
       subshell body without an explicit "exit" skipped that
       destructor call entirely: the trap either never ran at all, or
       (since nothing uninstalled it either) ran much later instead,
       after the *parent* shell's own next real exit, using the
       parent's fdstack/stdout rather than the subshell's
       (subshell-exit-trap-output-misdirected). Mirror eval_exit()'s
       destructor call here too, while this frame's fdstack/vartab/sh
       context is still current. */
    if(en.destructor)
      en.exitcode = en.destructor(en.exitcode);
  }

  ret = eval_pop(&en);

#if BUILTIN_TRAP
  trap_snapshot_restore(traps_snap);
#endif
  exec_functions_restore(&funcs);
  sh_pop(&she);
  vartab_pop(&vars);
  fdstack_pop(&io);
  fd_state_restore(&fdst);

  sh->exitcode = ret;

  /* an "exit" triggered by a real-signal trap (sh_async_exit, see
     sh.h) landed here via eval_exit()'s longjmp same as any other
     "exit" inside a subshell would -- but unlike an *ordinary*
     synchronous "exit" written directly in the script (which is
     correctly done at this point: bash's own subshells are separate
     processes, so "exit" inside one only ever kills that process),
     this one needs to keep going. shish's "(...)" runs in-process
     rather than forking, so terminating "just this subshell" here
     means nothing more than returning normally and letting the rest
     of the script carry on completely unaffected by the signal --
     exactly the "Ctrl-C doesn't stop ./configure" bug this fixes.
     Every other bit of this subshell's own state is already correctly
     unwound above (funcs/traps/env/vartab/fdstack) at this point, so
     it's safe to just ask again from here: sh_exit() re-runs
     eval_exit(), which finds the *next* enclosing subshell (if any)
     and repeats this same cleanup-then-recheck one level up, or -- if
     there's no further subshell above us -- falls through to its own
     real exit() call, actually terminating the process. Either way
     this call never returns. */
  if((jmpret & 1) && sh_async_exit)
    sh_exit(ret);

  return ret;
}
