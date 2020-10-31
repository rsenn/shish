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
#include "../parse.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"

#include <stdlib.h>

int sh_argc;
char** sh_argv;
char* sh_name;

#define SH_INPLACE 0x1000

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
  /*  struct var* envvars;*/
  struct parser p;
  union node* list;
  stralloc cmd;
  enum tok_flag tok;

  fd_exp = STDERR_FILENO + 1;

  /* create new fds for every valid file descriptor until stderr */
  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    if((flags = fdtable_check(e))) {
#ifdef HAVE_ALLOCA
      fd = fd_allocab();
      fd_push(fd, e, flags);
#else
      fd = fd_mallocb();
      fd_push(fd, e, flags | FD_FREE);
#endif
      fd_setfd(fd, e);
    } else {
      if(e < fd_exp)
        fd_exp = e;
    }
  }

  /* stat the file descriptors and then set the buffers */
  fdtable_foreach(v) {
    fd_stat(fdtable[v]);
    fd_setbuf(fdtable[v], &fdtable[v][1], FD_BUFSIZE);
  }

  /* set initial $0 */
  sh_argv0 = argv[0];
  sh_name = shell_basename(sh_argv0);

  shell_init(fd_err->w, sh_name);

  /* import environment variables to the root vartab */
  /*  for(c = 0; envp[c]; c++)
      ;
  #ifdef HAVE_ALLOCA
    if(!(envvars = alloca(sizeof(struct var) * c)))
  #endif
      envvars = malloc(sizeof(struct var) * c);

    for(c = 0; envp[c]; c++) var_import(envp[c], V_EXPORT, &envvars[c]);
  */

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:xe")) > 0) switch(c) {
      case 'c': cmds = shell_optarg; break;
      case 'x': sh->flags |= SH_DEBUG; break;
      case 'e': sh->flags |= SH_ERREXIT; break;
      case 'i': sh->flags |= SH_INPLACE; break;
      case '?': sh_usage(); break;
    }

      /* set up the source fd (where the shell reads from) */
#ifdef HAVE_ALLOCA
  fd = fd_alloca();
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
  sh_name = shell_basename(sh_argv0);

  if(*sh_name == '-') {
    sh_name++;
    //    sh_login++;
  }

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  source_push(&src);
  sh_init();

  stralloc_init(&cmd);

  parse_init(&p, P_DEFAULT);

  while(!(((tok = parse_gettok(&p, P_DEFAULT)) & T_EOF))) {
    p.pushback++;
    parse_lineno = source->pos.line;

    //   var_setvint("LINENO", parse_lineno, V_DEFAULT);

    /* launch the parser to get a complete command */
    list = parse_list(&p);
    stralloc_zero(&cmd);

    if(list)
      tree_catlist(list, &cmd, NULL);

    if(!(p.tok & (T_NL | T_SEMI | T_BGND))) {
      /* we have a parse error */
      if(p.tok != T_EOF)
        parse_error(&p, 0);

      /* exit if not interactive */
      if(!(source->mode & SOURCE_IACTIVE))
        sh_exit(1);

      /* ..otherwise discard the input buffer */
      source_flush();
      p.pushback = 0;
    }

    if(p.tok & (T_NL | T_SEMI | T_BGND))
      p.pushback = 0;

    buffer_putsa(fd_out->w, &cmd);
    buffer_putnlflush(fd_out->w);
  }

  sh_exit(0);

  return 0;
}
