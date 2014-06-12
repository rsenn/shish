#include <shell.h>
#include "sh.h"
#include "exec.h"
#include "builtin.h"

/* exec built-in 
 * 
 * ----------------------------------------------------------------------- */
int builtin_exec(int argc, char **argv)
{
  int c;
  int nullenv = 0;
  int dash = 0;
  char *argv0 = NULL;
  enum hash_id id;
  union command cmd;

  /* check options, -l for login dash, -c for null env, -a to set argv[0] */
  while((c = shell_getopt(argc, argv, "cla:")) > 0)
  {
    switch(c)
    {
      case 'c': nullenv = 1; break;
      case 'l': dash = 1; break;
      case 'a': argv0 = shell_optarg; break;
      default: builtin_invopt(argv); return 1;
    }
  }
  
  // TODO
  (void)dash;
  (void)nullenv;
  
  /* no arguments? return now! */
  if(argv[shell_optind] == NULL)
    return 0;

  /* look up the command and exec if found */
  cmd = exec_hash(argv[shell_optind], &id);

  if(cmd.ptr)
  {
    /* command name was set, replace argv[shell_optind] */
    if(argv0)
      argv[shell_optind] = argv0;

    /* try to exec */
    exec_command(id, cmd, argc - shell_optind, &argv[shell_optind], 1, NULL);
  }
  
  /* at this point the exec stuff failed */
  sh_error(argv[shell_optind]);
  return exec_error();
}

