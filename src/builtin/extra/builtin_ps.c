#include "builtin_config.h"

#if BUILTIN_PS

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/fmt.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/uint64.h"
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

const char help_ps[] = "    Report process status.\n"
                       "\n"
                       "    -e, -A          every process (default: those of this user on this terminal)\n"
                       "    -f              full format: UID, PID, PPID, STIME, TTY, TIME and the command line\n"
                       "    -p list         only these process ids (comma or blank separated)\n"
                       "    -o format       columns: pid ppid pgid sid uid user ni stat tty stime time comm args;\n"
                       "                    name=HEADING sets a heading, name= none (all none: no header line)\n";

/* what the columns show about a process */
struct proc {
  int64 ppid, pgid, sid, tty, nice, ticks, start;
  uid_t uid;
  char state;
  stralloc comm, cmd;
};

typedef enum { C_PID, C_PPID, C_PGID, C_SID, C_UID, C_USER, C_NI, C_STAT, C_TTY, C_STIME, C_TIME, C_COMM, C_ARGS } col_id;

/* keyword, heading, width; widths of text columns are padded on the right */
static const struct col {
  const char* name;
  const char* head;
  unsigned char w;
  unsigned left : 1;
} cols[] = {
    {"pid", "PID", 7, 0},
    {"ppid", "PPID", 7, 0},
    {"pgid", "PGID", 7, 0},
    {"sid", "SID", 7, 0},
    {"uid", "UID", 7, 0},
    {"user", "USER", 8, 1},
    {"ni", "NI", 3, 0},
    {"stat", "STAT", 4, 1},
    {"tty", "TTY", 8, 1},
    {"stime", "STIME", 5, 0},
    {"time", "TIME", 8, 0},
    {"comm", "COMMAND", 0, 1},
    {"args", "COMMAND", 0, 1},
};

enum { NCOLS = sizeof(cols) / sizeof(cols[0]), MAXSEL = 16, MAXPID = 64 };

/* the fields of /proc/PID/stat after the command name; state is 0 */
enum { F_STATE, F_PPID, F_PGRP, F_SESSION, F_TTY, F_UTIME = 11, F_STIME, F_NICE = 16, F_START = 19, F_N = 20 };

/* fill p from /proc/pid; 0 or -1 if the process is gone */
static int
read_proc(struct proc* p, const char* pid, stralloc* sa) {
  char path[32];
  struct stat st;
  int64 f[F_N] = {0};
  const char *s, *e;
  size_t n = str_copy(path, "/proc/");
  int i;

  n += str_copy(path + n, pid);
  str_copy(path + n, "/stat");

  if(slurp(path, sa) == -1)
    return -1;

  s = str_chr(sa->s, '(') + sa->s;

  for(e = sa->s + sa->len; e > s && *e != ')'; e--) {}

  if(!*s || e <= s)
    return -1;

  stralloc_zero(&p->comm);
  stralloc_catb(&p->comm, s + 1, (size_t)(e - s - 1));
  p->state = e[2];

  for(s = e + 2, i = 0; i < F_N && *s; i++) {
    while(*s == ' ')
      s++;

    scan_longlong(s, &f[i]);
    s += str_chr(s, ' ');
  }

  p->ppid = f[F_PPID];
  p->pgid = f[F_PGRP];
  p->sid = f[F_SESSION];
  p->tty = f[F_TTY];
  p->nice = f[F_NICE];
  p->ticks = f[F_UTIME] + f[F_STIME];
  p->start = f[F_START];
  path[n] = '\0';
  p->uid = stat(path, &st) == 0 ? st.st_uid : 0;

  str_copy(path + n, "/cmdline");
  stralloc_zero(&p->cmd);

  if(slurp(path, sa) == 0 && sa->len) {
    for(i = 0; (size_t)i < sa->len; i++) {
      if(!sa->s[i])
        sa->s[i] = ' ';
      else if(sa->s[i] < ' ')
        sa->s[i] = '?';
    }

    for(n = sa->len; n && sa->s[n - 1] == ' '; n--) {}

    stralloc_catb(&p->cmd, sa->s, n);
  }

  return 0;
}

/* the terminal of tty_nr: pts/N, ttyN or ? */
static void
cat_tty(stralloc* t, int64 nr) {
  char b[FMT_ULONG];
  unsigned major = (unsigned)((nr >> 8) & 0xfff), minor = (unsigned)((nr & 0xff) | ((nr >> 12) & 0xfff00));

  if(!nr)
    stralloc_cats(t, "?");
  else {
    stralloc_cats(t, major >= 136 && major <= 143 ? "pts/" : "tty");
    stralloc_catb(t, b, fmt_ulong(b, minor));
  }
}

static void
cat_2(stralloc* t, unsigned long v, size_t w) {
  char b[FMT_ULONG];

  stralloc_catb(t, b, fmt_ulong0(b, v, w));
}

/* HH:MM if started today, else MonDD */
static void
cat_start(stralloc* t, time_t at) {
  static const char mon[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  struct tm tm, now;
  time_t n = time(0);

  localtime_r(&at, &tm);
  localtime_r(&n, &now);

  if(tm.tm_yday == now.tm_yday && tm.tm_year == now.tm_year) {
    cat_2(t, (unsigned long)tm.tm_hour, 2);
    stralloc_catb(t, ":", 1);
    cat_2(t, (unsigned long)tm.tm_min, 2);
  } else {
    stralloc_catb(t, mon + tm.tm_mon * 3, 3);
    cat_2(t, (unsigned long)tm.tm_mday, 2);
  }
}

/* what the run knows besides the process itself */
struct ctx {
  long hz;
  uint64 boot;
  uid_t last;
  stralloc name; /* the user name of `last` */
};

static void
cat_num(stralloc* t, int64 v) {
  char b[FMT_ULONG + 1];

  stralloc_catb(t, b, fmt_longlong(b, v));
}

/* the text of column c for p */
static void
cat_col(stralloc* t, col_id c, const struct proc* p, struct ctx* x) {
  unsigned long s = (unsigned long)(p->ticks / x->hz);

  switch(c) {
    case C_PID: break;
    case C_PPID: cat_num(t, p->ppid); break;
    case C_PGID: cat_num(t, p->pgid); break;
    case C_SID: cat_num(t, p->sid); break;
    case C_UID: cat_num(t, p->uid); break;
    case C_USER:
      if(x->last != p->uid) {
        stralloc_zero(&x->name);

        if(!id_name("/etc/passwd", p->uid, &x->name))
          cat_num(&x->name, p->uid);

        x->last = p->uid;
      }

      stralloc_catb(t, x->name.s, x->name.len);
      break;
    case C_NI: cat_num(t, p->nice); break;
    case C_STAT: stralloc_catb(t, &p->state, 1); break;
    case C_TTY: cat_tty(t, p->tty); break;
    case C_STIME: cat_start(t, (time_t)(x->boot + (uint64)p->start / (uint64)x->hz)); break;
    case C_TIME:
      cat_2(t, s / 3600, 2);
      stralloc_catb(t, ":", 1);
      cat_2(t, s / 60 % 60, 2);
      stralloc_catb(t, ":", 1);
      cat_2(t, s % 60, 2);
      break;
    case C_COMM: stralloc_catb(t, p->comm.s, p->comm.len); break;
    case C_ARGS:
      if(p->cmd.len)
        stralloc_catb(t, p->cmd.s, p->cmd.len);
      else {
        stralloc_catb(t, "[", 1);
        stralloc_catb(t, p->comm.s, p->comm.len);
        stralloc_catb(t, "]", 1);
      }

      break;
  }
}

/* text padded to the column's width, columns separated by a blank */
static void
put_cell(buffer* o, col_id c, const char* s, size_t len, int last) {
  size_t w = cols[c].w;

  if(!cols[c].left && len < w)
    buffer_putnspace(o, (int)(w - len));

  buffer_put(o, s, len);

  if(!last) {
    if(cols[c].left && len < w)
      buffer_putnspace(o, (int)(w - len));

    buffer_putspace(o);
  }
}

/* -o list: "pid,tty=T ..."; fills sel/head, returns the new count or -1 */
static int
parse_columns(char* list, col_id* sel, const char** head, int n) {
  while(*list) {
    char* eq;
    size_t len, k;
    int i;

    if(*list == ',' || *list == ' ') {
      *list++ = '\0';
      continue;
    }

    len = str_chr(list, ',');
    len = len < str_chr(list, ' ') ? len : str_chr(list, ' ');
    eq = list + str_chr(list, '=');
    k = (size_t)(eq - list) < len ? (size_t)(eq - list) : len;

    for(i = 0; i < NCOLS; i++)
      if(str_len(cols[i].name) == k && !str_diffn(cols[i].name, list, k))
        break;

    if(i == NCOLS || n == MAXSEL)
      return -1;

    sel[n] = (col_id)i;
    head[n++] = (size_t)(eq - list) < len ? eq + 1 : cols[i].head;

    if(list[len])
      list[len++] = '\0';

    if((size_t)(eq - list) < len && eq[0] == '=')
      *eq = '\0';

    list += len;
  }

  return n;
}

int
builtin_ps(int argc, char* argv[]) {
  static const char* const def_head[] = {"PID", "TTY", "TIME", "CMD"};
  static const char* const full_head[] = {"UID", "PID", "PPID", "STIME", "TTY", "TIME", "CMD"};
  static const col_id def_sel[] = {C_PID, C_TTY, C_TIME, C_COMM};
  static const col_id full_sel[] = {C_USER, C_PID, C_PPID, C_STIME, C_TTY, C_TIME, C_ARGS};
  struct proc p = {0}, me = {0};
  struct ctx x = {0};
  stralloc sa = {0}, row = {0}, fmt = {0};
  col_id sel[MAXSEL];
  const char* head[MAXSEL];
  unsigned long pids[MAXPID], v;
  struct dirent* e;
  buffer* o = fd_out->w;
  DIR* d;
  int c, i, nsel = 0, npid = 0, all = 0, full = 0, any = 0;
  char b[FMT_ULONG];

  while((c = shell_getopt(argc, argv, "eAfp:o:")) > 0) {
    switch(c) {
      case 'e':
      case 'A': all = 1; break;
      case 'f': full = 1; break;
      case 'p':
        for(i = 0; shell_optarg[i];) {
          while(shell_optarg[i] == ',' || shell_optarg[i] == ' ')
            i++;

          if(!shell_optarg[i])
            break;

          if(!(c = (int)scan_ulong(shell_optarg + i, &v)) || npid == MAXPID)
            return builtin_errmsg(argv, shell_optarg, "invalid process id list");

          pids[npid++] = v;
          i += c;
        }

        break;
      case 'o':
        stralloc_zero(&fmt);
        stralloc_cats(&fmt, shell_optarg);
        stralloc_0(&fmt);

        if((nsel = parse_columns(fmt.s, sel, head, nsel)) == -1)
          return builtin_errmsg(argv, shell_optarg, "unknown format keyword");

        break;
      default: return builtin_invopt(argv);
    }
  }

  if(!nsel) {
    nsel = full ? 7 : 4;

    for(i = 0; i < nsel; i++) {
      sel[i] = full ? full_sel[i] : def_sel[i];
      head[i] = full ? full_head[i] : def_head[i];
    }
  }

  if((x.hz = sysconf(_SC_CLK_TCK)) <= 0)
    x.hz = 100;

  x.last = (uid_t)-1;
  b[fmt_ulong(b, (unsigned long)getpid())] = 0;
  read_proc(&me, b, &sa);

  for(i = 0; i < nsel; i++)
    if(sel[i] == C_STIME && slurp("/proc/stat", &sa) == 0) {
      const char* s;

      for(s = sa.s; *s; s++) {
        if(!str_diffn(s, "btime ", 6)) {
          scan_ulonglong(s + 6, &x.boot);
          break;
        }

        s += str_chr(s, '\n');

        if(!*s)
          break;
      }
    }

  if(!(d = opendir("/proc")))
    return builtin_error(argv, "/proc");

  for(i = 0; i < nsel; i++)
    if(*head[i])
      any = 1;

  if(any) {
    for(i = 0; i < nsel; i++)
      put_cell(o, sel[i], head[i], str_len(head[i]), i == nsel - 1);

    buffer_putnlflush(o);
  }

  while((e = readdir(d))) {
    unsigned long pid;
    size_t n = str_len(e->d_name);

    if(!n || scan_ulong(e->d_name, &pid) != n)
      continue;

    if(npid) {
      for(i = 0; i < npid && pids[i] != pid; i++) {}

      if(i == npid)
        continue;
    }

    if(read_proc(&p, e->d_name, &sa) == -1 || (!npid && !all && (p.uid != geteuid() || p.tty != me.tty)))
      continue;

    for(i = 0; i < nsel; i++) {
      stralloc_zero(&row);

      if(sel[i] == C_PID)
        cat_num(&row, (int64)pid);
      else
        cat_col(&row, sel[i], &p, &x);

      put_cell(o, sel[i], row.s, row.len, i == nsel - 1);
    }

    buffer_putnlflush(o);
  }

  closedir(d);
  stralloc_free(&p.comm);
  stralloc_free(&p.cmd);
  stralloc_free(&me.comm);
  stralloc_free(&me.cmd);
  stralloc_free(&x.name);
  stralloc_free(&sa);
  stralloc_free(&row);
  stralloc_free(&fmt);
  return 0;
}

#endif /* BUILTIN_PS */
