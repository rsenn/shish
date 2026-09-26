#include "../../builtin.h"
#include "../../sh.h"
#include "../../fdtable.h"
#include "config.h"
#include "../../../lib/windoze.h"
#include "../../../lib/unix.h"

#ifndef UNIX_UTSNAME
#include <sys/utsname.h>
#endif

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_uname[] = "    Print system information.\n"
                          "\n"
                          "    -a              -s -n -r -v -m\n"
                          "    -s              kernel/OS name (default)\n"
                          "    -n              network hostname\n"
                          "    -r              kernel release\n"
                          "    -v              kernel version\n"
                          "    -m              machine hardware name\n"
                          "    -i              hardware platform\n";

int
builtin_uname(int argc, char* argv[]) {
  int c, all = 0, machine = 0, nodename = 0, kern_release = 0, kern_name = 0, processor = 0,
         kern_version = 0, hw_platform = 0, os_name = 0;
  struct utsname unbuf;

  /* check options */
  while((c = shell_getopt(argc, argv, "amnrspvio")) > 0) {
    switch(c) {
      case 'a': all = 1; break;
      case 'm': machine = 1; break;
      case 'n': nodename = 1; break;
      case 'r': kern_release = 1; break;
      case 's': kern_name = 1; break;
      case 'p': processor = 1; break;
      case 'v': kern_version = 1; break;
      case 'i': hw_platform = 1; break;
      case 'o': os_name = 1; break;

      default: builtin_invopt(argv); return 1;
    }
  }

  if(argc > shell_optind) {
    builtin_errmsg(argv, argv[shell_optind], "extra operand");
    return 1;
  }

  if(uname(&unbuf) == -1) {
    builtin_error(argv, 0);
    return 1;
  }

  /* fixed order s n r v m; "-a" or no option selects the default set */
  {
    const struct {
      int on;
      const char* s;
    } f[] = {
        {kern_name || all, unbuf.sysname},
        {nodename || all, unbuf.nodename},
        {kern_release || all, unbuf.release},
        {kern_version || all, unbuf.version},
        {machine || all, unbuf.machine},
        {hw_platform, "unknown"},
        {processor, "unknown"},
        {os_name, unbuf.sysname},
    };
    int i, n = 0,
           any = all || kern_name || nodename || kern_release || kern_version || machine ||
                 hw_platform || processor || os_name;

    for(i = 0; i < (int)(sizeof(f) / sizeof(f[0])); i++) {
      if(!(f[i].on || (!any && i == 0)))
        continue;

      if(n++)
        buffer_putc(fd_out->w, ' ');

      buffer_puts(fd_out->w, f[i].s);
    }
  }

  buffer_putnlflush(fd_out->w);

  return 0;
}
