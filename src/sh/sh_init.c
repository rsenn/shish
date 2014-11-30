#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include "shell.h"
#include "sh.h"
#include "var.h"
#include "job.h"
#include "fd.h"
#include "uint32.h"
#include "history.h"

static struct var sh_ps1;
static struct var sh_ps2;
static struct var sh_ps3;
static struct var sh_path;
static struct var sh_ifs;

/* initialize the shell 
 * ----------------------------------------------------------------------- */
void sh_init(void)
{
  job_init();

  /* get current uid and pid */
  sh_uid = getuid();
  sh_pid = getpid();

  uint32_seed(&sh_pid, sizeof(sh_pid));

  /* initialize variables if they're not set */
  var_import("PS1=\\s-\\v:\\w \\$ ", V_INIT, &sh_ps1);
  var_import("PS2=> ", V_INIT, &sh_ps2);
  var_import("PS3=~ ", V_INIT, &sh_ps3);
  var_import("PATH=/bin:/usr/bin", V_INIT, &sh_path);
  var_import("IFS= \t\n", V_INIT, &sh_ifs);

  /* initialize the shell environment */
  sh_getcwd(sh);
  sh_home = sh_gethome();

  uint32_seed(sh->cwd.s, sh->cwd.len);

  sh->arg.v = sh_argv;
  sh->arg.c = sh_argc;
  sh->arg.a = 0;
  sh->arg.s = 0;

  /* set up signal handling */
/*  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);*/
}


