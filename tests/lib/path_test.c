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

  /* TEST: int path_resolve(p, n, sa, symbolic, cwd, cwdn): absolute, canonical; returns 1 + links */
  CHECK(path_resolve(B("~/Documents/blah/../dummy/test.txt"), &sa, 0, B("/h")) == 1);
  CHECK_STREQUAL(sa.s, "/h/~/Documents/dummy/test.txt");
  CHECK(path_resolve(B("/usr/lib/../include/"), &sa, 0, 0, 0) == 1);
  CHECK_STREQUAL(sa.s, "/usr/include");
  CHECK(path_resolve(B("x/../y"), &sa, 1, B("/a")) == 1);
  CHECK_STREQUAL(sa.s, "/a/y");
  CHECK(path_resolve(B("//tmp///"), &sa, 1, 0, 0) == 1);
  CHECK_STREQUAL(sa.s, "/tmp");
  CHECK(path_resolve(B("//tmp///"), &sa, 0, 0, 0) == 1);
  CHECK_STREQUAL(sa.s, "/tmp");
  CHECK(path_resolve(B("/.."), &sa, 1, 0, 0) == 1);
  CHECK_STREQUAL(sa.s, "/");
  CHECK(path_resolve(B("."), &sa, 1, B("/a/b")) == 1);
  CHECK_STREQUAL(sa.s, "/a/b");
  CHECK(path_resolve(B(""), &sa, 1, B("/a")) == 1);
  CHECK_STREQUAL(sa.s, "/a");
  CHECK(path_resolve(B("rel"), &sa, 1, 0, 0) == 1 && sa.s[0] == '/' && sa.len > 4);

  /* TEST: path_canon collapses a leading separator run (realpath -m -s) */
  {
    char buf[16];

    CHECK(path_canon(buf, sizeof buf, B("//a")) == 2 && !byte_diff(buf, 3, "/a"));
    CHECK(path_canon(buf, sizeof buf, B("///")) == 1 && !byte_diff(buf, 2, "/"));
    byte_copy(buf, 4, "//a");
    CHECK(path_canon(buf, sizeof buf, buf, 3) == 2 && !byte_diff(buf, 3, "/a"));
  }

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

  /* TEST: int path_isabs(p, n), path_isrel(p, n); "//" roots are absolute (the old rootlen & 1 said no) */
  CHECK(path_isabs(B("/")));
  CHECK(path_isabs(B("/a")));
  CHECK(path_isabs(B("//")));
  CHECK(path_isabs(B("//a//b//")));
  CHECK(!path_isabs(B("")));
  CHECK(!path_isabs(B("a/b")));
  CHECK(!path_isabs(B("./a")));
  CHECK(path_isrel(B("")));
  CHECK(path_isrel(B("a")));
  CHECK(!path_isrel(B("/a")));
  CHECK(!path_isrel(B("//a")));
  CHECK(!path_isabs("/a", 0));

  /* TEST: size_t path_next(p, n, pos): end of the next component prefix, 0 at the end */
  CHECK_EQUAL(path_next(B("/a//b/"), 0), 2);
  CHECK_EQUAL(path_next(B("/a//b/"), 2), 5);
  CHECK_EQUAL(path_next(B("/a//b/"), 5), 0);
  CHECK_EQUAL(path_next(B("a"), 0), 1);
  CHECK_EQUAL(path_next(B("a"), 1), 0);
  CHECK_EQUAL(path_next(B("//a"), 0), 3);
  CHECK_EQUAL(path_next(B("/"), 0), 0);
  CHECK_EQUAL(path_next(B(""), 0), 0);
  CHECK_EQUAL(path_next(B("a"), 9), 0);
  {
    size_t k, steps = 0;
    const char p[] = "a/b/c";

    for(k = 0; (k = path_next(p, sizeof(p) - 1, k)); ++steps)
      CHECK_EQUAL(k, 2 * (int)steps + 1);

    CHECK_EQUAL(steps, 3);
  }

  /* TEST: size_t path_parentlen(p, n): strictly shorter, 0 at the end; the root is a parent */
  CHECK_EQUAL(path_parentlen(B("a/b/c")), 3);
  CHECK_EQUAL(path_parentlen(B("a//b/")), 1);
  CHECK_EQUAL(path_parentlen(B("/a/b")), 2);
  CHECK_EQUAL(path_parentlen(B("/a")), 1);
  CHECK_EQUAL(path_parentlen(B("//a")), 2);
  CHECK_EQUAL(path_parentlen(B("a")), 0);
  CHECK_EQUAL(path_parentlen(B("a/")), 0);
  CHECK_EQUAL(path_parentlen(B("/")), 0);
  CHECK_EQUAL(path_parentlen(B("")), 0);
  {
    size_t k, steps = 0;
    const char p[] = "/a/b/c";

    for(k = path_parentlen(p, sizeof(p) - 1); k; k = path_parentlen(p, k))
      ++steps;

    CHECK_EQUAL(steps, 3); /* "/a/b", "/a", "/" */
  }

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

  /* TEST: size_t path_common(a, an, b, bn, &blen)  */
  {
    const char *a = "some//of///this//is/common", *b = "some/of/this/doesnt/match";
    size_t bl, al = path_common(a, str_len(a), b, str_len(b), &bl);

    CHECK_EQUAL(al, 15);
    CHECK_EQUAL(bl, 12);
  }

  /* TEST: path_listlen / path_listnext / PATH_LIST_FOREACH  */
  {
    const char* l = "a;bc::d:";
    const char* e;
    size_t el, got = 0, lens[8];

    PATH_LIST_FOREACH(l, str_len(l), e, el) {
      if(got < 8)
        lens[got] = el;
      ++got;
    }

#if WINDOWS_NATIVE
    CHECK_EQUAL(got, 4);
#else
    CHECK_EQUAL(got, 5);
    CHECK_EQUAL(lens[0], 1);
    CHECK_EQUAL(lens[1], 2);
    CHECK_EQUAL(lens[2], 0);
    CHECK_EQUAL(lens[3], 1);
    CHECK_EQUAL(lens[4], 0);
#endif
    CHECK_EQUAL(path_listnext("a:b", 3, 2), (size_t)-1);
  }

  return failed;
}
