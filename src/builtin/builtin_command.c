#include "builtin_config.h"

#if BUILTIN_COMMAND

#include "../fdtable.h"
#include "../sh.h"
#include <errno.h>
#include "../builtin.h"
#include "../exec.h"
#include "../parse.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"
#include "../../lib/stralloc.h"
#include "../var.h"

/* Helper function to check if a name is a shell keyword */
static inline int
is_keyword(const char* str) {
  int i;

  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return 0;
}

/* command built-in
 *
 * ----------------------------------------------------------------------- */
const char help_command[] =
    "    Run a command, bypassing any shell function of the same name.\n"
    "\n"
    "    -p              use a default PATH guaranteed to find the standard\n"
    "                    utilities\n"
    "    -v              print the resolved path/description instead of\n"
    "                    running the command\n"
    "    -V              like -v, but more verbose\n"
    "    command         command to run, bypassing any shell function of\n"
    "                    the same name\n"
    "    arg             arguments passed to command\n";

#define COMMAND_STDPATH "/bin:/usr/bin"

/* absolute path of a command name found by a "/" lookup
 * ----------------------------------------------------------------------- */
static void
command_abspath(const char* path, stralloc* out) {
  out->len = 0;

  if(path[0] != '/' && sh->cwd.len) {
    stralloc_catb(out, sh->cwd.s, sh->cwd.len);
    stralloc_catc(out, '/');
    stralloc_cats(out, path[0] == '.' && path[1] == '/' ? path + 2 : path);
  } else {
    stralloc_cats(out, path);
  }

  stralloc_nul(out);
}

/* command -v / -V: describes how "name" would be interpreted; returns 0 if found
 * ----------------------------------------------------------------------- */
static int
command_describe(char* name, int verbose, int stdpath) {
  struct command cmd;
  struct alias* a;
  stralloc abs;
  const char* where = 0;
  const char* kind = 0;
  int ret = 0;

  stralloc_init(&abs);

  if(!stdpath && (a = parse_findalias(0, name, str_len(name)))) {
    if(verbose)
      buffer_putm_internal(fd_out->w, name, " is an alias for ", a->def + a->namelen + 1, NULL);
    else
      buffer_putm_internal(fd_out->w, "alias ", name, "='", a->def + a->namelen + 1, "'", NULL);
  } else if(!stdpath && is_keyword(name)) {
    if(verbose)
      buffer_putm_internal(fd_out->w, name, " is a shell keyword", NULL);
    else
      buffer_puts(fd_out->w, name);
  } else if((cmd = exec_hash(name, 0)).ptr) {
    switch(cmd.id) {
      case H_FUNCTION:
        kind = "a function";
        break;
      case H_SBUILTIN:
        kind = "a special built-in";
        break;
      case H_BUILTIN: {
        /* a regular built-in is described by the program it shadows */
        char* p = str_chr(name, '/') == str_len(name) ? exec_path(name) : 0;

        if(p) {
          command_abspath(p, &abs);
          where = abs.s;
        }

        kind = "a regular built-in";
        break;
      }
      default:
        command_abspath(cmd.path ? cmd.path : name, &abs);
        where = abs.s;
        break;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, name, " is ", kind ? kind : where, NULL);

      if(kind && where)
        buffer_putm_internal(fd_out->w, " (", where, ")", NULL);
    } else {
      buffer_puts(fd_out->w, where && (!kind || cmd.id == H_BUILTIN) ? where : name);
    }
  } else {
    if(verbose)
      buffer_putm_internal(fd_err->w, name, ": not found\n", NULL), buffer_flush(fd_err->w);

    ret = 1;
  }

  if(!ret)
    buffer_putnlflush(fd_out->w);

  stralloc_free(&abs);
  return ret;
}

int
builtin_command(int argc, char* argv[]) {
  int c, default_path = 0, print_desc = 0, print_verbose = 0;
  struct command cmd;
  char *name, *oldpath = 0;
  int ret = 1;

  /* check options, -l for login dash, -c for null env, -a to set argv[0] */
  while((c = shell_getopt(argc, argv, "pvV")) > 0) {
    switch(c) {
      case 'p': default_path = 1; break;
      case 'v': print_desc = 1; break;
      case 'V': print_verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* no arguments? return now! */
  if(!(name = argv[shell_optind]))
    return 0;

  /* -p: search a PATH that finds the standard utilities */
  if(default_path) {
    const char* old = var_value("PATH", NULL);

    oldpath = old ? str_dup(old) : 0;
    var_setv("PATH", COMMAND_STDPATH, str_len(COMMAND_STDPATH), 0);
  }

  if(print_desc || print_verbose) {
    for(ret = 0; argv[shell_optind]; shell_optind++)
      if(command_describe(argv[shell_optind], print_verbose, default_path))
        ret = 1;

    goto done;
  }

  /* look up the command and exec if found */
  if((cmd = exec_hash(name, H_FUNCTION)).ptr) {
    /* the program is resolved; the child sees the caller's PATH */
    if(oldpath) {
      var_setv("PATH", oldpath, str_len(oldpath), 0);
      alloc_free(oldpath);
      oldpath = 0;
    }

    /* try to exec */
    exec_via_command++;
    ret = exec_command(&cmd, argc - shell_optind, &argv[shell_optind], 0);
    exec_via_command--;

    if(EXIT_NOEXEC > ret)
      return ret;
  }

  /* at this point the exec stuff failed */
  if(!cmd.ptr)
    errno = exec_lasterrno ? exec_lasterrno : ENOENT;

  {
    int saved = errno;

    ret = exec_error();
    errno = saved;
  }

  sh_error_errno(argv[shell_optind]);

done:
  if(oldpath) {
    var_setv("PATH", oldpath, str_len(oldpath), 0);
    alloc_free(oldpath);
  }

  return ret;
}
#endif /* BUILTIN_COMMAND */
