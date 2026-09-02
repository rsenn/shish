#include "../builtin.h"
#include "../eval.h"
#include "../exec.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../job.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/shell.h"
#include "../tree.h"
#include "../vartab.h"

/* execute a command
 * ----------------------------------------------------------------------- */
int
exec_command(struct command* cmd, int argc, char** argv, enum execflag flag) {
  int ret = 1;

  /* H_PROGRAM handles X_NOWAIT itself below (exec_program() forks and
     registers the job); every other kind (builtin, special builtin,
     function) otherwise just runs in-process regardless of "&", which
     both isn't real backgrounding and used to crash the caller:
     eval_simple_command.c unconditionally dereferences *job_pointer
     after a bgnd command returns, assuming a job_new() happened
     somewhere in here, and for these cases nothing ever created one
     (confirmed crash: "true & echo after", job_pointer still NULL or
     stale from an earlier job). Fork here too, mirroring exactly what
     exec_program()'s X_NOWAIT branch already does. */
  if((flag & X_NOWAIT) && cmd->id != H_PROGRAM) {
    struct job* job = job_new(1);
    pid_t pid;

    job->bgnd = 1;
    pid = job_fork(job, 0, 1);

    if(!pid) {
      flag &= ~X_NOWAIT;
      exit(exec_command(cmd, argc, argv, flag));
    }

    /* interactive-use-only, see eval_node_bgnd.c's matching comment
       (job-start-banner-printed-noninteractively) */
    if(sh->opts.monitor)
      job_banner(job, fd_err->w, JOB_START);

    return 0;
  }

  switch(cmd->id) {
    case H_SBUILTIN:
    case H_BUILTIN:
    case H_EXEC: {
      /* reset shell_optind for shell_getopt() inside builtins */
      shell_optind = 1;
      shell_optofs = 0;

      /* fdtable_open() only resolves a fd struct that's itself
         pending a real open() (FD_OPEN mode) -- it's a no-op for one
         that's instead a "N<&M"-style dup (FD_DUP mode) of some
         *other*, possibly still-unopened fd, which a builtin's
         stdin/stdout/stderr can easily be: a plain command's
         redirections are only ever recorded as parsed, with the real
         open()/dup2() deferred to here (eval_simple_command.c), so
         "cmd 9<in0 8<&9 ... 0<&3" leaves fd_in a dup of fd 9, which
         nothing above this ever asked to actually open. fd_dup()
         already flattened ->dup to that ultimate source, and once
         *it* is open, fd_setfd()'s fdstack_update() fans the real fd
         out to every dup sharing it (including fd_in, via the ->r/->w
         buffer pointer fd_dup() also already aliased directly to the
         source's own) -- so opening the source here is sufficient,
         with no need to also resolve fd_in itself.
         (redir-fd-chain-resolves-to-invalid-fd, fixes/89) */
      /* none of these fdtable_open() results used to be checked at
         all, so a builtin whose own redirection failed here (its real
         open() is deferred all the way to this point -- see the
         comment above) still ran anyway, using whatever fd it already
         had, and still reported whatever exit status *it* felt like
         returning: "echo foo <_no_such_file_" printed "foo" and
         reported "$?" as 0. POSIX requires the command not execute at
         all, and the shell to treat it as a failure
         (redirect-failure-does-not-block-execution-or-set-status).
         sh_error_errno() (inside fdtable_open() itself, on the
         FDTABLE_ERROR path) already prints the actual error message;
         this only adds the missing "so don't run it, and say so"
         half. */
      int redir_failed = 0;

      if(fd_in) {
        if((fd_in->mode & FD_DUP) && fd_in->dup)
          if(fdtable_open(fd_in->dup, FDTABLE_MOVE) == FDTABLE_ERROR)
            redir_failed = 1;

        if(fdtable_open(fd_in, FDTABLE_MOVE) == FDTABLE_ERROR)
          redir_failed = 1;
      }

      if(fd_out) {
        if((fd_out->mode & FD_DUP) && fd_out->dup)
          if(fdtable_open(fd_out->dup, FDTABLE_MOVE) == FDTABLE_ERROR)
            redir_failed = 1;

        if(fdtable_open(fd_out, FDTABLE_MOVE) == FDTABLE_ERROR)
          redir_failed = 1;
      }

      if(fd_err) {
        if((fd_err->mode & FD_DUP) && fd_err->dup)
          if(fdtable_open(fd_err->dup, FDTABLE_MOVE) == FDTABLE_ERROR)
            redir_failed = 1;

        if(fdtable_open(fd_err, FDTABLE_MOVE) == FDTABLE_ERROR)
          redir_failed = 1;
      }

      exec_redir_error = redir_failed;
      ret = redir_failed ? 1 : cmd->builtin->fn(argc, argv);
      break;
    }

    case H_FUNCTION: {
      struct env inst;
      struct eval e;
      struct vartab vars;

      vartab_push(&vars, 1);

      sh_push(&inst);
      inst.arg.v = argv;

      for(inst.arg.c = 0; argv[inst.arg.c]; inst.arg.c++)
        ;

      inst.arg.v++;
      inst.arg.c--;

      // sh_setargs(argv, 0);
      eval_push(&e, E_FUNCTION);
      sh->eval = &e;

      if((ret = setjmp(e.jumpbuf)) == 0) {
        e.jump = 1;
        /* Use eval_cmdlist's/eval_subshell's return (last command's
           status) rather than eval_pop's e->exitcode, which is only
           set on longjmp/return. Without this, `f() { (exit 1); }; f`
           returns 0 because the subshell longjmps to ITS OWN E_ROOT,
           so e->exitcode stays at 0.

           A function whose body is "(...)" rather than "{...}" must
           get the same subshell isolation (variable assignments,
           etc. not surviving the call) a bare "(...)" gets -- dispatch
           on the body's node kind the same way eval_command.c does
           for a standalone grouping, instead of always going through
           eval_cmdlist(), which runs in the current environment with
           no isolation at all.
           
           POSIX allows any compound command as a function body, not just
           {...} or (...). For loops, while/until loops, if statements,
           and case statements should be evaluated directly. */
        switch(cmd->fn->id) {
          case N_SUBSHELL:
            ret = eval_subshell(&e, &cmd->fn->ngrp);
            break;
          case N_BRACEGROUP:
          case N_LIST:
            ret = eval_cmdlist(&e, &cmd->fn->ngrp);
            break;
          case N_FOR:
            ret = eval_for(&e, &cmd->fn->nfor);
            break;
          case N_WHILE:
          case N_UNTIL:
            ret = eval_loop(&e, &cmd->fn->nloop);
            break;
          case N_IF:
            ret = eval_if(&e, &cmd->fn->nif);
            break;
          case N_CASE:
            ret = eval_case(&e, &cmd->fn->ncase);
            break;
          default:
            /* Fallback: try eval_cmdlist for other node types */
            ret = eval_cmdlist(&e, &cmd->fn->ngrp);
            break;
        }
        e.exitcode = ret;

        eval_pop(&e);
      } else {
        ret >>= 1;
        eval_pop(&e);
      }

      sh_pop(&inst);
      vartab_pop(&vars);

      break;
    }

    case H_PROGRAM: {
      ret = exec_program(cmd->path, argv, flag);
      break;
    }
  }

  /* POSIX requires special builtins to kill the shell on error in
     non-interactive mode -- sh_interactive (sh.h), the whole
     session's own interactive-ness, not source->mode's per-buffer
     SOURCE_IACTIVE bit, which source_push() resets for every nested
     source and would otherwise make this fire inside any `.`-sourced
     file even when the real session is interactive. */
  if(cmd->id == H_SBUILTIN && ret != 0 && !sh_interactive) {
    sh_exit(ret);
  }

  /* if exec is set we never return! */
  if(flag & X_EXEC)
    sh_exit(ret);

  return ret;
}
