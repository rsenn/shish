#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/path.h"
#include "../../../lib/unix.h"
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

static int eval_expr(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action);

static int
eval_primary(char* argv[], int* i, int end, const char* path, struct stat* st, int* has_action) {
  if(*i >= end) return 1;

  char* arg = argv[*i];

  if(str_equal(arg, "(")) {
    (*i)++;
    int res = eval_expr(argv, i, end, path, st, has_action);
    if(*i < end && str_equal(argv[*i], ")")) (*i)++;
    return res;
  }

  if(str_equal(arg, "!")) {
    (*i)++;
    return !eval_primary(argv, i, end, path, st, has_action);
  }

  if(str_equal(arg, "-type")) {
    (*i)++;
    if(*i >= end) return 0;
    char* val = argv[(*i)++];
    if(str_equal(val, "f")) return S_ISREG(st->st_mode);
    if(str_equal(val, "d")) return S_ISDIR(st->st_mode);
    if(str_equal(val, "l")) return S_ISLNK(st->st_mode);
    return 0;
  }

  if(str_equal(arg, "-name") || str_equal(arg, "-iname")) {
    int ignore_case = str_equal(arg, "-iname");
    (*i)++;
    if(*i >= end) return 0;
    char* pat = argv[(*i)++];
    char* base = path_basename(path);
    (void)ignore_case;
    return path_fnmatch(pat, str_len(pat), base, str_len(base), 0) == 0;
  }

  if(str_equal(arg, "-print")) {
    (*i)++;
    *has_action = 1;
    buffer_putm_internal(fd_out->w, path, 0);
    buffer_putnlflush(fd_out->w);
    return 1;
  }

  return 1;
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
    int rhs = eval_primary(argv, i, end, path, st, has_action);
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
      int rhs = eval_term(argv, i, end, path, st, has_action);
      lhs = lhs || rhs;
    } else {
      break;
    }
  }
  return lhs;
}

static int
find_recursive(char* argv[], int expr_start, int expr_end, stralloc* path, int deref_links, struct ancestor* anc) {
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
  if(eval_expr(argv, &i, expr_end, path->s, &st, &has_action)) {
    if(!has_action) {
      buffer_putm_internal(fd_out->w, path->s, 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  if(S_ISDIR(st.st_mode)) {
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

      find_recursive(argv, expr_start, expr_end, path, 0, &current_anc);
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

  while(argv[expr_start] && argv[expr_start][0] != '-' && 
        !str_equal(argv[expr_start], "(") && !str_equal(argv[expr_start], "!")) {
    expr_start++;
  }

  stralloc_init(&path);

  if(expr_start == shell_optind) {
    stralloc_copys(&path, ".");
    stralloc_nul(&path);
    if(find_recursive(argv, shell_optind, argc, &path, deref_links, NULL))
      failed = 1;
  } else {
    for(i = shell_optind; i < expr_start; i++) {
      stralloc_copys(&path, argv[i]);
      stralloc_nul(&path);
      if(find_recursive(argv, expr_start, argc, &path, deref_links, NULL))
        failed = 1;
    }
  }

  stralloc_free(&path);
  return failed;
}
