#include "builtin_config.h"

#if BUILTIN_PKILL

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../job.h"
#include "../../re-compat.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/open.h"
#include "../../../lib/fmt.h"
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

const char help_pkill[] = "    Signal processes based on name.\n"
                          "\n"
                          "    -i              ignore case when matching\n"
                          "    -v              negate the match\n"
                          "    -x              match exactly\n"
                          "\n"
                          "    pattern         regex to match\n";

/* Get the process name for a given PID from /proc/pid/stat */
static int
get_process_name(pid_t pid, char* buf, size_t buf_size) {
  int fd;
  char path[(FMT_LONG + 7) & (~15)];
  ssize_t i, j, len;

  if(buf_size == 0)
    return -1;

  len = str_copy(path, "/proc/");
  len += fmt_long(&path[len], pid);
  len += str_copy(&path[len], "/stat");

  if((fd = open_read(path)) == -1)
    return -1;

  len = read(fd, buf, buf_size - 1);
  close(fd);

  if(len <= 0)
    return -1;

  buf[len] = '\0';

  /* In /proc/pid/stat, the comm field is between parentheses */
  if((i = byte_chr(buf, (size_t)len, '(')) == (size_t)len)
    return -1;

  i++;
  len -= i;

  if((j = byte_chr(&buf[i], (size_t)len, ')')) == (size_t)len)
    return -1;

  byte_copyr(buf, (size_t)j, &buf[i]);
  buf[j] = '\0';
  return 0;
}

/* Check if a process name matches the given pattern */
static int
match_pattern(const char* proc_name, rx_t* re, const char* pattern, int exact) {
  if(exact)
    return str_len(proc_name) == str_len(pattern) && str_equal(proc_name, pattern);

  return rx_test(re, proc_name, str_len(proc_name)) != 0;
}

int
builtin_pkill(int argc, char* argv[]) {
  int c, sig = SIGTERM, inverse = 0, exact = 0, icase = 0;
  pid_t pid;
  const char* pattern = NULL;
  char proc_name[256];
  int pid_count = 0, signaled = 0;
  rx_t re;
  DIR* dir;

  rx_init(&re);

  /* Check options - short options only, no --longopts */
  while((c = shell_getopt(argc, argv, "cde fgi noprs t u U v w x X q V h")) > 0) {
    switch(c) {
      case 'c': /* count - not typical for pkill, but */ break;
      case 'd': /* delimiter */ break;
      case 'e': /* echo - show process being killed */ break;
      case 'f': exact = 1; break; /* full command line */
      case 'g': /* process group */ break;
      case 'i': icase = 1; break; /* ignore case */
      case 'n': /* newest */ break;
      case 'o': /* oldest */ break;
      case 'p': /* pidwait */ break;
      case 'r': /* runstates */ break;
      case 's': /* session */ break;
      case 't': /* terminal */ break;
      case 'u': /* euid */ break;
      case 'U': /* uid */ break;
      case 'v': inverse = 1; break;
      case 'w': /* lightweight */ break;
      case 'x': exact = 1; break; /* exact match */
      case 'X': /* exact match (alias) */ break;
      case 'q': /* queue */ break;
      case 'Q': /* queue (alias) */ break;
      default: return builtin_invopt(argv);
    }
  }

  /* Get the pattern argument */
  if(argc <= shell_optind) {
    builtin_error(argv, "pattern missing");
    return 1;
  }

  pattern = argv[shell_optind];

  if(!exact && rx_compile(&re, pattern, RX_ERE | (icase ? RX_ICASE : 0))) {
    builtin_errmsg(argv, (char*)pattern, "invalid regular expression");
    return 2;
  }

  /* Scan through /proc to find matching processes and send signals */
  if(!(dir = opendir("/proc"))) {
    if(!exact)
      rx_free(&re);

    builtin_error(argv, "cannot open /proc");
    return 1;
  }

  while(1) {
    struct dirent* entry;

    if(!(entry = readdir(dir)))
      break;

    if((pid = (pid_t)atoi(entry->d_name)) <= 0 || pid == getpid())
      continue;

    if(get_process_name(pid, proc_name, sizeof(proc_name)) != 0)
      continue;

    /* Check if process matches */
    int matches = match_pattern(proc_name, &re, pattern, exact);

    if(inverse)
      matches = !matches;

    if(matches) {
      pid_count++;

      if(kill(pid, sig) != -1)
        signaled++;
    }
  }

  closedir(dir);

  if(!exact)
    rx_free(&re);

  /* Return appropriate exit status */
  if(pid_count == 0)
    return 1;

  if(signaled > 0)
    return 0;

  return 1;
}

#endif /* BUILTIN_PKILL */