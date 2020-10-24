#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../builtin.h"
#include "../../lib/byte.h"
#include "../fd.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"
#include <errno.h>
#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

/* sets or displays current hostname
 * ----------------------------------------------------------------------- */
int
builtin_hostname(int argc, char* argv[]) {
  int c;
  int force = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, "f")) > 0) {
    switch(c) {
      case 'f': force = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* if there is an argument we set it as new hostname */
  if(argv[shell_optind]) {
    unsigned long n;

    n = str_len(argv[shell_optind]);

    /* unless force is set and if the new hostname is
       the same as the current then do not update it */
    if(!force && n == sh_hostname.len && !byte_diff(sh_hostname.s, n, argv[shell_optind]))
      return 0;

#if defined(HAVE_SETHOSTNAME) || !WINDOWS_NATIVE
      /* set the supplied hostname */
#if !WINDOWS_NATIVE
    if(sethostname(argv[shell_optind], n))
#else
    errno = ENOSYS;
#endif
    {
      /* report any error */
      builtin_error(argv, "sethostname");
      return 1;
    }
#endif

    /* on success update internal hostname */
    stralloc_copyb(&sh_hostname, argv[shell_optind], n);
  }
  /* if there is no argument we display the current hostname */
  else {
    /* force re-get of hostname by clearing it now */
    if(force)
      stralloc_zero(&sh_hostname);

    /* get hostname if it isn't there */
    if(sh_hostname.len == 0)
      shell_gethostname(&sh_hostname);

    /* report errors */
    if(sh_hostname.len == 0) {
      builtin_error(argv, "gethostname");
      return 1;
    }

    /* finally output the hostname */
    buffer_putsa(fd_out->w, &sh_hostname);
    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
