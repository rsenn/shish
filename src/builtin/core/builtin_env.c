#include "../../builtin.h"
#include "../../exec.h"
#include "../../fdtable.h"
#include "../../var.h"
#include "../../vartab.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/alloc.h"
#include "../../../lib/str.h"
#include <errno.h>

const char help_env[] = "    Run a utility in a changed environment, or print the environment.\n"
                        "\n"
                        "    -i              start with an empty environment\n"
                        "    -u name         remove name from the environment\n"
                        "    name=value      set name in the environment\n"
                        "    utility         command to run (functions are not considered)\n"
                        "    argument        argument for utility\n"
                        "\n"
                        "    Without utility the resulting environment is printed.\n";

/* hide a variable for the utility: a local, unexported, unset shadow
 * ----------------------------------------------------------------------- */
static void
env_hide(const char* name) {
  struct var* v = var_create(name, V_LOCAL);

  if(v)
    v->flags = (v->flags & ~V_EXPORT) | V_UNSET;
}

int
builtin_env(int argc, char* argv[]) {
  int c, clear = 0, ret = 0, i;
  struct vartab vars;
  struct command cmd;
  stralloc names, sa;
  size_t n;
  char** envp;

  stralloc_init(&names);

  while((c = shell_getopt(argc, argv, "iu:")) > 0) {
    switch(c) {
      case 'i': clear = 1; break;
      case 'u':
        stralloc_cats(&names, shell_optarg);
        stralloc_catc(&names, '\0');
        break;
      default: builtin_invopt(argv); ret = 125; goto done;
    }
  }

  vartab_push(&vars, 1);

  /* -i: shadow everything that is exported */
  if(clear) {
    n = var_count(V_EXPORT);
    envp = var_export(alloc((n + 1) * sizeof(char*)));

    for(i = 0; envp[i]; i++) {
      stralloc name;

      stralloc_init(&name);
      stralloc_catb(&name, envp[i], str_chr(envp[i], '='));
      stralloc_nul(&name);
      env_hide(name.s);
      stralloc_free(&name);
    }

    alloc_free(envp);
  }

  for(i = 0; (size_t)i < names.len; i += str_len(names.s + i) + 1)
    env_hide(names.s + i);

  /* name=value operands */
  while(argv[shell_optind] && str_chr(argv[shell_optind], '=') < str_len(argv[shell_optind])) {
    stralloc_init(&sa);
    stralloc_copys(&sa, argv[shell_optind]);
    stralloc_nul(&sa);

    if(str_chr(sa.s, '=') == 0 || !var_setsa(&sa, V_EXPORT | V_LOCAL)) {
      builtin_errmsg(argv, argv[shell_optind], "invalid assignment");
      stralloc_free(&sa);
      ret = 125;
      vartab_pop(&vars);
      goto done;
    }

    stralloc_free(&sa);
    shell_optind++;
  }

  if(!argv[shell_optind]) {
    n = var_count(V_EXPORT);
    envp = var_export(alloc((n + 1) * sizeof(char*)));

    for(i = 0; envp[i]; i++) {
      buffer_puts(fd_out->w, envp[i]);
      buffer_putc(fd_out->w, '\n');
    }

    buffer_flush(fd_out->w);
    alloc_free(envp);
    vartab_pop(&vars);
    goto done;
  }

  /* env runs programs: a function of the same name is not a candidate */
  cmd = exec_hash(argv[shell_optind], H_FUNCTION);

  if(cmd.ptr) {
    ret = exec_command(&cmd, argc - shell_optind, &argv[shell_optind], 0);
  } else {
    errno = exec_lasterrno ? exec_lasterrno : ENOENT;
    ret = exec_error();
    errno = exec_lasterrno ? exec_lasterrno : ENOENT;
    builtin_error(argv, argv[shell_optind]);
  }

  vartab_pop(&vars);

done:
  stralloc_free(&names);
  return ret;
}
