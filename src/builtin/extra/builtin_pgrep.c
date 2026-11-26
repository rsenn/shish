#include "builtin_config.h"

#if BUILTIN_PGREP

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../re-compat.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

const char help_pgrep[] = "    Look up processes based on name.\n"
                          "\n"
                          "    -c              print a count of matching processes\n"
                          "    -f              match against full command line\n"
                          "    -i              ignore case when matching\n"
                          "    -l              list process name as well as PID\n"
                          "    -v              negate the match\n"
                          "    -x              match exactly\n"
                          "\n"
                          "    pattern         regex to match\n";

static int
proc_matches(pid_t pid, rx_t* re, const char* pattern, int full, int exact, int inverse) {
  char path[64];
  int fd, match = 0;
  stralloc sa;

  stralloc_init(&sa);
  fmt_ulong(path, (unsigned long)pid);
  stralloc_copys(&sa, "/proc/");
  stralloc_cats(&sa, path);
  stralloc_cats(&sa, full ? "/cmdline" : "/comm");

  if((fd = open_read(sa.s)) != -1) {
    stralloc_zero(&sa);

    if(readclose_append(fd, &sa, 1024) > 0) {
      stralloc_nul(&sa);

      if(exact) {
        if(str_equal(sa.s, pattern))
          match = 1;
      } else if(rx_test(re, sa.s, sa.len)) {
        match = 1;
      }
    }
  }

  stralloc_free(&sa);

  return inverse ? !match : match;
}

int
builtin_pgrep(int argc, char* argv[]) {
  int c, n = 0, full = 0, list = 0, exact = 0, inverse = 0, icase = 0, count = 0;
  rx_t re;
  DIR* d;
  struct dirent* e;

  rx_init(&re);

  while((c = shell_getopt(argc, argv, "cfxliv h")) > 0) {
    switch(c) {
      case 'c': count = 1; break;
      case 'f': full = 1; break;
      case 'x': exact = 1; break;
      case 'l': list = 1; break;
      case 'i': icase = 1; break;
      case 'v': inverse = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(shell_optind >= argc)
    return builtin_errmsg(argv, "no pattern", NULL);

  const char* pattern = argv[shell_optind];

  if(!exact && rx_compile(&re, pattern, RX_ERE | (icase ? RX_ICASE : 0))) {
    builtin_errmsg(argv, (char*)pattern, "invalid regular expression");
    return 1;
  }

  if(!(d = opendir("/proc"))) {
    if(!exact)
      rx_free(&re);

    return builtin_error(argv, "/proc");
  }

  while((e = readdir(d))) {
    pid_t pid;

    if(scan_ulong(e->d_name, (unsigned long*)&pid) == 0)
      continue;

    if(pid == getpid())
      continue;

    if(proc_matches(pid, &re, pattern, full, exact, inverse)) {
      if(count)
        n++;
      else {
        buffer_putulong(fd_out->w, (unsigned long)pid);

        if(list) {
          int fd;
          char path[64];

          buffer_putspace(fd_out->w);

          /* print name */
          stralloc sa;
          stralloc_init(&sa);
          fmt_ulong(path, (unsigned long)pid);
          stralloc_copys(&sa, "/proc/");
          stralloc_cats(&sa, path);
          stralloc_cats(&sa, "/comm");

          if((fd = open_read(sa.s)) != -1) {
            stralloc_zero(&sa);
            readclose_append(fd, &sa, 1024);
            buffer_puts(fd_out->w, sa.s);
          }

          stralloc_free(&sa);
        }

        buffer_putnlflush(fd_out->w);
      }
    }
  }

  closedir(d);

  if(!exact)
    rx_free(&re);

  if(count) {
    buffer_putulong(fd_out->w, n);
    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
#endif
