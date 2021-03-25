#include "../builtin.h"
#include "../sh.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"
#include "../../lib/open.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_mktemp(int argc, char* argv[]) {
  int c;
  int directory = 0, quiet = 0, temp = 0, printonly = 0;
  const char* base = "/tmp";
  stralloc name;
  size_t i;
  static const char alphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                                  'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

  /* check options */
  while((c = shell_getopt(argc, argv, "dqtp:u")) > 0) {
    switch(c) {
      case 'd': directory = 1; break;
      case 'q': quiet = 1; break;
      case 't': temp = 1; break;
      case 'u': printonly = 1; break;
      case 'p':
        temp = 1;
        base = argv[shell_optind];
        break;
      default: builtin_invopt(argv); return 1;
    }
  }
  stralloc_init(&name);
  if(temp) {
    stralloc_cats(&name, base);
    stralloc_catc(&name, '/');
  }
  i = name.len;

  if(argv[shell_optind]) {
    stralloc_cats(&name, argv[shell_optind]);
  } else {
    size_t len = str_rchr(sh_argv0, '/');
    stralloc_cats(&name, sh_argv0[len] ? &sh_argv0[len + 1] : sh_argv0);
    stralloc_cats(&name, "-XXXXXXXX");
  }

  while(i < name.len) {
    if(name.s[i] == 'X') {
      uint32 r = uint32_random();
      name.s[i] = alphabet[r % sizeof(alphabet)];
    }
    i++;
  }

  if(!printonly) {
    int fd;
    stralloc_nul(&name);
    if((fd = (directory ? mkdir(name.s, 0700) : open_excl(name.s))) == -1) {
      if(!quiet)
        builtin_error(argv, name.s);
      return 1;
    }
    close(fd);
  }

  buffer_putsa(fd_out->w, &name);
  buffer_putnlflush(fd_out->w);

  return 0;
}
