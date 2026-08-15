#include "../builtin.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../source.h"
#include "../exec.h"
#include "../../lib/alloc.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../eval.h"
#include "../../lib/windoze.h"
#include <stdio.h>
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Search PATH for a readable file (for dot/source builtin).
 * Unlike exec_path, this checks for R_OK not X_OK.
 * Returns allocated path or NULL. */
static char*
source_search_path(const char* name) {
  const char* vpath;
  static char path[PATH_MAX];
  unsigned long si = 0, pi = 0;
  vpath = var_value("PATH", NULL);

  if(!vpath)
    return NULL;

  do {
    unsigned long ni;

    if(vpath[si] == ':')
      si++;

    ni = str_chr(&vpath[si], ':');

    if(ni >= PATH_MAX) {
      si += ni;
      continue;
    }

    pi = str_copyn(path, &vpath[si], ni);

    if(pi && pi < PATH_MAX - 1)
      if(path[pi - 1] != '/')
        path[pi++] = '/';

    str_copyn(&path[pi], name, PATH_MAX - pi - 1);

    /* Check if file is readable (not executable like exec_path) */
    if(access(path, R_OK) == 0)
      return str_dup(path);

    si += ni;
  } while(vpath[si]);

  return NULL;
}

/* source shell script
 * ----------------------------------------------------------------------- */
const char help_source[] =
    "    Read and run commands from a file in the current shell.\n"
    "\n"
    "    file            script to read and run\n"
    "    arguments       positional parameters ($1, $2, ...) while running\n";

int
builtin_source(int argc, char* argv[]) {
  const char* fname;
  const char* path_to_open;
  char* searched_path = NULL;
  struct fd src;
  struct source in;
  struct arg oldarg;
  struct eval e;
  int ret;
  int jmpret;

  if((fname = argv[shell_optind]) == NULL) {
    builtin_errmsg(argv, "filename argument required", NULL);
    return EXIT_ERROR;
  }

  /* If filename contains no slash, search PATH (POSIX requirement) */
  if(str_chr(fname, '/') >= str_len(fname)) {
    searched_path = source_search_path(fname);
    if(searched_path)
      path_to_open = searched_path;
    else
      path_to_open = fname;  /* Will fail with "not found" */
  } else {
    path_to_open = fname;
  }

  fd_push(&src, STDSRC_FILENO, FD_READ);
  source_push(&in);
  in.fd = &src;

  if(!fd_mmap(&src, path_to_open)) {
    /* Set up an eval frame with a jump buffer so that return/break/continue
       from the sourced script can unwind back to this point */
    eval_push(&e, E_ROOT);
    e.jump = 1;
    
    jmpret = setjmp(e.jumpbuf);
    if(jmpret == 0) {
      /* Normal execution path */
      sh_pushargs(&oldarg);
      sh_setargs(&argv[++shell_optind], 0);
      sh_loop();
      sh_popargs(&oldarg);
      ret = sh->exitcode;
    } else {
      /* Longjmp from return/break/continue - jmpret is (value << 1) | 1
         for return, or just 1 for break/continue */
      ret = jmpret >> 1;
      sh->exitcode = ret;
      fprintf(stderr, "DEBUG builtin_source: longjmp returned, ret=%d, sh->exitcode=%d\n", 
              ret, sh->exitcode);
      sh_popargs(&oldarg);
    }
    
    eval_pop(&e);
  } else {
    ret = 1;
  }

  source_popfd(&src);

  if(searched_path)
    alloc_free(searched_path);

  return ret;
}
