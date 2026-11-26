#include "builtin_config.h"

#if BUILTIN_PKILL

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../job.h"
#include "../../fdtable.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/open.h"
#include "../../../lib/fmt.h"
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Shared process matching logic with builtin_pgrep.c */
/* In a real implementation, this would be in a shared header */

/* Get the process name for a given PID from /proc/pid/stat */
static int
get_process_name(pid_t pid, char* buf, size_t buf_size) {
  int fd;
  char path[PATH_MAX], *p, *end;
  ssize_t name_len, name_offs, len;

  len = str_copy(path, "/proc/");
  len += fmt_long(&path[len], pid);
  len += str_copy(&path[len], "/stat");

  if((fd = open_read(path)) == -1)
    return -1;

  len = read(fd, buf, buf_size);

  close(fd);

  if(len <= 0)
    return -1;

  /* In /proc/pid/stat, the comm field is between parentheses */
  if((name_offs = byte_chr(buf, len, '(')) == len)
    return -1;

  p = buf + ++name_offs; /* skip '(' */

  len -= name_offs;

  if((name_len = byte_chr(&buf[name_offs], len, ')')) == len)
    return -1;

  if(name_len  >= buf_size)
    name_len = buf_size - 1;

  str_copyn(buf, &buf[name_offs], name_len);
  return 0;
}

/* Check if a process name matches the given pattern */
static int
match_pattern(const char* proc_name, const char* pattern, int exact, int icase) {
  int match = 1;

  if(exact) {
    if(str_len(proc_name) != str_len(pattern) || str_diff(proc_name, pattern))
      match = 0;
  } else {
    if(!icase) {
      if(strstr(proc_name, pattern) == NULL)
        match = 0;
    } else {
      /* Case-insensitive match */
      char* lower_name = malloc(str_len(proc_name) + 1);
      char* lower_pattern = malloc(str_len(pattern) + 1);

      if(lower_name && lower_pattern) {
        for(size_t i = 0; i < str_len(proc_name); i++)
          lower_name[i] = (char)tolower((unsigned char)proc_name[i]);
        lower_name[str_len(proc_name)] = '\0';

        for(size_t i = 0; i < str_len(pattern); i++)
          lower_pattern[i] = (char)tolower((unsigned char)pattern[i]);
        lower_pattern[str_len(pattern)] = '\0';

        if(strstr(lower_name, lower_pattern) == NULL)
          match = 0;
      }

      free(lower_name);
      free(lower_pattern);
    }
  }

  return match;
}

int
builtin_pkill(int argc, char* argv[]) {
  int c, sig = SIGTERM, inverse = 0, exact = 0, icase = 0;
  pid_t my_pid = getpid();
  const char* pattern = NULL;
  int ret = 0, pid_count = 0, signaled = 0;

  /* Check options - short options only, no --longopts */
  while((c = shell_getopt(argc, argv, "cde fgi:noprs t u U v w x X q V h")) > 0) {
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
      case 'h': /* help */ builtin_usage(argv[0]); return 0;
      default: return builtin_invopt(argv);
    }
  }

  /* Get the pattern argument */
  if(argc <= shell_optind) {
    builtin_error(argv, "pattern missing");
    return 1;
  }

  pattern = argv[shell_optind];

  /* Scan through /proc to find matching processes and send signals */
  DIR* dir;

  if(!(dir = opendir("/proc"))) {
    builtin_error(argv, "cannot open /proc");
    return 1;
  }

  while(1) {
    struct dirent* entry = readdir(dir);
    if(!entry)
      break;

    pid_t pid;
    if((pid = (pid_t)atoi(entry->d_name)) <= 0 || pid == my_pid)
      continue;

    char proc_name[256];
    if(get_process_name(pid, proc_name, sizeof(proc_name)) != 0)
      continue;

    /* Check if process matches */
    int matches = match_pattern(proc_name, pattern, exact, icase);

    if(inverse)
      matches = !matches;

    if(matches) {
      pid_count++;

      if(kill(pid, sig) != -1) {
        signaled++;
      } /* Error - process might have died or we don't have permission */
      else {}
    }
  }

  closedir(dir);

  /* Return appropriate exit status */
  if(pid_count == 0)
    /* No processes matched */
    return 1;

  /* For pkill: return 0 if at least one process was signaled,
     1 if none could be signaled, 2 if syntax error */
  if(signaled > 0)
    return 0;

  return 1;
}

#endif /* BUILTIN_PKILL */