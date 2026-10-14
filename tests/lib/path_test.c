/* lib/path unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../../lib/path.h"
#include "../../lib/byte.h"
#include "../../lib/str.h"
#include "../../lib/buffer.h"
#include <stdio.h>
#include <unistd.h>

static int failed;
#define check(cond, what) \
  do { \
    int ok = (cond); \
    printf("%s: %s\n", (what), (ok) ? "OK" : "FAIL"); \
    failed |= !(ok); \
  } while(0);

#define check_equal(a, b, what) \
  do { \
    int ok = (a) == (b); \
    printf("%s: ", (what)); \
    printf("%s (%d %s %d)\n", (ok) ? "OK" : "FAIL", (int)(a), (ok) ? "==" : "!=", (int)(b)); \
    failed |= !(ok); \
  } while(0);

#define CHECK_EQUAL(a, b) check_equal(a, b, (#a " == " #b))
#define CHECK_STREQUAL(a, b) check((a) && (b) && str_equal((a), (b)), #a " == " #b)

#define CHECK(condition) check((condition), #condition)

#define B(str) (str), (sizeof((str)) - 1)

int
main(void) {
  int result;
  char* s;
  size_t len;
  stralloc sa;
  stralloc_init(&sa);

  /* TEST: const char* path_basename(p, n, *len)  */
  CHECK_STREQUAL(path_basename_s("/usr/lib"), "lib");
  CHECK(path_basename(B("/usr/include/"), &len) && len == str_len("include"));
  CHECK(path_basename(B("//"), &len) && len == str_len("/"));
  CHECK_STREQUAL(path_basename_s(""), "");
  CHECK(path_basename(B("/usr/lib//"), NULL) && path_basename(B("/usr/lib//"), &len) && len == 3);

  /* TEST: size_t path_cat(out, cap, a, an, b, bn)  */
  {
    char buf[16];
    size_t n;

    n = path_cat(buf, sizeof buf, B("a"), B("b"));
    CHECK(n == 3 && !byte_diff(buf, 4, "a/b"));
    n = path_cat(buf, sizeof buf, B("a/"), B("b"));
    CHECK(n == 3 && !byte_diff(buf, 4, "a/b"));
    n = path_cat(buf, sizeof buf, B(""), B("b"));
    CHECK(n == 1 && !byte_diff(buf, 2, "b"));
    n = path_cat(buf, sizeof buf, B("a"), B(""));
    CHECK(n == 1 && !byte_diff(buf, 2, "a"));
    n = path_cat(buf, sizeof buf, B("a"), B("/b"));
    CHECK(n == 2 && !byte_diff(buf, 3, "/b"));

    /* in place: the walker idiom */
    byte_copy(buf, 2, "a");
    n = path_cat(buf, sizeof buf, buf, 1, B("bc"));
    CHECK(n == 4 && !byte_diff(buf, 5, "a/bc"));

    /* all or nothing: too small a buffer is left untouched */
    byte_copy(buf, 5, "xxxx");
    CHECK(path_cat(buf, 4, B("a"), B("bc")) == 4 && !byte_diff(buf, 5, "xxxx"));
    CHECK(path_cat(0, 0, B("a"), B("bc")) == 4);
  }

  /* TEST: size_t path_canon(out, cap, p, n)  */
  {
    static const char* const tab[][2] = {
        {"", "."}, {".", "."}, {"..", ".."}, {"/", "/"}, {"/..", "/"}, {"a/", "a"}, {"a//b/./c/../d", "a/b/d"},
        {"a/b/..", "a"}, {"a/..", "."}, {"../..", "../.."}, {"x/../../y", "../y"}, {"./a", "a"}, {"/a/b/../../..", "/"},
        {"/this/is/a/test/dir/..", "/this/is/a/test"},
    };
    char buf[64];
    size_t i, n;

    for(i = 0; i < sizeof(tab) / sizeof(tab[0]); ++i) {
      n = path_canon(buf, sizeof buf, tab[i][0], str_len(tab[i][0]));
      check(n == str_len(tab[i][1]) && !byte_diff(buf, n + 1, tab[i][1]), tab[i][0]);
    }

    byte_copy(buf, 9, "a/b/../c");
    n = path_canon(buf, sizeof buf, buf, 8);
    CHECK(n == 3 && !byte_diff(buf, 4, "a/c"));

    byte_copy(buf, 5, "xxxx");
    CHECK(path_canon(buf, 4, B("a/b")) == 4 && !byte_diff(buf, 5, "xxxx"));
  }

  /* TEST: int path_canonicalize(path, sa, symbolic)  */
  CHECK_STREQUAL((path_canonicalize("~/Documents/blah/../dummy/test.txt", &sa, 0), sa.s), "~/Documents/dummy/test.txt");
  CHECK_STREQUAL((path_canonicalize("/usr/lib/../include/", &sa, 0), sa.s), "/usr/include");

  buffer_putsa(buffer_2, &sa);
  buffer_putnlflush(buffer_2);

  /* TEST: size_t path_dirlen(path, len) */
  CHECK(path_dirlen("a/b/c", 5) == 3);
  CHECK(path_dirlen("a//b/", 5) == 1);
  CHECK(path_dirlen("/a", 2) == 1);
  CHECK(path_dirlen("a", 1) == 0);
  CHECK(path_dirlen("/", 1) == 1);
  CHECK(path_dirlen("", 0) == 0);

  /* TEST: int path_fnmatch(pattern, plen, string, slen, flags) */
  CHECK(path_fnmatch(B("foo"), B("foo"), 0) == 0);
  CHECK(path_fnmatch(B("foo"), B("bar"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("fo*"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("*bar"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("f*r"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("f*o*r"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("foobar*"), B("foobar"), 0) == 0);

  CHECK(path_fnmatch(B("fo?ar"), B("foobar"), 0) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("f??bar"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("fo?ar"), B("fooabar"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("foo[bc]r"), B("foobar"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("foob[a-z]r"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("foo[0-9]r"), B("foobar"), 0) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("foob[!0-9]r"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("foo[!a-z]r"), B("foobar"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("[[:digit:]]"), B("5"), 0) == 0);
  CHECK(path_fnmatch(B("[[:digit:]]"), B("a"), 0) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("[[:alpha:]]"), B("a"), 0) == 0);
  CHECK(path_fnmatch(B("[[:alpha:]]"), B("5"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("foo\\*bar"), B("foo*bar"), 0) == 0);
  CHECK(path_fnmatch(B("foo\\*bar"), B("foo*bar"), PATH_FNM_NOESCAPE) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("foo\\[bar"), B("foo[bar"), 0) == 0);

  CHECK(path_fnmatch(B(""), B(""), 0) == 0);
  CHECK(path_fnmatch(B(""), B("a"), 0) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("*"), B(""), 0) == 0);
  CHECK(path_fnmatch(B("*"), B("abc"), 0) == 0);
  CHECK(path_fnmatch(B("?"), B(""), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("f**o**bar"), B("foobar"), 0) == 0);
  CHECK(path_fnmatch(B("a*b?c[d-f]"), B("axybqce"), 0) == 0);
  CHECK(path_fnmatch(B("a*b?c[d-f]"), B("axybqcz"), 0) == PATH_FNM_NOMATCH);

  CHECK(path_fnmatch(B("[[:lower:]]"), B("m"), 0) == 0);
  CHECK(path_fnmatch(B("[[:upper:]]"), B("M"), 0) == 0);
  CHECK(path_fnmatch(B("[[:upper:]]"), B("m"), 0) == PATH_FNM_NOMATCH);
  CHECK(path_fnmatch(B("[[:space:]]"), B(" "), 0) == 0);
  CHECK(path_fnmatch(B("[[:space:]]"), B("a"), 0) == PATH_FNM_NOMATCH);

  /* TEST: int path_getcwd(sa)  */
  {
    stralloc cwd;
    stralloc_init(&cwd);
    stralloc_ready(&cwd, PATH_MAX + 2);
    byte_fill(cwd.s, cwd.a, 'A');

    CHECK(path_getcwd(&cwd));
    CHECK(cwd.len);
    CHECK(cwd.a > cwd.len && cwd.s[cwd.len] == '\0');
    CHECK(path_rootlen(cwd.s, cwd.len));
  }

  /* TEST: char* path_gethome(uid)  */
  CHECK(path_gethome(-1) == NULL);
  /*CHECK(path_gethome(0) != NULL);*/
  CHECK(path_gethome(1000) > 0);

  /* TEST: size_t path_lenb(ptr)  */
  CHECK_EQUAL(path_lenb(B("dir/subdir/subsub/")), 3);
  CHECK_EQUAL(path_lenb(B("./")), 1);
  CHECK_EQUAL(path_lenb(B("../")), 2);
  CHECK_EQUAL(path_lenb(B("/usr")), 0);
  CHECK_EQUAL(path_lenb(B("//usr")), 0);

  /* TEST: size_t path_rootlen(x, n)  */
  CHECK_EQUAL(path_rootlen(B("some/dir")), 0);
  CHECK_EQUAL(path_rootlen(B("/some/dir")), 1);
  CHECK_EQUAL(path_rootlen(B("//some/dir")), 2);
#if WINDOWS
  CHECK_EQUAL(path_rootlen(B("c:/windows")), 3);
  CHECK_EQUAL(path_rootlen(B("c:system32")), 2);
  CHECK_EQUAL(path_rootlen(B("c:/windows/")), 3);
#endif

  /* TEST: size_t path_rootlen(s)  */
  /*CHECK_EQUAL(path_rootlen("some/dir"), 0);
  CHECK_EQUAL(path_rootlen("/some/dir"), 1);
  CHECK_EQUAL(path_rootlen("//some/dir"), 2);
#if WINDOWS
  CHECK_EQUAL(path_rootlen("c:/windows"), 3);
  CHECK_EQUAL(path_rootlen("c:system32"), 2);
  CHECK_EQUAL(path_rootlen("c:/windows/"), 3);
#endif*/

  /* TEST: size_t path_dirlen(x, n)  */
  CHECK_EQUAL(path_dirlen(B("some/dir")), 4);
  CHECK_EQUAL(path_dirlen(B("/some/dir")), 5);
  CHECK_EQUAL(path_dirlen(B("//some/dir")), 6);
#if WINDOWS
  CHECK_EQUAL(path_dirlen(B("c:/windows/system32")), 10);
#endif

  /* TEST: SizePair path_commonlenb(b, blen, t, tlen)  */
  SizePair sp = path_commonlenb(B("some//of///this//is/common"), B("some/of/this/doesnt/match"));
  CHECK_EQUAL(sp.sz1, 17);
  CHECK_EQUAL(sp.sz2, 13);

  /* TEST: SizePair path_commonlen(b, t)  */
  CHECK_EQUAL(path_commonlen("some/of/this/is/common", "some/of/this/doesnt/match"), 13);

  return failed;
}
