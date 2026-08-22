#include "../../lib/alloc.h"
#include "../eval.h"
#include "../exec.h"
#include "../expand.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/windoze.h"
#include "builtin_config.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

#if BUILTIN_TRAP
void* trap_snapshot_save(void);
void trap_snapshot_restore(void*);
#endif

/* evaluates backquoted command list, while writing stdout to a stralloc
 * ----------------------------------------------------------------------- */
union node*
expand_command(struct nargcmd* cmd, union node** nptr, int flags) {
  union node* n = *nptr;
  struct vartab vars;
  struct fd fd;
  struct fdstack fdst;
  struct fd_state fdstate;
  struct env she;
  struct eval en;
  int jmpret;
  int ret;
  stralloc sa;
  struct func_snapshot funcs;
#if BUILTIN_TRAP
  void* traps_snap;
#endif
  stralloc_init(&sa);

  /* do this in a new i/o context so we can redirect stdout */

  /* make the output buffer write to the stralloc */
  fdstack_push(&fdst);
  /* the real-kernel-fd bookkeeping (fd_expected, fd_list[], ...) is
     process-global; fdstack_push()/fdstack_pop() scope the struct fd
     entries but not that. A subshell environment that runs in this
     process has to put it back itself -- same pairing eval_subshell()
     uses. */
  fd_state_save(&fdstate);
  fd_push(&fd, STDOUT_FILENO, FD_WRITE);
  fd_subst(&fd, &sa);

  /* evaluate the command tree in a subshell */
  vartab_push(&vars, 0);

  /* command substitution is a subshell (POSIX 2.6.3), same as "(...)":
     push a fresh struct env so "set -e"/"cd"/"umask" etc. run inside
     "$(...)" only affect the substitution's own, discarded-afterward
     environment, not the calling shell's state. */
  sh_push(&she);
  exec_functions_save(&funcs);

#if BUILTIN_TRAP
  /* traps live in a process-global list, so a "trap" run inside
     "$(...)" would otherwise stay installed in the calling shell --
     see trap_snapshot_save() in builtin_trap.c. */
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
    eval_tree(&en, cmd->list, E_LIST);
  }

  ret = eval_pop(&en);

#if BUILTIN_TRAP
  trap_snapshot_restore(traps_snap);
#endif
  exec_functions_restore(&funcs);
  sh_pop(&she);
  vartab_pop(&vars);

  sh->exitcode = ret;
  sh->cmdsubst_ran = 1;

  fdstack_pop(&fdst);
  fd_state_restore(&fdstate);

  /* split trailing newlines */
  while(sa.len && sa.s[sa.len - 1] == '\n')
    sa.len--;

  /* expand the output of the command

     FIXME: we could do this much nicer by doing an
            expand_write() which is set as buffer op
            on the output fd.

            so we won't have to alloc all the stuff twice!
   */
  n = expand_cat(sa.s, sa.len, nptr, flags);
  stralloc_free(&sa);

  return n;
}
