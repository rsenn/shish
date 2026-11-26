#include "builtin_config.h"

#if BUILTIN_SKILL || BUILTIN_SNICE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/scan.h"
#include "../../../lib/sig_internal.h"
#include "../../../lib/open.h"
#include "../../../lib/byte.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <signal.h>

const char help_skill[] = "    Send a signal to processes.\n"
                          "\n"
                          "    -f              fast mode\n"
                          "    -i              interactive use\n"
                          "    -l              list signal names\n"
                          "    -L              list signal names in a table\n"
                          "    -n              no action (simulation)\n"
                          "    -v              verbose\n"
                          "    -t tty          terminal expression\n"
                          "    -u user         user expression\n"
                          "    -p pid          pid expression\n"
                          "    -c command      command expression\n"
                          "\n"
                          "    A process matches if it satisfies any expression; at least one is required.\n";

const char help_snice[] = "    Change process priority (nice value).\n"
                          "\n"
                          "    -f              fast mode\n"
                          "    -i              interactive use\n"
                          "    -n              no action (simulation)\n"
                          "    -v              verbose\n"
                          "    -t tty          terminal expression\n"
                          "    -u user         user expression\n"
                          "    -p pid          pid expression\n"
                          "    -c command      command expression\n"
                          "\n"
                          "    A process matches if it satisfies any expression; at least one is required.\n";

#define SKILL_MAX 32

/* one expression: what it is compared with */
struct skill_expr {
  char type; /* 't' tty, 'u' user, 'p' pid, 'c' command */
  const char* s;
  unsigned long n; /* pid or uid */
};

static int
skill_list_signals(int table) {
  for(const sigtable_t* p = sigtable + 1; p->name; p++) {
    if(table) {
      buffer_putulong(fd_out->w, (unsigned long)p->number);
      buffer_putspace(fd_out->w);
    } else if(p > sigtable + 1) {
      buffer_putspace(fd_out->w);
    }

    buffer_puts(fd_out->w, p->name);

    if(table)
      buffer_putnlflush(fd_out->w);
  }

  if(!table)
    buffer_putnlflush(fd_out->w);

  return 0;
}

/* classify a bare expression: pid, user name, tty or command */
static int
skill_guess(const char* s) {
  size_t i;

  for(i = 0; s[i] >= '0' && s[i] <= '9'; i++) {}

  if(i && !s[i])
    return 'p';

  if(getpwnam(s))
    return 'u';

  if(!str_diffn(s, "tty", 3) || !str_diffn(s, "pts/", 4) || s[0] == '/')
    return 't';

  return 'c';
}

static int
skill_add(struct skill_expr* e, int* n, int type, const char* s) {
  struct passwd* pw;
  size_t len = str_len(s);

  if(*n == SKILL_MAX)
    return -1;

  e += *n;
  e->type = (char)type;
  e->s = s;

  if(type == 'p') {
    if(!len || scan_ulong(s, &e->n) != len)
      return -1;
  } else if(type == 'u') {
    if(len && scan_ulong(s, &e->n) == len) {
    } else if((pw = getpwnam(s)))
      e->n = pw->pw_uid;
    else
      return -1;
  }

  ++*n;
  return 0;
}

/* does process pid satisfy expression e? */
static int
skill_match(const struct skill_expr* e, pid_t pid) {
  char path[64], buf[256];
  size_t len;
  ssize_t r;
  int fd;
  struct stat st;

  if(e->type == 'p')
    return (unsigned long)pid == e->n;

  len = str_copy(path, "/proc/");
  len += fmt_ulong(path + len, (unsigned long)pid);

  if(e->type == 'u') {
    path[len] = '\0';
    return stat(path, &st) == 0 && (unsigned long)st.st_uid == e->n;
  }

  if(e->type == 't') {
    const char* t = e->s;

    str_copy(path + len, "/fd/0");

    if((r = readlink(path, buf, sizeof(buf) - 1)) <= 0)
      return 0;

    buf[r] = '\0';

    if(!str_diffn(t, "/dev/", 5))
      t += 5;

    return !str_diffn(buf, "/dev/", 5) && str_equal(buf + 5, t);
  }

  str_copy(path + len, "/comm");

  if((fd = open_read(path)) == -1)
    return 0;

  r = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if(r <= 0)
    return 0;

  if(buf[r - 1] == '\n')
    r--;

  buf[r] = '\0';
  return str_equal(buf, e->s);
}

/* a leading -SIG, -N (skill) or [+-]N (snice) is not an option */
static int
skill_lead(char* a, int is_snice, int* sig, int* prio) {
  const char* p = a + (*a == '-' || *a == '+');
  unsigned long v;
  size_t n = str_len(p);

  if(!n || (p == a && !is_snice))
    return 0;

  if(scan_ulong(p, &v) == n) {
    if(is_snice)
      *prio = *a == '-' ? -(int)v : (int)v;
    else
      *sig = (int)v;

    return 1;
  }

  if(!is_snice && a[0] == '-' && n > 1 && sig_byname(p) > 0) {
    *sig = sig_byname(p);
    return 1;
  }

  return 0;
}

int
builtin_skill(int argc, char* argv[]) {
  int c, sig = SIGTERM, priority = 4, interactive = 0, no_action = 0, verbose = 0, nexpr = 0, hits = 0, i;
  int is_snice = !str_diff(argv[0], "snice");
  struct skill_expr expr[SKILL_MAX];
  struct dirent* entry;
  DIR* dir;
  pid_t pid;

  /* take the signal (or priority) out of the option run, wherever it is;
     snice also accepts a bare priority as its very first argument */
  for(i = 1, c = is_snice; i < argc; i++, c = 0) {
    const char* a = argv[i];

    if((a[0] != '-' && a[0] != '+' && !c) || !str_diff(a, "--"))
      break;

    if(a[0] == '-' && a[1] && !a[2] && str_chr("tupc", a[1]) < 4) { /* takes an argument */
      i++;
      continue;
    }

    if(skill_lead(argv[i], is_snice, &sig, &priority)) {
      byte_copy(&argv[i], (size_t)(argc - i) * sizeof(argv[0]), &argv[i + 1]);
      argc--;
      i--;
    }
  }

  /* Check options - short options only, no --longopts */
  while((c = shell_getopt(argc, argv, "filLnvwt:u:p:c:")) > 0) {
    switch(c) {
      case 'f': break;
      case 'i': interactive = 1; break;
      case 'l': return skill_list_signals(0);
      case 'L': return skill_list_signals(1);
      case 'n': no_action = 1; break;
      case 'v': verbose = 1; break;
      case 'w': break;
      case 't':
      case 'u':
      case 'p':
      case 'c':
        if(skill_add(expr, &nexpr, c, shell_optarg))
          return builtin_errmsg(argv, shell_optarg, "invalid expression");

        break;
      default: return builtin_invopt(argv);
    }
  }

  (void)interactive;

  for(i = shell_optind; i < argc; i++)
    if(skill_add(expr, &nexpr, skill_guess(argv[i]), argv[i]))
      return builtin_errmsg(argv, argv[i], "invalid expression");

  /* without an expression every process would match */
  if(!nexpr)
    return builtin_errmsg(argv, "no process selected", "give a tty, user, pid or command");

  if(!(dir = opendir("/proc")))
    return builtin_error(argv, "cannot open /proc");

  while((entry = readdir(dir))) {
    if((pid = (pid_t)atoi(entry->d_name)) <= 0 || pid == getpid())
      continue;

    for(i = 0; i < nexpr; i++)
      if(skill_match(&expr[i], pid))
        break;

    if(i == nexpr)
      continue;

    if(!no_action && (is_snice ? setpriority(PRIO_PROCESS, pid, priority) : kill(pid, sig)) == -1) {
      builtin_errmsgn(argv, entry->d_name, str_len(entry->d_name), strerror(errno));
      continue;
    }

    hits++;

    if(verbose) {
      buffer_puts(fd_out->w, "pid ");
      buffer_putulong(fd_out->w, (unsigned long)pid);
      buffer_puts(fd_out->w, is_snice ? " reniced\n" : " signaled\n");
      buffer_flush(fd_out->w);
    }
  }

  closedir(dir);
  return hits ? 0 : 1;
}

int
builtin_snice(int argc, char* argv[]) {
  return builtin_skill(argc, argv);
}

#endif /* BUILTIN_SKILL || BUILTIN_SNICE */
