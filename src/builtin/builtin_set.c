#include <shell.h>
#include "sh.h"
#include "fd.h"
#include "vartab.h"
#include "builtin.h"

/* set arguments of flags
 * ----------------------------------------------------------------------- */
int builtin_set(int argc, char **argv)
{
  int c;
  
  /* check options */
  while((c = shell_getopt(argc, argv, "LP")) > 0)
  {
    switch(c)
    {
      default: builtin_invopt(argv); return 1;
    }
  }
  
  if(argv[shell_optind])
    sh_setargs(&argv[shell_optind], 1);
  else
    vartab_print(V_DEFAULT);
  
  return 0;
}

