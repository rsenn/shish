#include "builtin_config.h"

#if BUILTIN_FIND

#include "../../builtin.h"
#include "../../fdtable.h"
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

struct ancestor {
  dev_t dev;
  ino_t ino;
  struct ancestor* next;
};

/* >0 while the right side of a && or || that is not needed is parsed: no actions run */
static int find_dry;
static int find_bad;
static long find_maxdepth = -1, find_mindepth;

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
    if(str_equal(val, "s"))
      return S_ISSOCK(st->st_mode);
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
    char* base = path_basename(path);
    if(ignore_case) {
      stralloc lp, lb;
      size_t k;
      int m;

      stralloc_init(&lp);
      stralloc_init(&lb);
      stralloc_copys(&lp, pat);
      stralloc_copys(&lb, base);

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

    return path_fnmatch(pat, str_len(pat), base, str_len(base), 0) == 0;
  }

  if(str_equal(arg, "-path")) {
    char* pat;

    (*i)++;
    if(*i >= end)
      return 0;
    pat = argv[(*i)++];
    return path_fnmatch(pat, str_len(pat), path, str_len(path), 0) == 0;
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

static int
find_recursive(char* argv[], int expr_start, int expr_end, stralloc* path, int deref_links, struct ancestor* anc, long depth) {
  struct stat st;
  int has_action = 0;
  int i;
  struct ancestor current_anc;

  if(deref_links ? stat(path->s, &st) == -1 : lstat(path->s, &st) == -1) {
    builtin_error(argv, path->s);
    return 1;
  }

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

  i = expr_start;
  if(depth >= find_mindepth && eval_expr(argv, &i, expr_end, path->s, &st, &has_action)) {
    if(!has_action) {
      buffer_putm_internal(fd_out->w, path->s, 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  if(find_bad)
    return 1;

  if(S_ISDIR(st.st_mode) && (find_maxdepth < 0 || depth < find_maxdepth)) {
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

      path->len = dirlen;
      if(dirlen > 0 && path->s[dirlen - 1] != PATHSEP_C)
        stralloc_catc(path, PATHSEP_C);
      stralloc_cats(path, de->d_name);
      stralloc_nul(path);

      find_recursive(argv, expr_start, expr_end, path, 0, &current_anc, depth + 1);
    }

    closedir(dp);
    path->len = dirlen;
    stralloc_nul(path);
  }

  return 0;
}

const char help_find[] = "    Search for files in a directory hierarchy[cite: 23].\n"
                         "    -H              dereference command-line symbolic links\n"
                         "    -L              dereference all symbolic links\n";

int
builtin_find(int argc, char* argv[]) {
  int c;
  int deref_links = 0;
  int expr_start = 1;
  stralloc path;
  int i, failed = 0;

  while((c = shell_getopt(argc, argv, "HL")) > 0) {
    switch(c) {
      case 'H': deref_links = 0; break;
      case 'L': deref_links = 1; break;
      default: break;
    }
  }

  expr_start = shell_optind;

  while(argv[expr_start] && argv[expr_start][0] != '-' && !str_equal(argv[expr_start], "(") &&
        !str_equal(argv[expr_start], "!")) {
    expr_start++;
  }

  find_dry = find_bad = 0;
  find_maxdepth = -1;
  find_mindepth = 0;

  for(i = expr_start; i + 1 < argc; i++) {
    unsigned long v;

    if(str_equal(argv[i], "-maxdepth") && scan_ulong(argv[i + 1], &v) > 0)
      find_maxdepth = (long)v;
    else if(str_equal(argv[i], "-mindepth") && scan_ulong(argv[i + 1], &v) > 0)
      find_mindepth = (long)v;
  }

  stralloc_init(&path);

  if(expr_start == shell_optind) {
    stralloc_copys(&path, ".");
    stralloc_nul(&path);
    if(find_recursive(argv, shell_optind, argc, &path, deref_links, NULL, 0))
      failed = 1;
  } else {
    for(i = shell_optind; i < expr_start; i++) {
      stralloc_copys(&path, argv[i]);
      stralloc_nul(&path);
      if(find_recursive(argv, expr_start, argc, &path, deref_links, NULL, 0))
        failed = 1;
    }
  }

  stralloc_free(&path);
  return failed;
}
#endif /* BUILTIN_FIND */
