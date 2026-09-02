#include "../fd.h"
#include "../history.h"
#include "../job.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/sig.h"
#include "../../lib/uint32.h"
#include "../var.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#include "builtin_config.h"

#if !WINDOWS_NATIVE
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <unistd.h>

#if WINDOWS_NATIVE && !defined(HAVE_GETPPID)
#include "../../lib/unix.h"
#endif

static struct var sh_ps1;
static struct var sh_ps2;
static struct var sh_ps3;
static struct var sh_path;
static struct var sh_ifs;

/* initialize the shell
 * ----------------------------------------------------------------------- */
void
sh_init(void) {
#ifdef DEBUG_OUTPUT
  debug_open();
#endif

  /* must run before anything else touches a signal's disposition
     (job_init() doesn't, but sig_catch(SIGCHLD, ...)/term_init() do,
     later in sh_main.c) -- see sig_snapshot()'s own comment. */
  sig_snapshot();

  job_init();

  /* get current uid and pid */
#if WINDOWS_NATIVE
  sh_uid = 0;
#else
  sh_uid = getuid();
#endif
  sh_pid = getpid();
  sh_shpid = sh_pid;

  uint32_seed(&sh_pid, sizeof(sh_pid));

  /* set PPID to parent process id */
#if defined(HAVE_GETPPID) || WINDOWS_NATIVE
  var_setvint("PPID", getppid(), 0);
#else
  var_setvint("PPID", 0, 0);
#endif

#ifdef BUILTIN_GETOPTS
  /* POSIX: "Whenever the shell is invoked, OPTIND shall be initialized to 1." */
  var_setvint("OPTIND", 1, 0);
#endif

  /* initialize variables if they're not set */
  var_import("PS1=\\s-\\v:\\w \\$ ", V_INIT, &sh_ps1);
  var_import("PS2=> ", V_INIT, &sh_ps2);
  var_import("PS3=~ ", V_INIT, &sh_ps3);
  var_import("PATH=/bin:/usr/bin", V_INIT, &sh_path);

  /* unlike PS1/PS2/PS3/PATH above, IFS must always start at the
     POSIX default, even if a different value was inherited via the
     environment -- so this is a plain var_import(), not V_INIT
     ("only set when unset"), letting it overwrite an inherited
     value instead of deferring to it. */
  var_import("IFS= \t\n", 0, &sh_ifs);

  /* initialize the shell environment */
  sh_getcwd(sh);
  var_setv("PWD", sh->cwd.s, sh->cwd.len, 0);

  sh_home = sh_gethome();

  uint32_seed(sh->cwd.s, sh->cwd.len);

  sh->arg.v = sh_argv;
  sh->arg.c = sh_argc;
  sh->arg.a = 0;
  sh->arg.s = 0;

  sh->umask = umask(sh->umask);
  umask(sh->umask);
}
