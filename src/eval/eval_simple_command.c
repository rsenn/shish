#include "../../lib/alloc.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif
#include "../fdtable.h"
#include "../sh.h"
#include "../eval.h"
#include "../exec.h"
#include "../expand.h"
#include "../fdstack.h"
#include "../job.h"
#include "../parse.h"
#include "../redir.h"
#include "../source.h"
#include "../tree.h"
#include "../vartab.h"
#include "../job.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#if !WINDOWS_NATIVE
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>

/* evaluate a simple command (3.9.1)
 *
 * this function doesn't put stuff in background, it always wait()s, so
 * it only needs to fork() real programs
 * ----------------------------------------------------------------------- */
int
eval_simple_command(struct eval* e, struct ncmd* ncmd) {
  int argc, status = 0, assign_error = 0, redir_error = 0;
  char** argv;
  union node *node, *args = 0, *assigns = 0, *r, *redir = ncmd->rdir;
  struct command cmd = {H_BUILTIN, {0}};
  struct vartab vars;
  struct fdstack io;
  char buf[FD_BUFSIZE];

  /* expand arguments,
     if there are arguments we start a hashed search for the command */
  expand_error = 0;

  if(expand_args(ncmd->args, &args, 0)) {
    stralloc_nul(&args->narg.stra);
    cmd = exec_hash(args->narg.stra.s, 0);
  }

  /* POSIX 2.8.1: an expansion error means this command does not run at
     all, and its status is nonzero. A non-interactive shell never gets
     here -- expand_param() has already exited it. */
  if(expand_error) {
    expand_error = 0;
    status = 1;
    goto end;
  }

  /*if(sh->exitcode) {
    tree_free(args);
    eval_exit(sh->exitcode);
  }*/

  /* expand and set the variables,
     mark them for export if we're gonna execute a command --
     cmdsubst_ran is reset here (not any earlier, since "$?" within
     ncmd->args above must still see whatever ran before this command)
     so the "no command, only assignments" status handling below can
     tell "a command substitution in one of these assignments ran"
     apart from "sh->exitcode is just still holding the previous
     command's status" */
  sh->cmdsubst_ran = 0;

  if(expand_vars(ncmd->vars, &assigns)) {

#ifdef DEBUG_OUTPUT_
    if(ncmd->vars) {
      buffer_puts(debug_output, "Vars ");
      debug_list(ncmd->vars, 0);
      debug_nl_fl();
      buffer_puts(debug_output, "Assigns ");
    }
#endif
    /* if we don't exit after the command, have a command and not a
       special builtin the variable changes should be temporary --
       pushed as a function-like scope (the "1") so that a *non-local*
       var_create() elsewhere (a builtin like "read" writing its
       target variable, or a called function doing a plain, non-
       "local" assignment) walks straight past it to the real
       enclosing scope instead of being silently captured and thrown
       away by vartab_pop() below along with the prefix assignment
       itself. Confirmed both ways were broken before: "IFS=x read
       line" leaked IFS afterward (no scope was pushed for "read" at
       all, since it used to be misclassified as H_SBUILTIN), and once
       that got fixed, "FOO=bar f" for a function f that sets a plain
       (non-local) global went from "leaks FOO, keeps its own global
       write" to "keeps neither" -- var_create()'s walk-past-transient-
       scope logic (already used to skip a *function call's* own
       scope for exactly this reason) didn't know this prefix-
       assignment scope was transient too, since it was pushed with
       function=0. */
    int temp_scope = !(e->flags & E_EXIT) && cmd.ptr && cmd.id != H_SBUILTIN;

    if(temp_scope)
      vartab_push(&vars, 1);

    for(node = assigns; node; node = node->next) {

      if(e->flags & E_PRINT) {
        stralloc* sa = &node->narg.stra;
        size_t offs = byte_chr(sa->s, sa->len, '=');

        if(offs < sa->len)
          offs++;
        eval_print_prefix(e, fd_err->w);
        buffer_put(fd_err->w, sa->s, offs);
        debug_squoted(&sa->s[offs], sa->len - offs, fd_err->w);
        buffer_putnlflush(fd_err->w);
      }

      /* when a temp scope was pushed above, V_LOCAL forces the
         assignment into that exact scope, bypassing the same
         walk-past-transient-scope logic used elsewhere -- the prefix
         assignment itself must land here, not wherever an existing
         same-named variable already lives, or it would permanently
         overwrite that variable instead of shadowing it for just this
         command's duration. Without a temp scope (a special builtin,
         or E_EXIT), there's nothing to shadow into -- fall back to
         ordinary assignment semantics, which already correctly finds
         and updates whatever scope the variable actually lives in. */
      if(!var_setsa(&node->narg.stra,
                    (cmd.ptr ? V_EXPORT : V_DEFAULT) | (temp_scope ? V_LOCAL : 0))) {
        status = 1;
        assign_error = 1;
        break;
      }
    }

    tree_free(assigns);

    if(status)
      goto end;
  }

  /* do redirections if present */
  if((ncmd->rdir || (ncmd->bgnd && !sh->opts.monitor)) && cmd.id != H_SBUILTIN &&
     cmd.id != H_EXEC)
    fdstack_push(&io);

  /* POSIX 2.9.3.1 (Asynchronous Lists): "the standard input for an
     asynchronous list, before any explicit redirections are
     performed, shall be considered to be assigned to a file that has
     the same properties as /dev/null" -- except when job control is
     enabled, when this redirection does not occur. Pushed first,
     exactly like a real "< /dev/null" would be (fd_push()+fd_open(),
     the same two calls redir_eval()/redir_open() make for a plain
     "<file"), so this command's own real redirections -- the loop
     right below, if it has any -- naturally override it by pushing
     their own fd 0 entry on top, the same last-one-wins semantics any
     two of the command's own redirections targeting the same fd
     already have. Doing this here, before that loop, rather than
     after forking for the background (as a raw dup2() on fd 0 would
     have to), is what makes it correctly precede a redirection that
     is itself relative to "the current fd 0" (e.g. "cmd <&0 &") --
     that dup must see this null default already in place, not
     whatever fd 0 happened to mean before this command ran. */
  if(ncmd->bgnd && !sh->opts.monitor && cmd.id != H_SBUILTIN && cmd.id != H_EXEC) {
    struct fd* nullfd;
#ifdef HAVE_ALLOCA
    nullfd = fd_alloc();
#else
    nullfd = fd_malloc();
#endif
    nullfd = fd_push(nullfd, STDIN_FILENO, FD_READ);
    fd_open(nullfd, "/dev/null", 0);

    if(fd_needbuf(nullfd))
      fd_setbuf(nullfd, buf, FD_BUFSIZE);
  }

  for(r = ncmd->rdir; r; r = r->next) {
    struct fd* fd = NULL;

    /* if its the exec special builtin the new fd needs to be persistent */
    if(cmd.id != H_EXEC) {
#ifdef HAVE_ALLOCA
      fd = fd_alloc();
#else
      fd = fd_malloc();
#endif
    }

    /* return if a redirection failed. Force immediate resolution
       (R_NOW) rather than the usual lazy one when there's no command
       to run at all ("args == NULL" -- e.g. a bare "<_no_such_file_"):
       exec_command.c is what forces (and checks) lazy resolution for
       an actual command's fd 0/1/2 right before running it, but that
       code never runs here, so nothing would ever open()/notice the
       failure at all otherwise (redirect-failure-does-not-block-
       execution-or-set-status -- this is the "no command" half of it;
       exec_command.c's own fdtable_open() result checks are the
       other). */
    if(redir_eval(&r->nredir, fd, (cmd.id == H_EXEC || args == NULL ? R_NOW : 0))) {
      redir_error = 1;
      status = 1;
      goto end;
    }

    /* check if we need to initialize fd buffers for the new redirection */
    if(fd_needbuf(r->nredir.fd)) {
      /* if its not exec then set up buffers for
         temporary redirections on the stack */
      if(cmd.id != H_EXEC)
        fd_setbuf(r->nredir.fd, buf, FD_BUFSIZE);
      else
        fd_allocbuf(r->nredir.fd, FD_BUFSIZE);
    }

#ifdef DEBUG_OUTPUT_
    buffer_puts(debug_output, "Redirection ");
    debug_node(r, -1);

    if(r->nredir.fd) {
      buffer_puts(debug_output, "fd { n= ");
      buffer_putlong(debug_output, r->nredir.fd->n);
      buffer_puts(debug_output, " }");
    }

    debug_nl_fl();
#endif
  }

  /* if there is no command we can return after
     setting the vars and doing the redirections -- per POSIX, this
     command's status is that of the last command substitution
     performed in one of the assignments above, or 0 if none ran */
  if(args == NULL) {
    status = sh->cmdsubst_ran ? sh->exitcode : 0;
    goto end;
  }

  /* when the command wasn't found we abort. exec_lasterrno is what
     exec_hash()/exec_path() actually saw (ENOENT if nothing on PATH
     matched, EACCES/EISDIR if something did but couldn't run) --
     plain "errno" isn't trustworthy here, since redirections, variable
     expansion and everything else between the lookup and this check
     routinely clobber it */
  if(cmd.ptr == NULL) {
    /* the redirections above only *allocate* a pending fd (FD_OPEN,
       real open()/dup2() deferred) -- exec_command() is what normally
       resolves them, right before running a builtin or program. We
       bail out before ever reaching exec_command(), so without this,
       the message below goes out through whatever fd_err->w was still
       wired to beforehand (typically the original terminal), and the
       redirection target file is never even created */
    if(fd_in)
      fdtable_open(fd_in, FDTABLE_MOVE);

    if(fd_out)
      fdtable_open(fd_out, FDTABLE_MOVE);

    if(fd_err)
      fdtable_open(fd_err, FDTABLE_MOVE);

    source_msg(&e->pos);

    buffer_putsa(fd_err->w, &args->narg.stra);
    buffer_putm_internal(fd_err->w, ": ", strerror(exec_lasterrno), 0);
    buffer_putnlflush(fd_err->w);
    errno = exec_lasterrno;
    status = exec_error();
    goto end;
  }

  /* allocate based on tree_count (upper bound); expand_argv returns the
     actual count after dropping empty unquoted fields */
  argc = tree_count(args);
#ifdef HAVE_ALLOCA
  argv = alloca((argc + 1) * sizeof(char*));
#else
  argv = alloc((argc + 1) * sizeof(char*));
#endif
  argc = expand_argv(args, argv);

  if(e->flags & E_PRINT) {
    eval_print_prefix(e, fd_err->w);

    if(debug_argv(argv, fd_err->w))
      buffer_putnlflush(fd_err->w);
  }

  /* execute the command, this may or may not return, depending on E_EXIT */
  exec_redir_error = 0;
  status = exec_command(&cmd,
                        argc,
                        argv,
                        ((e->flags & E_EXIT) ? X_EXEC : 0) | (ncmd->bgnd ? X_NOWAIT : 0));

  redir_error |= exec_redir_error;

  if(ncmd->bgnd) {
    struct job* j = *job_pointer;

    j->command = tree_string((union node*)ncmd);
  }

#ifndef HAVE_ALLOCA
  alloc_free(argv);
#endif
end:

  /* restore variable stack */
  if(varstack == &vars)
    vartab_pop(&vars);

  if(args)
    tree_free(args);

  /* undo redirections */

  if(fdstack == &io) {
    fdstack_pop(&io);
  } else if(cmd.id != H_EXEC) {
    for(r = redir; r; r = r->next)
      fd_pop(r->nredir.fd);
  }

  sh->exitcode = status;

  /* POSIX 2.8.1: a non-interactive shell exits on either of these.
       assignment error    any command, or none:  "readonly a=a; a=b"
       redirection error   special builtins only: "shift <_no_such_file_"
     The same redirection error on a plain utility is not fatal.
     sh_interactive (the whole session's own interactive-ness, sh.h)
     is what gates this, not source->mode's SOURCE_IACTIVE bit -- the
     latter is reset for every nested source (a `.`-sourced file), so
     checking it here would kill an interactive shell the moment one
     of these fails one level into any sourced file. */
  if((assign_error || (redir_error && (cmd.id == H_SBUILTIN || cmd.id == H_EXEC))) &&
     !sh_interactive) {
    sh_exit(status);
  }

  return status;
}
