#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../var.h"
#include "../../lib/path.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"
#include "../term.h"
#include "../history.h"

#include <stdlib.h>

int sh_argc;
char** sh_argv;
char* sh_name;
int sh_login = 0;
int sh_no_position = 0;

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
  int no_interactive = 0;

  fd_expected = STDERR_FILENO + 1;

  /* create new fds for every valid file descriptor until stderr */
  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    if((flags = fdtable_check(e))) {
#ifdef HAVE_ALLOCA
      fd = fd_allocb();
      fd_push(fd, e, flags);
#else
      fd = fd_mallocb();
      fd_push(fd, e, flags | FD_FREE);
#endif
      fd_setfd(fd, e);
    } else {
      if(e < fd_expected)
        fd_expected = e;
    }
  }

  /* stat the file descriptors and then set the buffers */
  fdtable_foreach(v) {
    fd_stat(fdtable[v]);
    fd_setbuf(fdtable[v], &fdtable[v][1], FD_BUFSIZE);
  }

  /* set initial $0 */
  sh_argv0 = argv[0];

  shell_init(fd_err->w, path_basename(sh_argv0));

  /* import environment variables to the root vartab */
  for(c = 0; envp[c]; c++)
    ;

#ifdef HAVE_ALLOCA
  if(!(envvars = alloca(sizeof(struct var) * c)))
#endif
    envvars = malloc(sizeof(struct var) * c);

  for(c = 0; envp[c]; c++) var_import(envp[c], V_EXPORT, &envvars[c]);

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:xe")) > 0) switch(c) {
      case 'c': cmds = shell_optarg; break;
      case 'x': sh->opts.xtrace = 1; break;
      case 'e': sh->opts.errexit = 1; break;

#ifdef _DEBUG
      case 'I': no_interactive = 1; break;

#endif

      default:
        sh_usage();
        sh_exit(1);
        break;
    }

      /* set up the source fd (where the shell reads from) */
#ifdef HAVE_ALLOCA
  fd = fd_alloc();
  fd_push(fd, STDSRC_FILENO, FD_READ);
#else
  fd = fd_malloc();
  fd_push(fd, STDSRC_FILENO, FD_READ | FD_FREE);
#endif

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
    fd_setbuf(fd_src, &fd_src[1], FD_BUFSIZE);

  /* set our basename for the \v prompt escape seq and maybe other stuff*/
  sh_name = path_basename(sh_argv0);

  if(*sh_name == '-') {
    sh_name++;
    sh_login++;
  }

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  sh_init();

  source_push(&src);

  if((fd_src->mode & FD_CHAR) && !no_interactive && term_init(fd_src, fd_err)) {
    src.mode |= SOURCE_IACTIVE;

    sh->opts.monitor = 1;
    sh->opts.histexpand = 1;
  } else
    src.mode &= ~SOURCE_IACTIVE;

  /*  if(fd_expected != fd_top && (flags = fdtable_check(e)))
    {
      fd = fd_allocb();
      fd_push(fd, e, flags);
      fd_setfd(fd, e);
      fdtable_track(e, FDTABLE_LAZY);
    }*/

  sh_loop();

  if(source->mode & SOURCE_IACTIVE) {
    term_restore(source->b->fd, &term_tcattr);

    history_save();
    history_clear();
  }

  sh_exit(0);

  return 0;
}
