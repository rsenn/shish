#include "../builtin.h"
#include "../eval.h"
#include "../exec.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../tree.h"
#include "../vartab.h"

/* execute a command
 * ----------------------------------------------------------------------- */
int
exec_command(struct command* cmd, int argc, char** argv, int exec, union node* redir) {
  int ret = 1;

  switch(cmd->id) {
    case H_SBUILTIN:
    case H_BUILTIN:
    case H_EXEC: {
      /* reset shell_optind for shell_getopt() inside builtins */
      shell_optind = 1;
      shell_optofs = 0;

      if(fd_in)
        fdtable_open(fd_in, FDTABLE_MOVE);
      if(fd_out)
        fdtable_open(fd_out, FDTABLE_MOVE);
      if(fd_err)
        fdtable_open(fd_err, FDTABLE_MOVE);

      ret = cmd->builtin->fn(argc, argv);
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

      //    sh_setargs(argv, 0);
      eval_push(&e, E_FUNCTION | (sh->opts.debug ? E_PRINT : 0));

      if((ret = setjmp(e.jumpbuf)) == 0) {
        e.jump = 1;
        eval_cmdlist(&e, &cmd->fn->ngrp);

        ret = eval_pop(&e);
      } else {
        ret >>= 1;
      }

      sh_pop(&inst);
      vartab_pop(&vars);

      break;
    }

    case H_PROGRAM: {
      ret = exec_program(cmd->path, argv, exec, redir);
      break;
    }
  }

  /* if exec is set we never return! */
  if(exec)
    sh_exit(ret);

  return ret;
}
