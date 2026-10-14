#include "builtin_config.h"

#if BUILTIN_FIND

#include "../../builtin.h"
#include "../../exec.h"
#include "../../fdtable.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/path.h"
#include "../../../lib/unix.h"
#include "../../../lib/scan.h"
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#if USE_SYSTEM_REGEX
#include <regex.h>
typedef regex_t find_re;
#else
#include "../../../text/dfa.h"
typedef struct dfa find_re;
#endif

struct ancestor {
  dev_t dev;
  ino_t ino;
  struct ancestor* next;
};

/* >0 while the right side of a && or || that is not needed is parsed: no actions run */
static int find_dry;
static int find_bad;
static int find_failed;  /* an -exec, -delete or traversal error: exit status 1 */
static int find_prune;   /* -prune was evaluated for the current directory */
static int find_quit;    /* -quit was evaluated: stop the whole walk */
static int find_depth; /* -depth or -delete: a directory's contents before the directory itself */
static int find_xdev;  /* -xdev, -mount: stay on the starting point's device */
static long find_maxdepth = -1, find_mindepth;
static dev_t find_start_dev;
static time_t find_now;
static char** find_argv;
static int find_ere; /* -regextype posix-extended: -regex patterns are EREs, else BREs */

/* -regex/-iregex, compiled when first used; the key is the argv index of the primary */
#define FIND_RES 8
static struct findre {
  int key;
  find_re re;
} find_res[FIND_RES];
static int find_nres;

/* "-exec ... {} +": paths gathered per -exec, run when the walk ends or the batch is large */
#define FIND_BATCHES 8
#define FIND_BATCH_MAX 65536
static struct execbatch {
  int key;                /* argv index of the -exec token */
  int first, last;        /* command words are argv[first..last), "{}" and "+" excluded */
  size_t count;           /* paths held */
  stralloc paths;         /* NUL-separated */
} find_batches[FIND_BATCHES];
static int find_nbatches;

/* "N", "+N" or "-N" -> cmp 0, 1 or -1 and *n; returns 0 if s is not a number */
static int
find_num(const char* s, int* cmp, unsigned long* n) {
  *cmp = *s == '+' ? 1 : *s == '-' ? -1 : 0;
  s += *cmp != 0;
  return *s && scan_ulong(s, n) == str_len(s);
}

static int
find_compare(int cmp, unsigned long long v, unsigned long long n) {
  return cmp > 0 ? v > n : cmp < 0 ? v < n : v == n;
}

/* runs argv[first..last) through the shell's dispatch with "{}" replaced by each of paths
 * (plain run: one path; batch: all of them appended). builtins, functions and programs all work. */
static int
find_run(char* argv[], int first, int last, const char* const* paths, size_t npaths, int replace) {
  int n = last - first, ac = 0, ret;
  size_t k, total = (size_t)n + (replace ? 0 : npaths);
  char** v = alloc((total + 1) * sizeof(char*));
  stralloc* subs = alloc((total + 1) * sizeof(stralloc));
  struct command cmd;
  int i;

  if(!v || !subs) {
    builtin_error(argv, "-exec");
    return 1;
  }

  for(k = 0; k < total; k++)
    stralloc_init(&subs[k]);

  for(i = first; i < last; i++) {
    const char* a = argv[i];
    size_t pos;

    if(replace && str_chr(a, '{') != str_len(a)) {
      while(*a) {
        pos = str_chr(a, '{');
        stralloc_catb(&subs[ac], a, pos);
        a += pos;

        if(a[0] == '{' && a[1] == '}') {
          stralloc_cats(&subs[ac], paths[0]);
          a += 2;
        } else if(*a) {
          stralloc_catc(&subs[ac], *a++);
        }
      }

      stralloc_nul(&subs[ac]);
      v[ac] = subs[ac].s;
    } else {
      v[ac] = (char*)argv[i];
    }

    ac++;
  }

  if(!replace)
    for(k = 0; k < npaths; k++)
      v[ac++] = (char*)paths[k];

  v[ac] = NULL;
  buffer_flush(fd_out->w);
  cmd = exec_hash(v[0], 0);

  if(cmd.ptr) {
    ret = exec_command(&cmd, ac, v, 0);
  } else {
    builtin_errmsg(argv, v[0], "command not found");
    ret = EXIT_NOTFOUND;
  }

  for(k = 0; k < total; k++)
    stralloc_free(&subs[k]);

  alloc_free(subs);
  alloc_free(v);
  return ret;
}

static void
find_flush(char* argv[], struct execbatch* b) {
  const char** paths;
  size_t i, off = 0;

  if(!b->count)
    return;

  if((paths = (const char**)alloc(b->count * sizeof(char*)))) {
    for(i = 0; i < b->count; i++) {
      paths[i] = b->paths.s + off;
      off += str_len(b->paths.s + off) + 1;
    }

    if(find_run(argv, b->first, b->last, paths, b->count, 0) != 0)
      find_failed = 1;

    alloc_free(paths);
  }

  b->count = 0;
  b->paths.len = 0;
}

static void
find_flush_all(char* argv[]) {
  int i;

  for(i = 0; i < find_nbatches; i++) {
    find_flush(argv, &find_batches[i]);
    stralloc_free(&find_batches[i].paths);
  }

  find_nbatches = 0;
}

/* the compiled form of argv[key + 1], compiled on first use; NULL after an error message */
static find_re*
find_regex(char* argv[], int key, int icase) {
  int i;
  struct findre* r;
  const char* pat = argv[key + 1];

  for(i = 0; i < find_nres; i++)
    if(find_res[i].key == key)
      return &find_res[i].re;

  if(find_nres == FIND_RES) {
    builtin_errmsg(argv, "too many", "-regex");
    return NULL;
  }

  r = &find_res[find_nres];
#if USE_SYSTEM_REGEX
  if(regcomp(&r->re, pat, (find_ere ? REG_EXTENDED : 0) | (icase ? REG_ICASE : 0)) != 0) {
#else
  if(dfa_compile(&r->re, pat, str_len(pat), (find_ere ? DFA_ERE : 0) | (icase ? DFA_ICASE : 0)) != DFA_OK) {
#endif
    builtin_errmsg(argv, (char*)pat, "invalid regular expression");
    return NULL;
  }

  r->key = key;
  find_nres++;
  return &r->re;
}

/* does the regular expression match the whole path? (find -regex is anchored at both ends) */
static int
find_regex_match(find_re* re, const char* path) {
#if USE_SYSTEM_REGEX
  regmatch_t m;

  return regexec(re, path, 1, &m, 0) == 0 && m.rm_so == 0 && (size_t)m.rm_eo == str_len(path);
#else
  return dfa_prefix(re, path, str_len(path)) == (long)str_len(path);
#endif
}

static void
find_regex_free(void) {
  while(find_nres > 0) {
#if USE_SYSTEM_REGEX
    regfree(&find_res[--find_nres].re);
#else
    dfa_free(&find_res[--find_nres].re);
#endif
  }
}

static int
find_empty_dir(const char* path) {
  DIR* dp = opendir(path);
  struct dirent* de;
  int empty = 1;

  if(!dp)
    return 0;

  while((de = readdir(dp)))
    if(!str_equal(de->d_name, ".") && !str_equal(de->d_name, "..")) {
      empty = 0;
      break;
    }

  closedir(dp);
  return empty;
}

static int eval_expr(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action);

static int
eval_primary(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action) {
  if(*i >= end)
    return 1;

  char* arg = argv[*i];

  if(str_equal(arg, "(")) {
    (*i)++;
    int res = eval_expr(argv, i, end, path, st, has_action);
    if(*i < end && str_equal(argv[*i], ")"))
      (*i)++;
    return res;
  }

  if(str_equal(arg, "!")) {
    (*i)++;
    return !eval_primary(argv, i, end, path, st, has_action);
  }

  if(str_equal(arg, "-type")) {
    (*i)++;
    if(*i >= end)
      return 0;
    char* val = argv[(*i)++];
    if(str_equal(val, "f"))
      return S_ISREG(st->st_mode);
    if(str_equal(val, "d"))
      return S_ISDIR(st->st_mode);
    if(str_equal(val, "l"))
      return S_ISLNK(st->st_mode);
    if(str_equal(val, "b"))
      return S_ISBLK(st->st_mode);
    if(str_equal(val, "c"))
      return S_ISCHR(st->st_mode);
    if(str_equal(val, "p"))
      return S_ISFIFO(st->st_mode);
#ifdef S_ISSOCK
    if(str_equal(val, "s"))
      return S_ISSOCK(st->st_mode);
#endif
    return 0;
  }

  /* -newer file: modified more recently than file (whole seconds) */
  if(str_equal(arg, "-newer")) {
    struct stat ref;

    (*i)++;
    if(*i >= end)
      return 0;
    if(stat(argv[(*i)++], &ref) == -1) {
      builtin_error(argv, argv[*i - 1]);
      find_bad = 1;
      *i = end;
      return 0;
    }
    return st->st_mtime > ref.st_mtime;
  }

  /* -links n: link count is n, more than n (+n) or less than n (-n) */
  if(str_equal(arg, "-links")) {
    char* s;
    unsigned long n;

    (*i)++;
    if(*i >= end)
      return 0;
    s = argv[(*i)++];
    if(scan_ulong(s + (*s == '+' || *s == '-'), &n) == 0 || s[str_len(s) - 1] < '0' || s[str_len(s) - 1] > '9') {
      builtin_errmsg(argv, "bad number", s);
      find_bad = 1;
      *i = end;
      return 0;
    }
    return *s == '+' ? (unsigned long)st->st_nlink > n : *s == '-' ? (unsigned long)st->st_nlink < n : (unsigned long)st->st_nlink == n;
  }

  if(str_equal(arg, "-name") || str_equal(arg, "-iname")) {
    int ignore_case = str_equal(arg, "-iname");
    (*i)++;
    if(*i >= end)
      return 0;
    char* pat = argv[(*i)++];
    size_t blen;
    const char* base = path_basename(path, str_len(path), &blen);
    if(ignore_case) {
      stralloc lp, lb;
      size_t k;
      int m;

      stralloc_init(&lp);
      stralloc_init(&lb);
      stralloc_copys(&lp, pat);
      stralloc_copyb(&lb, base, blen);

      for(k = 0; k < lp.len; k++)
        if(lp.s[k] >= 'A' && lp.s[k] <= 'Z')
          lp.s[k] += 'a' - 'A';

      for(k = 0; k < lb.len; k++)
        if(lb.s[k] >= 'A' && lb.s[k] <= 'Z')
          lb.s[k] += 'a' - 'A';

      m = path_fnmatch(lp.s, lp.len, lb.s, lb.len, 0) == 0;
      stralloc_free(&lp);
      stralloc_free(&lb);
      return m;
    }

    return path_fnmatch(pat, str_len(pat), base, blen, 0) == 0;
  }

  /* -regex/-iregex PATTERN: PATTERN matches the whole path; -regextype was read up front */
  if(str_equal(arg, "-regex") || str_equal(arg, "-iregex")) {
    find_re* re;
    int key = *i;

    (*i)++;
    if(*i >= end)
      return 0;
    (*i)++;
    re = find_regex(argv, key, arg[1] == 'i');

    if(!re) {
      find_bad = 1;
      *i = end;
      return 0;
    }

    return find_regex_match(re, path);
  }

  if(str_equal(arg, "-regextype")) {
    *i += 2;
    return 1;
  }

  if(str_equal(arg, "-path")) {
    char* pat;

    (*i)++;
    if(*i >= end)
      return 0;
    pat = argv[(*i)++];
    return path_fnmatch(pat, str_len(pat), path, str_len(path), 0) == 0;
  }

  if(str_equal(arg, "-not")) {
    (*i)++;
    return !eval_primary(argv, i, end, path, st, has_action);
  }

  /* -exec cmd {} ;   -exec cmd {} +   ('{}' anywhere in a word with ';', last word with '+') */
  if(str_equal(arg, "-exec")) {
    int first = *i + 1, j, plus = 0;

    for(j = first; j < end; j++) {
      if(str_equal(argv[j], ";"))
        break;

      if(str_equal(argv[j], "+") && j > first && str_equal(argv[j - 1], "{}")) {
        plus = 1;
        break;
      }
    }

    if(j >= end || j == first + plus) {
      builtin_errmsg(argv, "missing argument to", "-exec");
      find_bad = 1;
      *i = end;
      return 0;
    }

    *i = j + 1;
    *has_action = 1;

    if(find_dry)
      return 1;

    if(plus) {
      struct execbatch* b = 0;
      int k;

      for(k = 0; k < find_nbatches; k++)
        if(find_batches[k].key == first - 1)
          b = &find_batches[k];

      if(!b && find_nbatches < FIND_BATCHES) {
        b = &find_batches[find_nbatches++];
        b->key = first - 1;
        b->first = first;
        b->last = j - 1;
        b->count = 0;
        stralloc_init(&b->paths);
      }

      if(!b) {
        builtin_errmsg(argv, "too many", "-exec ... +");
        find_bad = 1;
        return 0;
      }

      stralloc_catb(&b->paths, path, str_len(path) + 1);
      b->count++;

      if(b->paths.len > FIND_BATCH_MAX)
        find_flush(argv, b);

      return 1;
    }

    return find_run(argv, first, j, &path, 1, 1) == 0;
  }

  if(str_equal(arg, "-print0")) {
    (*i)++;
    *has_action = 1;

    if(!find_dry) {
      buffer_put(fd_out->w, path, str_len(path) + 1);
      buffer_flush(fd_out->w);
    }

    return 1;
  }

  if(str_equal(arg, "-delete")) {
    (*i)++;
    *has_action = 1;

    if(find_dry)
      return 1;

    if((S_ISDIR(st->st_mode) ? rmdir(path) : unlink(path)) == -1) {
      builtin_error(argv, (char*)path);
      find_failed = 1;
      return 0;
    }

    return 1;
  }

  if(str_equal(arg, "-prune")) {
    (*i)++;

    if(!find_dry)
      find_prune = 1;

    return 1;
  }

  if(str_equal(arg, "-quit")) {
    (*i)++;

    if(!find_dry)
      find_quit = 1;

    return 1;
  }

  /* -depth, -xdev, -mount were read up front; they are always true here */
  if(str_equal(arg, "-depth") || str_equal(arg, "-xdev") || str_equal(arg, "-mount")) {
    (*i)++;
    return 1;
  }

  if(str_equal(arg, "-empty")) {
    (*i)++;
    return S_ISREG(st->st_mode) ? st->st_size == 0 : S_ISDIR(st->st_mode) ? find_empty_dir(path) : 0;
  }

  if(str_equal(arg, "-readable") || str_equal(arg, "-writable") || str_equal(arg, "-executable")) {
    (*i)++;
    return access(path, arg[1] == 'r' ? R_OK : arg[1] == 'w' ? W_OK : X_OK) == 0;
  }

  /* -size [+-]N[bcwkMG]: size in units of N, rounded up (default 512-byte blocks) */
  if(str_equal(arg, "-size")) {
    char num[32];
    char* s;
    int cmp;
    unsigned long n;
    unsigned long long unit = 512;
    size_t l;

    (*i)++;
    if(*i >= end)
      return 0;
    s = argv[(*i)++];
    l = str_len(s);

    if(l > 0 && str_chr("bcwkMG", s[l - 1]) < 6) {
      unit = s[l - 1] == 'c' ? 1 : s[l - 1] == 'w' ? 2 : s[l - 1] == 'k' ? 1024 : s[l - 1] == 'M' ? 1048576ULL : s[l - 1] == 'G' ? 1073741824ULL : 512;
      l--;
    }

    if(l >= sizeof(num)) {
      l = sizeof(num) - 1;
    }

    byte_copy(num, l, s);
    num[l] = '\0';

    if(!find_num(num, &cmp, &n)) {
      builtin_errmsg(argv, "bad number", s);
      find_bad = 1;
      *i = end;
      return 0;
    }

    return find_compare(cmp, ((unsigned long long)st->st_size + unit - 1) / unit, n);
  }

  /* -mtime/-atime/-ctime N (days, rounded down), -mmin/-amin/-cmin N (minutes, rounded up) */
  if(arg[0] == '-' && (arg[1] == 'm' || arg[1] == 'a' || arg[1] == 'c') && (str_equal(arg + 2, "time") || str_equal(arg + 2, "min"))) {
    int cmp, mins = arg[2] == 'm';
    unsigned long n;
    time_t t = arg[1] == 'm' ? st->st_mtime : arg[1] == 'a' ? st->st_atime : st->st_ctime;
    long long age = (long long)find_now - (long long)t;

    (*i)++;
    if(*i >= end)
      return 0;
    if(!find_num(argv[*i], &cmp, &n)) {
      builtin_errmsg(argv, "bad number", argv[*i]);
      find_bad = 1;
      *i = end;
      return 0;
    }
    (*i)++;

    if(age < 0)
      age = 0;

    return find_compare(cmp, mins ? (unsigned long long)(age + 59) / 60 : (unsigned long long)age / 86400, n);
  }

  /* -perm MODE (exact), -perm -MODE (all bits set), -perm /MODE (any bit set); MODE is octal */
  if(str_equal(arg, "-perm")) {
    char* s;
    unsigned long m;
    int kind;

    (*i)++;
    if(*i >= end)
      return 0;
    s = argv[(*i)++];
    kind = *s == '-' ? 1 : *s == '/' ? 2 : 0;
    s += kind != 0;

    if(!*s || scan_8long(s, &m) != str_len(s)) {
      builtin_errmsg(argv, "bad mode", s);
      find_bad = 1;
      *i = end;
      return 0;
    }

    return kind == 1 ? (st->st_mode & m) == m : kind == 2 ? (st->st_mode & m) != 0 || m == 0 : (st->st_mode & 07777) == m;
  }

  /* -maxdepth/-mindepth were read up front; they are always true here */
  if(str_equal(arg, "-maxdepth") || str_equal(arg, "-mindepth")) {
    *i += 2;
    return 1;
  }

  if(str_equal(arg, "-true")) {
    (*i)++;
    return 1;
  }

  if(str_equal(arg, "-false")) {
    (*i)++;
    return 0;
  }

  if(str_equal(arg, "-print")) {
    (*i)++;
    *has_action = 1;

    if(!find_dry) {
      buffer_putm_internal(fd_out->w, path, 0);
      buffer_putnlflush(fd_out->w);
    }

    return 1;
  }

  /* anything else is not understood: stop here instead of looping on it */
  builtin_errmsg(argv, "unknown predicate", arg);
  find_bad = 1;
  *i = end;
  return 0;
}

static int
eval_term(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action) {
  int lhs = eval_primary(argv, i, end, path, st, has_action);
  while(*i < end) {
    char* op = argv[*i];
    if(str_equal(op, "-o") || str_equal(op, "-or") || str_equal(op, ")"))
      break;
    if(str_equal(op, "-a") || str_equal(op, "-and")) {
      (*i)++;
    }
    int rhs;

    find_dry += !lhs;
    rhs = eval_primary(argv, i, end, path, st, has_action);
    find_dry -= !lhs;
    lhs = lhs && rhs;
  }
  return lhs;
}

static int
eval_expr(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action) {
  int lhs = eval_term(argv, i, end, path, st, has_action);
  while(*i < end) {
    char* op = argv[*i];
    if(str_equal(op, "-o") || str_equal(op, "-or")) {
      (*i)++;
      int rhs;

      find_dry += !!lhs;
      rhs = eval_term(argv, i, end, path, st, has_action);
      find_dry -= !!lhs;
      lhs = lhs || rhs;
    } else {
      break;
    }
  }
  return lhs;
}

/* evaluates the expression for one file; prints the path when the expression has no action */
static void
find_visit(char* argv[], int expr_start, int expr_end, stralloc* path, struct stat* st, long depth) {
  int i = expr_start, has_action = 0;

  if(depth >= find_mindepth && eval_expr(argv, &i, expr_end, path->s, st, &has_action) && !has_action) {
    buffer_putm_internal(fd_out->w, path->s, 0);
    buffer_putnlflush(fd_out->w);
  }
}

static int
find_recursive(char* argv[], int expr_start, int expr_end, stralloc* path, int deref_links, struct ancestor* anc, long depth) {
  struct stat st;
  struct ancestor current_anc;
  int ret = 0;

  if(deref_links ? stat(path->s, &st) == -1 : lstat(path->s, &st) == -1) {
    builtin_error(argv, path->s);
    return 1;
  }

  if(depth == 0)
    find_start_dev = st.st_dev;

  current_anc.dev = st.st_dev;
  current_anc.ino = st.st_ino;
  current_anc.next = anc;

  if(S_ISDIR(st.st_mode)) {
    struct ancestor* p_anc = anc;
    while(p_anc) {
      if(p_anc->dev == st.st_dev && p_anc->ino == st.st_ino) {
        buffer_putm_internal(fd_err->w, argv[0], ": file system loop detected '", path->s, "'", 0);
        buffer_putnlflush(fd_err->w);
        return 1;
      }
      p_anc = p_anc->next;
    }
  }

  find_prune = 0;

  if(!find_depth)
    find_visit(argv, expr_start, expr_end, path, &st, depth);

  if(find_bad || find_quit)
    return find_bad;

  if(S_ISDIR(st.st_mode) && !(find_prune && !find_depth) && (find_maxdepth < 0 || depth < find_maxdepth) &&
     !(find_xdev && st.st_dev != find_start_dev)) {
    DIR* dp;
    struct dirent* de;
    size_t dirlen = path->len;

    if(!(dp = opendir(path->s))) {
      builtin_error(argv, path->s);
      return 1;
    }

    while((de = readdir(dp))) {
      if(str_equal(de->d_name, ".") || str_equal(de->d_name, ".."))
        continue;

      path_cat_sa(path, path->s, dirlen, de->d_name, str_len(de->d_name));

      if(find_recursive(argv, expr_start, expr_end, path, 0, &current_anc, depth + 1))
        ret = 1;

      if(find_bad || find_quit)
        break;
    }

    closedir(dp);
    path->len = dirlen;
    stralloc_nul(path);
  }

  if(find_depth && !find_bad && !find_quit)
    find_visit(argv, expr_start, expr_end, path, &st, depth);

  return find_bad ? 1 : ret;
}

const char help_find[] = "    Search for files in a directory hierarchy.\n"
                         "    find [-H|-L|-P] [PATH...] [EXPRESSION]   (PATH defaults to \".\")\n"
                         "\n"
                         "    -H              dereference command-line symbolic links\n"
                         "    -L              dereference all symbolic links\n"
                         "    -P              never dereference symbolic links (default)\n"
                         "\n"
                         "    Tests:   -name -iname -path -regex -iregex PATTERN, -type [fdlbcps], -newer FILE, -links N,\n"
                         "             -size [+-]N[bcwkMG], -mtime -atime -ctime N, -mmin -amin -cmin N,\n"
                         "             -perm [-/]MODE, -empty, -readable, -writable, -executable\n"
                         "    Options: -maxdepth N, -mindepth N, -depth, -xdev (-mount),\n"
                         "             -regextype posix-basic|posix-extended (default posix-basic)\n"
                         "    Actions: -print, -print0, -delete, -prune, -quit,\n"
                         "             -exec COMMAND ... ;   run COMMAND for each file, '{}' is the path\n"
                         "             -exec COMMAND ... {} +   run COMMAND once with many paths\n"
                         "    Operators: ( ) ! -not -a -and -o -or\n"
                         "\n"
                         "    COMMAND is run by the shell itself, so builtins and functions work,\n"
                         "    e.g. find -type f -name '*.sh' -exec source {} \\;\n";

int
builtin_find(int argc, char* argv[]) {
  int deref_links = 0;
  int first = 1, expr_start;
  stralloc path;
  int i, failed = 0;

  /* -H/-L/-P only when they are whole arguments: "-type" is an expression, not option letters */
  for(; argv[first]; first++) {
    if(str_equal(argv[first], "-H") || str_equal(argv[first], "-P"))
      deref_links = 0;
    else if(str_equal(argv[first], "-L"))
      deref_links = 1;
    else
      break;
  }

  expr_start = first;

  while(argv[expr_start] && argv[expr_start][0] != '-' && !str_equal(argv[expr_start], "(") &&
        !str_equal(argv[expr_start], "!")) {
    expr_start++;
  }

  find_dry = find_bad = find_failed = find_prune = find_quit = 0;
  find_depth = find_xdev = find_ere = 0;
  find_maxdepth = -1;
  find_mindepth = 0;
  find_nbatches = 0;
  find_nres = 0;
  find_argv = argv;
  find_now = time(NULL);

  for(i = expr_start; i < argc; i++) {
    unsigned long v;

    if(i + 1 < argc && str_equal(argv[i], "-maxdepth") && scan_ulong(argv[i + 1], &v) > 0)
      find_maxdepth = (long)v;
    else if(i + 1 < argc && str_equal(argv[i], "-mindepth") && scan_ulong(argv[i + 1], &v) > 0)
      find_mindepth = (long)v;
    else if(str_equal(argv[i], "-depth") || str_equal(argv[i], "-delete"))
      find_depth = 1;
    else if(str_equal(argv[i], "-xdev") || str_equal(argv[i], "-mount"))
      find_xdev = 1;
    else if(i + 1 < argc && str_equal(argv[i], "-regextype"))
      find_ere = str_equal(argv[i + 1], "posix-extended") || str_equal(argv[i + 1], "egrep") || str_equal(argv[i + 1], "extended");
  }

  stralloc_init(&path);

  if(expr_start == first) {
    stralloc_copys(&path, ".");
    stralloc_nul(&path);
    if(find_recursive(argv, first, argc, &path, deref_links, NULL, 0))
      failed = 1;
  } else {
    for(i = first; i < expr_start && !find_quit; i++) {
      stralloc_copys(&path, argv[i]);
      stralloc_nul(&path);
      if(find_recursive(argv, expr_start, argc, &path, deref_links, NULL, 0))
        failed = 1;
    }
  }

  find_flush_all(argv);
  find_regex_free();
  stralloc_free(&path);
  return failed || find_failed;
}
#endif /* BUILTIN_FIND */
