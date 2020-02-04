#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

#include "fd.h"
#include "fdtable.h"
#include "sh.h"
#include "shell.h"
#include "source.h"
#include "str.h"
#include "var.h"
#include <stdlib.h>

#include "uint32.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int sh_argc;
char** sh_argv;
char* sh_name;
stralloc sh_hostname = {NULL, 0, 0};
int sh_login = 0;
int sh_child = 0;

/* main routine
 * ----------------------------------------------------------------------- */
int
main(int argc, char** argv, char** envp) {
  int c;
  int e, v;
  int flags;
  struct fd* fd;
  struct source src;
  char* cmds = NULL;
  struct var* envvars;

  fd_exp = STDERR_FILENO + 1;

  /* create new fds for every valid file descriptor until stderr */
  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    if((flags = fdtable_check(e))) {
      fd_allocab(fd);
      fd_push(fd, e, flags);
      fd_setfd(fd, e);
    } else {
      if(e < fd_exp)
        fd_exp = e;
    }
  }

  /* stat the file descriptors and then set the buffers */
  fdtable_foreach(v) {
    fd_stat(fdtable[v]);
    fd_setbuf(fdtable[v], &fdtable[v][1], D_BUFSIZE);
  }

  /* set initial $0 */
  sh_argv0 = argv[0];
  sh_name = shell_basename(sh_argv0);

  shell_init(fd_err->w, sh_name);

  /* import environment variables to the root vartab */
  for(c = 0; envp[c]; c++)
    ;

#ifndef HAVE_ALLOCA
#warning no alloca
#else
  if(!(envvars = alloca(sizeof(struct var) * c)))
#endif
  envvars = malloc(sizeof(struct var) * c);

  for(c = 0; envp[c]; c++) var_import(envp[c], V_EXPORT, &envvars[c]);

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:")) > 0) switch(c) {
      case 'c': cmds = shell_optarg; break;
      case '?': sh_usage(); break;
    }

  /* set up the source fd (where the shell reads from) */
  fd_alloca(fd);
  fd_push(fd, STDSRC_FILENO, D_READ);

  /* if there were cmds supplied with the option
     -c then read input from this string. */
  if(cmds)
    fd_string(fd_src, cmds, str_len(cmds));

  /* if there is an argument we open it as input file */
  else if(argv[shell_optind]) {
    fd_mmap(fd_src, argv[shell_optind]);

    sh_argv0 = argv[shell_optind++];
  }

  /* input is read from stdin, maybe interactively */
  else if(fd_in)
    fd_dup(fd_src, STDIN_FILENO);

  if(fd_needbuf(fd_src))
    fd_setbuf(fd_src, &fd_src[1], D_BUFSIZE);

  /* set our basename for the \v prompt escape seq and maybe other stuff*/
  sh_name = shell_basename(sh_argv0);

  if(*sh_name == '-') {
    sh_name++;
    sh_login++;
  }

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  source_push(&src);
  sh_init();

  /*  if(fd_exp != fd_top && (flags = fdtable_check(e)))
    {
      fd_allocab(fd);
      fd_push(fd, e, flags);
      fd_setfd(fd, e);
      fdtable_track(e, FDTABLE_LAZY);
    }*/

  sh_loop();

  sh_exit(0);

  return 0;
}
