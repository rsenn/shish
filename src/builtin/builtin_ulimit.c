#include "builtin_config.h"

#if BUILTIN_ULIMIT

#include "../builtin.h"
#include "../sh.h"
#include "../../lib/alloc.h"
#include "../fdtable.h"
#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/scan.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"
#include <errno.h>
#include <string.h>
#if !WINDOWS_NATIVE
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#if WINDOWS_NATIVE
const char help_ulimit[] = "    Not available on this platform.\n";

int
builtin_ulimit(int argc, char* argv[]) {
  builtin_errmsg(argv, "ulimit", "not supported on this platform");
  return 1;
}
#else
/* one row per resource: option letter, rlimit, bytes per unit shown (0: a plain count) */
static const struct ulimit_res {
  char letter;
  int resource;
  unsigned long unit;
  const char* name;
  const char* units;
} ulimit_res[] = {
    {'c', RLIMIT_CORE, 1024, "core file size", "kbytes"},
    {'d', RLIMIT_DATA, 1024, "data seg size", "kbytes"},
    {'f', RLIMIT_FSIZE, 512, "file size", "512-byte blocks"},
    {'n', RLIMIT_NOFILE, 0, "open files", "count"},
    {'s', RLIMIT_STACK, 1024, "stack size", "kbytes"},
    {'t', RLIMIT_CPU, 0, "cpu time", "seconds"},
    {'v', RLIMIT_AS, 1024, "virtual memory", "kbytes"},
#ifdef RLIMIT_NPROC
    {'u', RLIMIT_NPROC, 0, "max user processes", "count"},
#endif
#ifdef RLIMIT_MEMLOCK
    {'l', RLIMIT_MEMLOCK, 1024, "max locked memory", "kbytes"},
#endif
#ifdef RLIMIT_RSS
    {'m', RLIMIT_RSS, 1024, "max memory size", "kbytes"},
#endif
};

#define ULIMIT_N (sizeof(ulimit_res) / sizeof(ulimit_res[0]))

const char help_ulimit[] = "    Show or set the resource limits of the shell and what it starts.\n"
                           "\n"
                           "    -a              show every limit\n"
                           "    -H, -S          the hard or the soft limit (default: show the soft one,\n"
                           "                    set both)\n"
                           "    -f              file size, in 512-byte blocks (the default resource)\n"
                           "    -c -d -s -v     core, data segment, stack, address space, in kbytes\n"
                           "    -l -m           locked and resident memory, in kbytes\n"
                           "    -n -u -t        open files, processes, cpu seconds\n"
                           "    limit           a number, 'unlimited', 'hard' or 'soft'\n";

/* A "( )" or "$( )" runs in this process, so a limit it sets would outlive it. The first change in
 * an env snapshots every limit and hangs a finalizer on the env that puts them back when sh_pop()
 * leaves it. A hard limit lowered unprivileged cannot be raised again: that one stays lowered.
 * ----------------------------------------------------------------------- */
struct ulimit_fence {
  struct ulimit_fence* up;
  struct env* env;
  pid_t pid;
  struct rlimit saved[ULIMIT_N];
  struct handler fin;
};

static struct ulimit_fence* ulimit_top;

static void
ulimit_restore(void) {
  struct ulimit_fence* f = ulimit_top;
  size_t k;

  for(k = 0; k < ULIMIT_N; k++)
    setrlimit(ulimit_res[k].resource, &f->saved[k]);

  ulimit_top = f->up;
  alloc_free(f);
}

static void
ulimit_fence_enter(void) {
  struct ulimit_fence* f;
  size_t k;

  if(ulimit_top && ulimit_top->pid != sh_pid)
    ulimit_top = NULL; /* a fork() child keeps what it has */

  if(sh == &sh_root || (ulimit_top && ulimit_top->env == sh))
    return;

  f = alloc_zero(sizeof(*f));
  f->up = ulimit_top;
  f->env = sh;
  f->pid = sh_pid;

  for(k = 0; k < ULIMIT_N; k++)
    getrlimit(ulimit_res[k].resource, &f->saved[k]);

  f->fin.fn = ulimit_restore;
  f->fin.next = sh->finalizers;
  sh->finalizers = &f->fin;
  ulimit_top = f;
}

static void
ulimit_put(rlim_t v, unsigned long unit) {
  char buf[FMT_ULONG + 1];

  if(v == RLIM_INFINITY) {
    buffer_puts(fd_out->w, "unlimited");
  } else {
    if(unit)
      v /= unit;

    buffer_put(fd_out->w, buf, fmt_ulonglong(buf, (unsigned long long)v));
  }
}

static void
ulimit_show(const struct ulimit_res* r, int hard, int labelled) {
  struct rlimit rl;

  getrlimit(r->resource, &rl);

  if(labelled) {
    size_t n = str_len(r->name), i;

    buffer_puts(fd_out->w, r->name);
    buffer_putc(fd_out->w, ' ');

    for(i = n; i < 20; i++)
      buffer_putc(fd_out->w, ' ');

    {
      size_t u = str_len(r->units);

      buffer_putm_internal(fd_out->w, "(", r->units, ", -", 0);
      buffer_putc(fd_out->w, r->letter);
      buffer_puts(fd_out->w, ") ");

      for(i = u; i < 15; i++)
        buffer_putc(fd_out->w, ' ');
    }
  }

  ulimit_put(hard ? rl.rlim_max : rl.rlim_cur, r->unit);
  buffer_putnlflush(fd_out->w);
}

int
builtin_ulimit(int argc, char* argv[]) {
  int c, hard = 0, soft = 0, all = 0, nsel = 0, i, ret = 0;
  const struct ulimit_res* sel[ULIMIT_N];
  char* arg;
  size_t k;

  while((c = shell_getopt(argc, argv, "HSacdfnstuvlm")) > 0) {
    switch(c) {
      case 'H': hard = 1; break;
      case 'S': soft = 1; break;
      case 'a': all = 1; break;
      default:
        for(k = 0; k < ULIMIT_N; k++)
          if(ulimit_res[k].letter == c) {
            if(nsel < (int)ULIMIT_N)
              sel[nsel++] = &ulimit_res[k];
            break;
          }

        if(k == ULIMIT_N) {
          builtin_errmsg(argv, argv[shell_optind - 1], "limit not available here");
          return 1;
        }
    }
  }

  arg = argv[shell_optind];

  if(arg && argv[shell_optind + 1]) {
    builtin_errmsg(argv, argv[shell_optind + 1], "too many arguments");
    return 1;
  }

  if(all) {
    for(k = 0; k < ULIMIT_N; k++)
      ulimit_show(&ulimit_res[k], hard && !soft, 1);

    return 0;
  }

  if(!nsel) {
    for(k = 0; k < ULIMIT_N; k++)
      if(ulimit_res[k].letter == 'f')
        sel[nsel++] = &ulimit_res[k];
  }

  if(!arg) {
    for(i = 0; i < nsel; i++)
      ulimit_show(sel[i], hard && !soft, nsel > 1);

    return 0;
  }

  if(nsel != 1) {
    builtin_errmsg(argv, arg, "one resource at a time when setting");
    return 1;
  }

  {
    struct rlimit rl;
    rlim_t v;
    uint64 n;

    getrlimit(sel[0]->resource, &rl);

    if(!str_diff(arg, "unlimited"))
      v = RLIM_INFINITY;
    else if(!str_diff(arg, "hard"))
      v = rl.rlim_max;
    else if(!str_diff(arg, "soft"))
      v = rl.rlim_cur;
    else if(*arg && scan_ulonglong(arg, &n) == str_len(arg))
      v = (rlim_t)n * (sel[0]->unit ? sel[0]->unit : 1);
    else {
      builtin_errmsg(argv, (char*)arg, "invalid number");
      return 1;
    }

    /* no -H/-S: both; lowering the hard limit drags the soft one down with it */
    if(hard || !soft)
      rl.rlim_max = v;

    if(soft || !hard)
      rl.rlim_cur = v;

    if(rl.rlim_cur != RLIM_INFINITY && rl.rlim_max != RLIM_INFINITY && rl.rlim_cur > rl.rlim_max)
      rl.rlim_cur = rl.rlim_max;

    ulimit_fence_enter();

    if(setrlimit(sel[0]->resource, &rl) == -1) {
      builtin_errmsg(argv, "cannot modify limit", strerror(errno));
      ret = 1;
    }
  }

  return ret;
}
#endif /* !WINDOWS_NATIVE */
#endif /* BUILTIN_ULIMIT */
