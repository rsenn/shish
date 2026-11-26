#include "builtin_config.h"

#if BUILTIN_PGREP

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

const char help_pgrep[] = "    Look up processes based on name.\n"
                          "\n"
                          "    -c              print a count of matching processes\n"
                          "    -f              match against full command line\n"
                          "    -l              list process name as well as PID\n"
                          "    -v              negate the match\n"
                          "    -x              match exactly\n"
                          "\n"
                          "    pattern         regex to match\n";

static int
proc_matches(pid_t pid, const char* pattern, int full, int exact, int inverse, int icase) {
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
      if(exact) {
        if(str_equal(sa.s, pattern))
          match = 1;
      } else if(icase) {
        /* basic caseless substring */
        for(size_t j, i = 0; i <= sa.len - str_len(pattern); i++) {
          for(j = 0; j < str_len(pattern); j++)
            if(tolower((unsigned char)sa.s[i + j]) != tolower((unsigned char)pattern[j]))
              break;

          if(j == str_len(pattern)) {
            match = 1;
            break;
          }
        }
      } else if(strstr(sa.s, pattern))
        match = 1;
    }
  }

  stralloc_free(&sa);

  return inverse ? !match : match;
}

int
builtin_pgrep(int argc, char* argv[]) {
  int c, n = 0, full = 0, list = 0, exact = 0, inverse = 0, icase = 0, count = 0;
  DIR* d;
  struct dirent* e;

  while((c = shell_getopt(argc, argv, "cfxliv")) > 0) {
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

  if(!(d = opendir("/proc")))
    return builtin_error(argv, "/proc");

  while((e = readdir(d))) {
    pid_t pid;

    if(scan_ulong(e->d_name, (unsigned long*)&pid) == 0)
      continue;

    if(pid == getpid())
      continue;

    if(proc_matches(pid, pattern, full, exact, inverse, icase)) {
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

  if(count) {
    buffer_putulong(fd_out->w, n);
    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
#endif
