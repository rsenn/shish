#include "../builtin.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../source.h"
#include "../exec.h"

/* source shell script
 * ----------------------------------------------------------------------- */
int
builtin_source(int argc, char* argv[]) {
  const char* fname;
  struct fd src;
  struct source in;
  struct arg oldarg;
  int ret;

  if((fname = argv[shell_optind]) == NULL) {
    builtin_errmsg(argv, "filename argument required", NULL);
    return EXIT_ERROR;
  }

  fd_push(&src, STDSRC_FILENO, FD_READ);
  source_push(&in);

  if(!fd_mmap(&src, argv[shell_optind])) {
    sh_pushargs(&oldarg);
    sh_setargs(&argv[++shell_optind], 0);
    sh_loop();
    sh_popargs(&oldarg);
    ret = sh->exitcode;
  } else {
    ret = 1;
  }

  source_popfd(&src);

  return ret;
}
