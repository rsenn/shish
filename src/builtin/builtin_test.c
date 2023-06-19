#include "../builtin.h"
#include "../debug.h"
#include "../fd.h"
#include "../exec.h"
#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include <sys/stat.h>
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#ifndef R_OK
#define R_OK 4
#endif /* defined(R_OK) */
#ifndef W_OK
#define W_OK 2
#endif /* defined(W_OK) */
#ifndef X_OK
#define X_OK 1
#endif /* defined(X_OK) */
#ifndef S_IFMT
#define S_IFMT 0xf000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0x6000
#endif /* defined(S_IFBLK) */
#else
#include <unistd.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#ifndef HAVE_LSTAT
#define lstat stat
#endif
#endif
typedef enum { OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE } binary_op;

static int test_boolean(int, char**);

static inline int
contains(const char* str, char ch) {
  return str[str_chr(str, ch)];
}

/* get number of arguments */
static inline int
num_args(int argc) {
  return argc - shell_optind;
}

/* get modification time */
static int64
filetime(const char* arg) {
  struct stat st;
  if(stat(arg, &st) == -1)
    return -1;

  return st.st_mtime;
}

/* get current argument string */
static inline const char*
current(char** v) {
  return v[shell_optind];
}

/* string arg to int64 */
static int64
intarg(const char* arg) {
  int64 num = INT64_MAX;

  scan_longlong(arg, &num);
  return num;
}

/* get current argument, then advance to next */
static const char*
next(char** v) {
  const char* ret = current(v);
  shell_optind++;
  return ret;
}

static binary_op
binary(const char* op) {
  switch(op[1]) {
    case 'l': return op[2] == 'e' ? OP_LE : OP_LT;
    case 'g': return op[2] == 'e' ? OP_GE : OP_GT;
    case 'e': return OP_EQ;
    case 'n': return OP_NE;
  }
  return -1;
}

/* evaluate binary expression
 * ----------------------------------------------------------------------- */
static int
test_binary(int argc, char** argv) {
  const char *left, *right, *op;

  /* if we have 3 arguments it should be something like STRING1 EXPR STRING2 */
  if(num_args(argc) < 3)
    return -1;

  left = next(argv);
  op = next(argv);
  right = next(argv);

  if(op[0] == '-') {

    if(contains("no", op[1])) {
      switch(op[1]) {
        case 'n': return filetime(left) > filetime(right);
        case 'o': return filetime(left) < filetime(right);
      }
    }

    if(str_len(op) == 3 && contains("egln", op[1]) && contains("eqt", op[2])) {
      switch(binary(op)) {
        case OP_LE: return intarg(left) <= intarg(right);
        case OP_LT: return intarg(left) < intarg(right);
        case OP_GE: return intarg(left) >= intarg(right);
        case OP_GT: return intarg(left) > intarg(right);
        case OP_EQ: return intarg(left) == intarg(right);
        case OP_NE: return intarg(left) != intarg(right);
      }
    }

    /* if operator doesn't start with '-' then its surely a string comparision */
  } else {
    int r = str_diff(left, right);

    switch(op[0]) {
      case '=': return r == 0;
      case '!': return r != 0;
      case '<': return op[1] == '=' ? r <= 0 : r < 0;
      case '>': return op[1] == '=' ? r >= 0 : r > 0;
    }
  }
  return -1;
}

/* evaluate unary expression
 * ----------------------------------------------------------------------- */
static int
test_unary(int argc, char** argv) {
  int c;
  const char* arg = current(argv);

  if(arg[0] != '-') {
    if(argc == 2) {
      int balanced = !str_diff(argv[0], "[") && !str_diff(argv[2], "]");
      if(balanced) {
        shell_optind++;
        return !!*argv[1];
      }
    }

    return -1;
  }

  /* check options */
  if((c = shell_getopt(argc, argv, "a:n:z:f:d:b:c:h:L:S:e:s:r:w:x:")) > 0) {
    struct stat st;
    const char* arg = shell_optarg;
    switch(c) {
      /* return true if argument is non-zero */
      case 'n': return arg && *arg;

      /* return true if argument is zero */
      case 'z': return !arg || *arg == '\0';

      /* return true if argument exists */
      case 'a': return stat(arg, &st) == 0;

      /* return true if argument is a regular file */
      case 'f': return stat(arg, &st) == 0 && S_ISREG(st.st_mode);
      /* return true if argument is a directory */
      case 'd': return stat(arg, &st) == 0 && S_ISDIR(st.st_mode);
      /* return true if argument is a character device */
      case 'c':
        return stat(arg, &st) == 0 && S_ISCHR(st.st_mode);

        /* return true if argument is a block device */
      case 'b': return stat(arg, &st) == 0 && (st.st_mode & S_IFMT) == S_IFBLK;
      /* return true if argument is a fifo */
      case 'p': return stat(arg, &st) == 0 && S_ISFIFO(st.st_mode);
      /* return true if argument is a symbolic link */
      case 'h':
#ifdef S_ISLNK
      case 'L': return lstat(arg, &st) == 0 && S_ISLNK(st.st_mode);
#endif

#ifdef S_ISSOCK
      /* return true if argument is a socket */
      case 'S': return stat(arg, &st) == 0 && S_ISSOCK(st.st_mode);
#endif

      /* return true if argument exists */
      case 'e': return stat(arg, &st) == 0;
      /* return true if argument exists and is not empty */
      case 's': return stat(arg, &st) == 0 && st.st_size;

      /* return true if readable */
      case 'r': return access(arg, R_OK) == 0;
      /* return true if writeable */
      case 'w': return access(arg, W_OK) == 0;
      /* return true if executable */
      case 'x': return access(arg, X_OK) == 0;
    }
  }

  return -1;
}

/* evaluate boolean expressions (-a & -o)
 * ----------------------------------------------------------------------- */
static int
test_expr(int argc, char** argv) {
  int index;
  const char* arg;
  int i, parens = 1;
  /*if(shell_optind == argc)
     return -1;
 */
  if(!str_equal((arg = current(argv)), "("))
    return -1;

  index = shell_optind;
  shell_optind++;

  for(i = shell_optind; i < argc; i++) {
    const char* arg = argv[/*shell_optind +*/ i];
    if(str_len(arg) == 1 && contains("()", arg[0]))
      parens += arg[0] == '(' ? 1 : -1;
    if(parens == 0)
      break;
  }

  if(/*shell_optind + */ i < argc) {
    int result;
    if((result = test_boolean(/*shell_optind +*/ i, argv)) != -1) {
      shell_optind++ /* += i*/;
      return result;
    }
  }

  shell_optind = index;
  return -1;
}

/* evaluate condition (! negates)
 * ----------------------------------------------------------------------- */
static int
test_cond(int argc, char** argv) {
  int result = -1, neg = 0;

  /* every condition can be negated by a leading ! */
  while(shell_optind < argc && str_equal(current(argv), "!")) {
    neg = !neg;
    shell_optind++;
  }

  if(shell_optind == argc)
    return -1;

  if(str_equal(current(argv), "("))
    result = test_expr(argc, argv);

  if(result == -1)
    result = test_binary(argc, argv);

  if(result == -1)
    result = test_unary(argc, argv);

  // if(result == -1)
  result = neg ^ result;

  return result;
}

/* evaluate boolean expressions (-a & -o)
 * ----------------------------------------------------------------------- */
static int
test_boolean(int argc, char* argv[]) {
  int result = 1, prev;
  enum { TEST_AND = 1, TEST_OR = 2 } and_or = 0;
  const char* arg;

  while(num_args(argc) > 0 && (result = test_cond(argc, argv)) != -1) {

    if(and_or) {
      switch(and_or) {
        case TEST_AND: result = prev && result; break;
        case TEST_OR: result = prev || result; break;
      }
      and_or = 0;
    }

    if(num_args(argc) > 1 && (arg = next(argv))[0] == '-' && contains("ao", arg[1])) {
      switch(arg[1]) {
        case 'a': and_or = TEST_AND; break;
        case 'o': and_or = TEST_OR; break;
      }
    }
    if(and_or == 0)
      break;
    prev = result;
  }
  return result;
}

/* test for expression
 * ----------------------------------------------------------------------- */
int
builtin_test(int argc, char* argv[]) {
  int result, ret = 1;
  int brackets = 0;

  if(argv[0][0] == '[') {
    brackets = 1;
    argc--;

    if(argv[argc][0] != ']') {
      builtin_errmsg(argv, "missing ]'", NULL);
      return EXIT_ERROR;
    }
  }

  /* TODO:*/
  //  (void)brackets;
  result = test_boolean(argc, argv);

  if(result == -1) {
    builtin_errmsg(argv, "invalid expression", argv[shell_optind]);
    ret = 2;
  } else if(num_args(argc) > 0) {
    builtin_errmsg(argv, "extra arguments", argv[shell_optind]);
    ret = 3;
  }

  if(ret <= 1)
    ret = !result;

#if DEBUG_OUTPUT_
  buffer_puts(fd_err->w, "test return value: ");
  buffer_putulong(fd_err->w, ret);
  buffer_putnlflush(fd_err->w);
#endif
  return ret;
}
