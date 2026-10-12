/* lib/path unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../../lib/path.h"
#include "../../lib/buffer.h"
#include <stdio.h>
#include <unistd.h>

static int failed;

/*static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}*/

#define check(cond, what) do { \
  int ok = (cond); \
  printf("%s: %s\n", (what), (ok) ? "OK" : "FAIL"); \
  failed |= !(ok); \
} while(0);

#define check_equal(a, b, what) do { \
  int ok = (a) == (b); \
  printf("%s: ", (what)); \
  printf((ok) ? "OK\n" : "FAIL (%d != %d)\n", (a), (b)); \
  failed |= !(ok); \
} while(0);


#define CHECK_EQUAL(a, b) check_equal(a , b, (#a " == " #b))
#define CHECK_STREQUAL(a, b) check((a) && (b) && !strcmp((a), (b)), #a " == " #b)

#define CHECK(condition) check((condition), #condition)

#define B(str) (str), (sizeof((str)) - 1)

int
main(void) {
  int result;
  char* s;
  size_t len;
  stralloc sa;
  stralloc_init(&sa);

  /* TEST: char* path_basename(path, *len)  */
  CHECK_STREQUAL(path_basename("/usr/lib", NULL), "lib");
  CHECK(path_basename("/usr/include/", &len) && len == strlen("include"));
  CHECK(path_basename("//", &len) && len == strlen("/"));
  CHECK_STREQUAL(path_basename("", &len), "");

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
    memset(cwd.s, 'A', cwd.a);

    CHECK(path_getcwd(&cwd));
    CHECK(stralloc_length(&cwd));
    CHECK(cwd.a > cwd.len && cwd.s[cwd.len] == '\0');
    CHECK(path_rootlen(cwd.s, cwd.len));
  }

  /* TEST: char* path_gethome(uid)  */
  CHECK(path_gethome(-1) == NULL);
  /*CHECK(path_gethome(0) != NULL);*/
  CHECK(path_gethome(getuid()) > 0);

  /* TEST: size_t path_len(ptr)  */
  CHECK_EQUAL(path_len(B("dir/subdir/subsub/")), 3);
  CHECK_EQUAL(path_len(B("./")), 1);
  CHECK_EQUAL(path_len(B("../")), 2);
  CHECK_EQUAL(path_len(B("/usr")), 0);
  CHECK_EQUAL(path_len(B("//usr")), 0);

  /* TEST: size_t path_rootlen(x, n)  */
  CHECK(path_rootlen("some/dir", 8) == 0);
  CHECK(path_rootlen("/some/dir", 9) == 1);
  CHECK(path_rootlen("//some/dir", 10) == 2);
  CHECK(path_rootlen("c:/windows", 10) == 3);
  CHECK(path_rootlen("c:system32", 10) == 2);

#if WINDOWS_NATIVE
  CHECK(path_rootlen(B("c:/windows/")) == 3);
#endif

  return failed;
}
