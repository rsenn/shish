#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef SHPARSE2AST

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../parse.h"
#include "../ast.h"
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

/* usage text, scoped to the options this tool actually accepts --
 * shparse2ast never executes anything, so it has no use for shish's
 * xtrace/errexit/job-control/etc. interpreter flags.
 * ----------------------------------------------------------------------- */
static void
ast_usage(void) {
  buffer_puts(fd_err->w, "usage: ");
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w,
              " [-c command_string] [-P] [-o FILE] [-w NUM] [-r MODE] [-nm5bsx] [script]\n"
              "\n"
              "    -c command_string   parse command_string instead of a script/stdin\n"
              "    -P                  suppress position information\n"
              "    -o FILE             write JSON output to FILE instead of stdout\n"
              "    -w NUM              indent width, in spaces\n"
              "    -r MODE             position field(s): loc, range, or both (default loc)\n"
              "    -n                  NDJSON: one single-line JSON object per top-level command\n"
              "    -m                  minify: no whitespace at all (overrides -w)\n"
              "    -5                  JSON5 output: same as -b -s -x\n"
              "    -b                  keys without quotes\n"
              "    -s                  strings in single quotes\n"
              "    -x                  numbers in hexadecimal\n");
  buffer_flush(fd_err->w);
}

int sh_argc;
char** sh_argv;
const char* sh_name;
unsigned int indent_width = 2;
const char* tmpl = 0;
int inplace = 0;

const char* in_file = 0;
stralloc out_file;
int out_fd = 1;
buffer out_buf;

int sh_no_position = 0;
int sh_interactive = 0;

/* main routine
 * ----------------------------------------------------------------------- */
int
main(int argc, char** argv, char** envp) {
  unsigned int i;
  int c, e, v;
  int indent = 2, loc = 1, range = 0;
  int ndjson = 0, minify = 0, bare_keys = 0, single_quotes = 0, hex_numbers = 0;
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
      fd = fd_push_allocb(e, flags);
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
  sh_name = path_basename(sh_argv0, NULL);

  shell_init(buffer_2, sh_name);

  sh_no_position = 0;

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:o:w:Pr:nm5bsx")) > 0)
    switch(c) {
      case 'c': cmds = shell_optarg; break;
      case 'P': sh_no_position = 1; break;
      case 'o': {
        /* the file replaces stdout: same push + lazy open + move as a "> FILE" redirection */
        struct fd* o = fd_push_allocb(STDOUT_FILENO, FD_WRITE);

        fd_open(o, shell_optarg, 0);

        /* fdtable_open() reports the failure itself */
        if(fdtable_open(o, FDTABLE_MOVE) == FDTABLE_ERROR)
          return 1;

        fd_setbuf(o, &o[1], FD_BUFSIZE);
        break;
      }
      case 'w': scan_int(shell_optarg, &indent); break;
      case 'n': ndjson = 1; break;
      case 'm': minify = 1; break;
      case '5': bare_keys = single_quotes = hex_numbers = 1; break;
      case 'b': bare_keys = 1; break;
      case 's': single_quotes = 1; break;
      case 'x': hex_numbers = 1; break;
      case 'r':
        /* -r loc|range|both: which position field(s) to emit per node
         * (default: loc only). -P still overrides both to none. */
        loc = str_diff(shell_optarg, "range") != 0;
        range = str_diff(shell_optarg, "loc") != 0;
        break;
      default: ast_usage(); return 1;
    }

  for(i = 0; i < indent_width; i++)
    stralloc_catc(&separator, ' ');
  stralloc_nul(&separator);
  tree_separator = separator.s;

  /* set up the source fd (where the shell reads from) */
  fd = fd_push_alloc(STDSRC_FILENO, FD_READ);

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
  sh_name = path_basename(sh_argv0, NULL);

  if(*sh_name == '-') 
    sh_name++;

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  source_push(&src);

  stralloc_init(&cmd);
  parse_init(&p, P_DEFAULT);
  buffer_init(&out_buf, &buffer_op_write, out_fd, alloca(1024), 1024);
  
  {
    union node *script = 0, **nptr = &script;

    for(;;) {
      p.pushback = 0;
      tok = parse_gettok(&p, P_DEFAULT);

      if(tok & T_EOF)
        break;

      p.pushback++;
      parse_lineno = source->position.line;

      /* launch the parser to get a complete command */
      nptr = tree_append(nptr, parse_list(&p));
    }

    if(script) {
      struct ast a;

      ast_init(&a, fd_out->w, indent);
      a.loc = loc;
      a.range = range;
      a.no_position = sh_no_position;
      a.j.bare_keys = bare_keys;
      a.j.single_quotes = single_quotes;
      a.j.hex_numbers = hex_numbers;
      a.j.minify = minify;

      if(ndjson) {
        ast_ndjson(&a, script);
      } else {
        ast_list(&a, script);
        buffer_putc(fd_out->w, '\n');
        buffer_flush(fd_out->w);
      }
    }
  }

  return 0;
}
#endif