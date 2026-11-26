#include "builtin_config.h"

#if BUILTIN_VMSTAT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/byte.h"
#include "../../../lib/fmt.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/uint64.h"
#include <unistd.h>

const char help_vmstat[] = "    Report virtual memory statistics.\n"
                           "\n"
                           "    -a              show active and inactive memory\n"
                           "    -f              show the number of forks since boot\n"
                           "    -s              show event counters and memory as a table\n"
                           "    -S unit         k (1000), K (1024, default), m (1000000) or M (1048576)\n"
                           "\n"
                           "    delay           seconds between reports; the first covers the time since boot\n"
                           "    count           number of reports (default: 1 without delay, else endless)\n";

enum { CPU_USER, CPU_NICE, CPU_SYS, CPU_IDLE, CPU_WAIT, CPU_IRQ, CPU_SOFTIRQ, CPU_STEAL, CPU_N, };

/* counters that grow; a report line shows their change over the interval */
struct vm_sample {
  uint64 cpu[CPU_N];
  uint64 intr, ctxt, pgin, pgout, swin, swout;
};

/* a "table" row: key in a file, label, and whether it is kB (memory) */
struct vm_row {
  const char* file;
  const char* key;
  const char* label;
  unsigned kb : 1;
};

static const struct vm_row rows[] = {
    {"/proc/meminfo", "MemTotal", "total memory", 1},
    {"/proc/meminfo", "Active", "active memory", 1},
    {"/proc/meminfo", "Inactive", "inactive memory", 1},
    {"/proc/meminfo", "MemFree", "free memory", 1},
    {"/proc/meminfo", "Buffers", "buffer memory", 1},
    {"/proc/meminfo", "Cached", "swap cache", 1},
    {"/proc/meminfo", "SwapTotal", "total swap", 1},
    {"/proc/meminfo", "SwapFree", "free swap", 1},
    {"/proc/stat", "cpu", "non-nice user cpu ticks", 0},
    {"/proc/stat", "intr", "interrupts", 0},
    {"/proc/stat", "ctxt", "CPU context switches", 0},
    {"/proc/stat", "btime", "boot time", 0},
    {"/proc/stat", "processes", "forks", 0},
    {"/proc/vmstat", "pgpgin", "pages paged in", 0},
    {"/proc/vmstat", "pgpgout", "pages paged out", 0},
    {"/proc/vmstat", "pswpin", "pages swapped in", 0},
    {"/proc/vmstat", "pswpout", "pages swapped out", 0},
};

/* the text after "key:" or "key " at the start of a line, or 0 */
static const char*
find_line(const char* s, const char* key) {
  size_t n = str_len(key);

  for(;;) {
    if(!str_diffn(s, key, n) && (s[n] == ':' || s[n] == ' ')) {
      s += n + 1;

      while(*s == ' ' || *s == '\t')
        s++;

      return s;
    }

    s += str_chr(s, '\n');

    if(!*s++)
      return 0;
  }
}

/* up to n blank-separated numbers; how many were read */
static int
nums(const char* p, uint64* v, int n) {
  int i;

  for(i = 0; i < n; i++) {
    size_t k;

    while(*p == ' ')
      p++;

    if(!(k = scan_ulonglong(p, &v[i])))
      break;

    p += k;
  }

  return i;
}

/* value of key in the text s; 0 if absent */
static uint64
value(const char* s, const char* key) {
  uint64 v = 0;
  const char* p;

  if((p = find_line(s, key)))
    nums(p, &v, 1);

  return v;
}

static void
read_sample(struct vm_sample* m, stralloc* sa) {
  const char* p;

  byte_zero(m, sizeof(*m));

  if(slurp("/proc/stat", sa) == 0) {
    if((p = find_line(sa->s, "cpu")))
      nums(p, m->cpu, CPU_N);

    m->intr = value(sa->s, "intr");
    m->ctxt = value(sa->s, "ctxt");
  }

  if(slurp("/proc/vmstat", sa) == 0) {
    m->pgin = value(sa->s, "pgpgin");
    m->pgout = value(sa->s, "pgpgout");
    m->swin = value(sa->s, "pswpin");
    m->swout = value(sa->s, "pswpout");
  }
}

/* right-aligned in w columns */
static void
put_num(uint64 v, int w) {
  char buf[FMT_ULONG];
  size_t n = fmt_ulonglong(buf, v);

  if(w <= (int)n)
    w = (int)n + 1;

  buffer_putnspace(fd_out->w, w - (int)n);

  buffer_put(fd_out->w, buf, n);
}

static void
put_head(int active) {
  buffer* o = fd_out->w;

  buffer_puts(o, "procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----\n");
  buffer_puts(o, " r  b   swpd   free   ");
  buffer_puts(o, active ? "inact active" : "buff  cache");
  buffer_puts(o, "   si   so    bi    bo   in   cs us sy id wa st\n");
}

/* one report line: instantaneous memory, plus the change since prev over secs */
static void
put_line(const struct vm_sample* cur, const struct vm_sample* prev, uint64 secs, int active, uint64 unit, stralloc* sa) {
  uint64 d[CPU_N], total = 0, run, blk, mem[4];
  int i;

  if(!secs)
    secs = 1;

  for(i = 0; i < CPU_N; i++)
    total += d[i] = cur->cpu[i] - prev->cpu[i];

  if(!total)
    total = 1;

  slurp("/proc/stat", sa);
  run = value(sa->s, "procs_running");
  blk = value(sa->s, "procs_blocked");
  slurp("/proc/meminfo", sa);
  mem[0] = value(sa->s, "SwapTotal") - value(sa->s, "SwapFree");
  mem[1] = value(sa->s, "MemFree");
  mem[2] = value(sa->s, active ? "Inactive" : "Buffers");
  mem[3] = value(sa->s, active ? "Active" : "Cached");

  put_num(run, 2);
  put_num(blk, 3);

  for(i = 0; i < 4; i++)
    put_num(mem[i] * 1024 / unit, i == 1 || i == 3 ? 8 : 7);

  put_num((cur->swin - prev->swin) / secs, 5);
  put_num((cur->swout - prev->swout) / secs, 5);
  put_num((cur->pgin - prev->pgin) / secs, 6);
  put_num((cur->pgout - prev->pgout) / secs, 6);
  put_num((cur->intr - prev->intr) / secs, 5);
  put_num((cur->ctxt - prev->ctxt) / secs, 5);
  put_num((d[CPU_USER] + d[CPU_NICE]) * 100 / total, 3);
  put_num((d[CPU_SYS] + d[CPU_IRQ] + d[CPU_SOFTIRQ]) * 100 / total, 3);
  put_num(d[CPU_IDLE] * 100 / total, 3);
  put_num(d[CPU_WAIT] * 100 / total, 3);
  put_num(d[CPU_STEAL] * 100 / total, 3);
  buffer_putnlflush(fd_out->w);
}

/* the -s table (all == 0: only the fork count, for -f) */
static void
put_table(uint64 unit, char ch, int all, stralloc* sa) {
  buffer* o = fd_out->w;
  size_t i;

  for(i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
    const struct vm_row* r = &rows[i];
    uint64 v;

    if(!all && !str_equal(r->key, "processes"))
      continue;

    if(slurp(r->file, sa) == -1)
      continue;

    v = value(sa->s, r->key);

    if(r->kb)
      v = v * 1024 / unit;

    if(all) {
      put_num(v, 13);
      buffer_putspace(o);

      if(r->kb) {
        buffer_putc(o, ch);
        buffer_putspace(o);
      }
    } else {
      buffer_putulonglong(o, v);
      buffer_putspace(o);
    }

    buffer_puts(o, r->label);
    buffer_putnlflush(o);
  }
}

int
builtin_vmstat(int argc, char* argv[]) {
  struct vm_sample prev, cur;
  stralloc sa = {0};
  uint64 unit = 1024, secs;
  unsigned long delay = 0, count = 0;
  int c, active = 0, forks = 0, table = 0, first = 1;
  char ch = 'K';

  while((c = shell_getopt(argc, argv, "afsS:")) > 0) {
    switch(c) {
      case 'a': active = 1; break;
      case 'f': forks = 1; break;
      case 's': table = 1; break;
      case 'S':
        if(shell_optarg[1])
          return builtin_errmsg(argv, shell_optarg, "unit must be k, K, m or M");

        if((ch = shell_optarg[0]) == 'k')
          unit = 1000;
        else if(ch == 'K')
          unit = 1024;
        else if(ch == 'm')
          unit = 1000000;
        else if(ch == 'M')
          unit = 1048576;
        else
          return builtin_errmsg(argv, shell_optarg, "unit must be k, K, m or M");

        break;
      default: return builtin_invopt(argv);
    }
  }

  if(forks || table) {
    put_table(unit, ch, table, &sa);
    stralloc_free(&sa);
    return 0;
  }

  if(shell_optind < argc && scan_ulong(argv[shell_optind], &delay) != str_len(argv[shell_optind]))
    return builtin_errmsg(argv, argv[shell_optind], "invalid delay");

  if(shell_optind + 1 < argc && scan_ulong(argv[shell_optind + 1], &count) != str_len(argv[shell_optind + 1]))
    return builtin_errmsg(argv, argv[shell_optind + 1], "invalid count");

  if(!count && !delay)
    count = 1;

  byte_zero(&prev, sizeof(prev));
  put_head(active);

  for(;;) {
    read_sample(&cur, &sa);

    if(first) {
      uint64 up = 0;

      if(slurp("/proc/uptime", &sa) == 0)
        nums(sa.s, &up, 1);

      secs = up;
    } else
      secs = delay;

    put_line(&cur, &prev, secs, active, unit, &sa);
    prev = cur;
    first = 0;

    if(count && !--count)
      break;

    sleep((unsigned)delay);
  }

  stralloc_free(&sa);
  return 0;
}

#endif /* BUILTIN_VMSTAT */
