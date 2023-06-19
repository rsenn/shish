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
#include "../debug.h"
#include "../../lib/path.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"
#include "../../lib/scan.h"
#include "../../lib/open.h"
#include "../../lib/buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

extern const char* tree_separator;
extern unsigned int tree_columnwrap;

int sh_argc;
char** sh_argv;
char* sh_name;
unsigned int indent_width = 2;
const char* tmpl = 0;
int inplace = 0;

const char* in_file = 0;
stralloc out_file;
int out_fd = 1;
buffer out_buf;

int sh_no_position = 0;

/* main routine
 * ----------------------------------------------------------------------- */
int
main(int argc, char** argv, char** envp) {
  unsigned int i;
  int c, e, v;
  struct fd* fd;
  struct source src;
  char* cmds = NULL;
  /*  struct var* envvars;*/
  struct parser p;
  stralloc cmd;
  enum tok_flag tok;
  stralloc separator;
  stralloc_init(&separator);
  stralloc_init(&out_file);

  fd_expected = STDERR_FILENO + 1;

  /* create new fds for every valid file descriptor until stderr */
  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    int flags;
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
  sh_name = path_basename(sh_argv0);

  shell_init(buffer_2, sh_name);

  debug_buffer.fd = 1;
  debug_nindent = 2;

  sh_no_position = 0;

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:xeo:q:w:l:P")) > 0)
    switch(c) {
      case 'c': cmds = shell_optarg; break;
      case 'x': sh->opts.xtrace = 1; break;
      case 'e': sh->opts.errexit = 1; break;
      case 'P': sh_no_position = 1; break;
      case 'o': debug_buffer.fd = open_trunc(shell_optarg); break;
      case 'w': scan_int(shell_optarg, &debug_nindent); break;
      case 'q': debug_quote = *optarg; break;
      default:
        sh_usage();

        buffer_puts(fd_err->w,
                    "\n"
                    "  -c CMDS     Read the commands from the first arg\n"
                    "  -e          Exit on error\n"
                    "  -x          Debug\n"
                    "  -P          Suppress position information\n"
                    "  -o FILE     Output file\n"
                    "  -w NUM      Indent num spaces\n"
                    "  -q CHAR     Quote char\n");

        buffer_flush(fd_err->w);
        return 1;
    }

  for(i = 0; i < indent_width; i++)
    stralloc_catc(&separator, ' ');
  stralloc_nul(&separator);
  tree_separator = separator.s;

  /* set up the source fd (where the shell reads from) */
#ifdef HAVE_ALLOCA
  fd = fd_alloc();
  fd_push(fd, STDSRC_FILENO, FD_READ);
#else
  fd = fd_malloc();
  fd_push(fd, STDSRC_FILENO, FD_READ | FD_FREE);
#endif

  if(cmds)
    fd_string(fd_src, cmds, str_len(cmds));

  else if(argv[shell_optind]) {
    in_file = argv[shell_optind];
    fd_mmap(fd_src, argv[shell_optind]);

    sh_argv0 = argv[shell_optind++];
  }

  else if(fd_in)
    fd_dup(fd_src, STDIN_FILENO);

  if(fd_needbuf(fd_src))
    fd_setbuf(fd_src, &fd_src[1], FD_BUFSIZE);

  /* set our basename for the \v prompt escape seq and maybe other stuff*/
  sh_name = path_basename(sh_argv0);

  if(*sh_name == '-') {
    sh_name++;
    //    sh_login++;
  }

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  source_push(&src);
  // sh_init();

  stralloc_init(&cmd);

  parse_init(&p, P_DEFAULT);

  buffer_init(
      &out_buf, (buffer_op_proto*)(void*)&write, out_fd, alloca(1024), 1024);
  {
    int n;
    union node *script = 0, **nptr;
    nptr = &script;

    // debug_begin(0, 0);
    for(n = 0;; n++) {
      union node* list;

      p.pushback = 0;
      tok = parse_gettok(&p, P_DEFAULT);

      if(tok & T_EOF)
        break;

      p.pushback++;
      parse_lineno = source->position.line;

      /* launch the parser to get a complete command */
      list = parse_list(&p); // parse_compound_list(&p, T_EOF);

      nptr = tree_append(nptr, list);
    }

    if(script) {
      debug_list(script, 1);
      debug_nl_fl();
    }
    //  debug_end(0);
  }

  return 0;
}
