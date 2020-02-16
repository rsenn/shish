#include "../builtin.h"
#include "../sh.h"
#include "../fd.h"

#include <sys/utsname.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_uname(int argc, char** argv) {
  int c, ret;
  stralloc dir;
  int all = 0, machine = 0, nodename = 0, kern_release = 0, kern_name = 0, processor = 0, kern_version = 0,
      hw_platform = 0, os_name = 0;

  const char* base = "/tmp";
  char* out;
  struct utsname unbuf;
  size_t i;

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

  if(uname(&unbuf) == -1) {
    builtin_error(argv, 0);
    return 1;
  }

  if(kern_release) {
    out = unbuf.release;
  } else if(kern_version) {
    out = unbuf.version;
  } else if(machine) {
    out = unbuf.machine;
  } else if(nodename) {
    out = unbuf.nodename;
  } else if(hw_platform) {
    out = "unknown";
  } else {
    out = unbuf.sysname;
  }

  buffer_puts(fd_out->w, out);
  buffer_putnlflush(fd_out->w);

  return 0;
}
