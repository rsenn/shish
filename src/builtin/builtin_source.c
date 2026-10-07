#include "builtin_config.h"

#if BUILTIN_SOURCE

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
#include <sys/stat.h>
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

/* put the caller's positional parameters back, freeing the ones "." made */
static void
source_restoreargs(struct arg* old, int olda, int newargs) {
  if(newargs)
    sh_setargs(NULL, 0);

  sh_popargs(old);
  sh->arg.a = olda;
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
  struct stat st;
  int ret, jmpret;
  int olda = 0, newargs = 0;

  if((fname = argv[shell_optind]) == NULL) {
    builtin_errmsg(argv, "filename argument required", NULL);
    return EXIT_ERROR;
  }

  /* If filename contains no slash, search PATH (POSIX requirement) */
  if(str_chr(fname, '/') >= str_len(fname)) {
    if((searched_path = source_search_path(fname)))
      path_to_open = searched_path;
    else
      path_to_open = fname; /* Will fail with "not found" */
  } else {
    path_to_open = fname;
  }

  fd_push(&src, STDSRC_FILENO, FD_READ);
  source_push(&in);
  in.mode |= SOURCE_VERBOSE;
  in.fd = &src;

  if(!fd_mmap(&src, path_to_open)) {
    /* Set up an eval frame with a jump buffer so that return/break/continue
       from the sourced script can unwind back to this point */
    eval_push(&e, E_ROOT | E_SOURCE);
    e.jump = 1;

    olda = sh->arg.a;
    sh_pushargs(&oldarg);

    /* without operands "." leaves $@ alone; with them it replaces it for the file's duration */
    if(argv[shell_optind + 1]) {
      sh->arg.a = 0;
      sh_setargs(&argv[shell_optind + 1], 1);
      newargs = 1;
    }

    if((jmpret = setjmp(e.jumpbuf)) == 0) {
      /* Normal execution path */
      sh_loop();
      source_restoreargs(&oldarg, olda, newargs);
      ret = sh->exitcode;

      /* an empty script yields 0, not the caller's $? */
      if(stat(path_to_open, &st) == 0 && st.st_size == 0)
        ret = sh->exitcode = 0;
    } else {
      /* Longjmp from return/break/continue - jmpret is (value << 1) | 1
         for return, or just 1 for break/continue */
      ret = jmpret >> 1;
      sh->exitcode = ret;
      source_restoreargs(&oldarg, olda, newargs);
    }

    eval_pop(&e);

    /* break/continue ran off the file: carry on in the caller */
    if(e.pending) {
      source_popfd(&src);

      if(searched_path)
        alloc_free(searched_path);

      eval_jump(e.pending, e.pendcont);
      return ret;
    }
  } else {
    ret = 1;
    source_popfd(&src);

    if(searched_path)
      alloc_free(searched_path);

    /* file not found: a special-builtin error, the one status of "."
       that exec_command() still treats as fatal */
    if(!sh_interactive && !exec_via_command)
      sh_exit(ret);

    return ret;
  }

  source_popfd(&src);

  if(searched_path)
    alloc_free(searched_path);

  return ret;
}
#endif /* BUILTIN_SOURCE */
