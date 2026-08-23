#include "../builtin.h"
#include "../debug.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../exec.h"
#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/unix.h"
#include <sys/stat.h>
#include <stdint.h>
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

/* POSIX XCU "test": the *number* of arguments decides how they are
 * read, before any operator is looked at.
 *   1  true if $1 is non-null                  "test !"      -> true
 *   2  "!" negates, otherwise a unary primary
 *   3  binary primary first, then "!", then "( x )"
 *                                              "test ( = )"  -> "(" = ")"
 *   4  "!" negates the 3-argument reading, then "( x y )"
 * Anything else POSIX leaves unspecified, and so are the "-a"/"-o"
 * forms of 3 and 4 arguments: the grammar below takes those, the way
 * every other shell does.
 * ----------------------------------------------------------------------- */

static const char unary_ops[] = "nzafdbchLSesrwxgput";

static int test_expr(void);

/* the words of the expression, and how far the grammar has read */
static char** test_v;
static int test_n, test_i, test_err;

static inline int
contains(const char* str, char ch) {
  return str[str_chr(str, ch)];
}

/* get modification time */
static int64
filetime(const char* arg) {
  struct stat st;

  if(stat(arg, &st) == -1)
    return -1;

  return st.st_mtime;
}

/* string arg to int64 */
static int64
intarg(const char* arg) {
  int64 num = INT64_MAX;

  scan_longlong(arg, &num);
  return num;
}

/* "-X", one letter, X one of the unary primaries */
static int
is_unary(const char* op) {
  return op[0] == '-' && op[1] && op[2] == '\0' && contains(unary_ops, op[1]);
}

static int
is_binary(const char* op) {
  static const char* const ops[] = {
      "=", "!=", "<", ">", "-eq", "-ne", "-lt", "-le", "-gt", "-ge", "-nt", "-ot", 0};
  const char* const* p;

  for(p = ops; *p; p++)
    if(str_equal(*p, op))
      return 1;

  return 0;
}

/* evaluate a unary primary ("-X arg")
 * ----------------------------------------------------------------------- */
static int
test_unary(const char* op, const char* arg) {
  struct stat st;

  switch(op[1]) {
    /* string is non-empty / empty */
    case 'n': return *arg != '\0';
    case 'z': return *arg == '\0';

    /* file exists */
    case 'a':
    case 'e': return stat(arg, &st) == 0;

    case 'f': return stat(arg, &st) == 0 && S_ISREG(st.st_mode);
    case 'd': return stat(arg, &st) == 0 && S_ISDIR(st.st_mode);
    case 'c': return stat(arg, &st) == 0 && S_ISCHR(st.st_mode);
    case 'b': return stat(arg, &st) == 0 && (st.st_mode & S_IFMT) == S_IFBLK;
    case 'p': return stat(arg, &st) == 0 && S_ISFIFO(st.st_mode);

#ifdef S_ISLNK
    /* a symbolic link is the one thing not to follow */
    case 'h':
    case 'L': return lstat(arg, &st) == 0 && S_ISLNK(st.st_mode);
#endif

#ifdef S_ISSOCK
    case 'S': return stat(arg, &st) == 0 && S_ISSOCK(st.st_mode);
#endif

    /* exists and is not empty */
    case 's': return stat(arg, &st) == 0 && st.st_size > 0;

    case 'r': return access(arg, R_OK) == 0;
    case 'w': return access(arg, W_OK) == 0;
    case 'x':
      return access(arg, X_OK) == 0;

      /* set-group-ID / set-user-ID bit */
#ifdef S_ISGID
    case 'g': return stat(arg, &st) == 0 && !!(st.st_mode & S_ISGID);
#endif
#ifdef S_ISUID
    case 'u': return stat(arg, &st) == 0 && !!(st.st_mode & S_ISUID);
#endif

    /* the fd is a terminal */
    case 't': {
      unsigned int fd = 1;

      scan_uint(arg, &fd);
      return isatty(fd);
    }
  }

  test_err = 1;
  return 0;
}

/* evaluate a binary primary ("arg OP arg")
 * ----------------------------------------------------------------------- */
static int
test_binary(const char* left, const char* op, const char* right) {
  if(op[0] != '-') {
    int r = str_diff(left, right);

    switch(op[0]) {
      case '=': return r == 0;
      case '!': return r != 0;
      case '<': return op[1] == '=' ? r <= 0 : r < 0;
      case '>': return op[1] == '=' ? r >= 0 : r > 0;
    }
  } else {
    int64 l = intarg(left), r = intarg(right);

    /* "-nt"/"-ot" (mtime) and "-ne"/"-eq" (numbers) share op[1], so
       op[2] is what tells them apart: "-nt" vs "-ne" */
    switch(op[1]) {
      case 'n': return op[2] == 't' ? filetime(left) > filetime(right) : l != r;
      case 'o': return filetime(left) < filetime(right);
      case 'e': return l == r;
      case 'l': return op[2] == 'e' ? l <= r : l < r;
      case 'g': return op[2] == 'e' ? l >= r : l > r;
    }
  }

  test_err = 1;
  return 0;
}

/* "( expr )", a primary, or a bare string
 * ----------------------------------------------------------------------- */
static int
test_primary(void) {
  const char* arg;

  if(test_i >= test_n) {
    test_err = 1;
    return 0;
  }

  arg = test_v[test_i];

  if(str_equal(arg, "(")) {
    int r;

    test_i++;
    r = test_expr();

    if(test_i >= test_n || !str_equal(test_v[test_i], ")")) {
      test_err = 1;
      return 0;
    }

    test_i++;
    return r;
  }

  /* an operator is only an operator where an operand can follow it:
     "test = = =" compares "=" with "=" */
  if(test_i + 2 < test_n && is_binary(test_v[test_i + 1])) {
    test_i += 3;
    return test_binary(test_v[test_i - 3], test_v[test_i - 2], test_v[test_i - 1]);
  }

  if(test_i + 1 < test_n && is_unary(arg)) {
    test_i += 2;
    return test_unary(arg, test_v[test_i - 1]);
  }

  test_i++;
  return *arg != '\0';
}

/* "! expr"
 * ----------------------------------------------------------------------- */
static int
test_not(void) {
  int neg = 0;

  while(test_i < test_n && str_equal(test_v[test_i], "!")) {
    /* the last word is an operand, not an operator: "test x -a !" */
    if(test_i + 1 >= test_n)
      break;

    neg = !neg;
    test_i++;
  }

  return neg ^ test_primary();
}

/* "expr -a expr", binding tighter than "-o"
 * ----------------------------------------------------------------------- */
static int
test_and(void) {
  int r = test_not();

  while(test_i < test_n && str_equal(test_v[test_i], "-a")) {
    test_i++;
    r = test_not() && r;
  }

  return r;
}

/* "expr -o expr"
 * ----------------------------------------------------------------------- */
static int
test_expr(void) {
  int r = test_and();

  while(test_i < test_n && str_equal(test_v[test_i], "-o")) {
    test_i++;
    r = test_and() || r;
  }

  return r;
}

/* evaluate n words by POSIX's argument-count table, falling back to
 * the grammar above for everything it does not cover
 * ----------------------------------------------------------------------- */
static int
test_count(int n, char** v) {
  switch(n) {
    case 0: return 0;

    case 1: return *v[0] != '\0';

    case 2:
      if(str_equal(v[0], "!"))
        return !test_count(1, v + 1);

      if(is_unary(v[0]))
        return test_unary(v[0], v[1]);

      break;

    case 3:
      if(is_binary(v[1]))
        return test_binary(v[0], v[1], v[2]);

      if(str_equal(v[0], "!"))
        return !test_count(2, v + 1);

      if(str_equal(v[0], "(") && str_equal(v[2], ")"))
        return test_count(1, v + 1);

      break;

    case 4:
      if(str_equal(v[0], "!"))
        return !test_count(3, v + 1);

      if(str_equal(v[0], "(") && str_equal(v[3], ")"))
        return test_count(2, v + 1);

      break;
  }

  test_v = v;
  test_n = n;
  test_i = 0;

  {
    int r = test_expr();

    if(test_i != test_n)
      test_err = 1;

    return r;
  }
}

/* test for expression
 * ----------------------------------------------------------------------- */
const char help_test[] = "    Evaluate a conditional expression (exit status 0 = true).\n"
                         "\n"
                         "    -e file         file exists\n"
                         "    -f file         file exists and is a regular file\n"
                         "    -d file         file exists and is a directory\n"
                         "    -r/-w/-x file   file is readable/writable/executable\n"
                         "    -s file         file exists and is non-empty\n"
                         "    -z string       string has zero length\n"
                         "    -n string       string has non-zero length\n"
                         "    s1 = s2         strings are equal (!= for not equal)\n"
                         "    n1 -eq n2       integers are equal (-ne/-lt/-le/-gt/-ge likewise)\n"
                         "    expr -a expr    both expressions are true (-o for either)\n"
                         "    ! expr          expr is false\n"
                         "\n"
                         "    '[' requires a matching trailing ']' argument.\n";

int
builtin_test(int argc, char* argv[]) {
  int result;
  int brackets = 0;

  if(argv[0][0] == '[') {
    brackets = 1;
    argc--;

    if(argv[argc][0] != ']') {
      builtin_errmsg(argv, "missing ]'", NULL);
      return EXIT_ERROR;
    }
  }

  /* POSIX: `test` and `[ ]` with no expression are false. */
  if(argc <= 1)
    return 1;

  (void)brackets;

  test_err = 0;
  result = test_count(argc - 1, argv + 1);

  if(test_err) {
    builtin_errmsg(argv, "invalid expression", NULL);
    return EXIT_ERROR;
  }

  return !result;
}
