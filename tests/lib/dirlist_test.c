/* lib/dirlist unit test over a generated tree; prints "<what>: OK|FAIL" */
#include "../../lib/dirlist.h"
#include "../../lib/windoze.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if WINDOWS_NATIVE
#include "../../lib/utf8.h"
#include <direct.h>
#include <io.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir((p), 0755)
#endif

static int failed;
static char root[512];

static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}

static void
path(char* out, const char* a, const char* b) {
  snprintf(out, 600, "%s/%s%s%s", root, a, b ? "/" : "", b ? b : "");
}

static void
touch(const char* d, const char* name) {
  char p[600];
  int fd;

  path(p, d, name);
#if WINDOWS_NATIVE
  { /* the narrow open() would use the ANSI codepage: create the name as UTF-8 */
    unsigned short w[600];

    w[u8tou16(w, 599, p, strlen(p), NULL)] = 0;
    fd = _wopen((const wchar_t*)w, O_WRONLY | O_CREAT, 0644);
  }
#else
  fd = open(p, O_WRONLY | O_CREAT, 0644);
#endif
  if(fd >= 0)
    close(fd);
}

static void
mkd(const char* name) {
  char p[600];

  path(p, name, NULL);
  MKDIR(p);
}

/* the record of name in d, or NULL */
static const char*
find(const struct dirlist* d, const char* name) {
  const char* p;

  for(p = d->s; p < d->s + d->len; p = dirlist_next(p))
    if(!strcmp(dirlist_name(p), name))
      return p;
  return NULL;
}

static int
rcmp(const char* a, const char* b) {
  return strcmp(b, a);
}

static int
sorted(const struct dirlist* d, int (*cmp)(const char*, const char*)) {
  const char *p, *q;
  size_t i = 0;

  for(p = d->s; p < d->s + d->len; p = q, i++) {
    q = dirlist_next(p);
    if(q < d->s + d->len && cmp(dirlist_name(p), dirlist_name(q)) > 0)
      return 0;
  }
  return i == d->n;
}

static void
test_basic(void) {
  arena a;
  struct dirlist d;
  const char* r;
  char p[600];

  mkd("t");
  touch("t", "file");
  touch("t", "\xc3\xa9t\xc3\xa9.txt");
  path(p, "t", "sub");
  MKDIR(p);

  arena_init(&a, &arena_heap, 0);

  path(p, "t", NULL);
  d = dirlist_read(&a, p, 0);
  check(d.s && d.n == 3, "read: three entries, dots dropped");
  check(d.s && !find(&d, ".") && !find(&d, ".."), "read: no . or ..");
  check(d.s && (r = find(&d, "file")) && (dirlist_type(r) == DL_REG || dirlist_type(r) == DL_UNKNOWN), "read: regular file type");
  check(d.s && (r = find(&d, "sub")) && (dirlist_type(r) == DL_DIR || dirlist_type(r) == DL_UNKNOWN), "read: directory type");
  check(d.s && find(&d, "\xc3\xa9t\xc3\xa9.txt"), "read: non-ASCII name comes back as UTF-8");

  d = dirlist_read(&a, p, DIRLIST_DOTS);
  check(d.s && d.n == 5 && find(&d, ".") && find(&d, ".."), "DIRLIST_DOTS: keeps . and ..");

  d = dirlist_read(&a, p, DIRLIST_SORT);
  check(d.s && d.n == 3 && sorted(&d, strcmp), "DIRLIST_SORT: records in str_diff order");

  d = dirlist_read(&a, p, DIRLIST_DOTS | DIRLIST_SORT);
  check(d.s && d.n == 5 && sorted(&d, strcmp) && dirlist_name(d.s)[0] == '.' && !dirlist_name(d.s)[1], "sort with dots: . first");

  if(d.s) {
    check(dirlist_sort(&a, &d, rcmp) == 0 && sorted(&d, rcmp), "dirlist_sort: caller comparator reorders in place");
  }

  path(p, "t", "file");
  errno = 0;
  d = dirlist_read(&a, p, 0);
  check(!d.s && errno == ENOTDIR, "non-directory path: NULL, ENOTDIR");

  path(p, "nonexistent", NULL);
  d = dirlist_read(&a, p, 0);
  check(!d.s && errno == ENOENT, "missing path: NULL, ENOENT");

  arena_free(&a);
}

static void
test_hidden(void) {
  arena a;
  struct dirlist d;
  const char* r;
  size_t n = 0;
  char p[600];

  mkd("h");
  touch("h", "vis");
  touch("h", ".hid");
  touch("h", "..two");
  path(p, "h", NULL);
  arena_init(&a, &arena_heap, 0);

  d = dirlist_read(&a, p, 0);
  check(d.s && d.n == 3 && find(&d, ".hid") && find(&d, "..two"), "default: hidden names kept, . and .. dropped");

  d = dirlist_read(&a, p, DIRLIST_NOHIDDEN);
  check(d.s && d.n == 1 && find(&d, "vis"), "NOHIDDEN: only the visible name");

  d = dirlist_read(&a, p, DIRLIST_NOHIDDEN | DIRLIST_DOTS);
  check(d.s && d.n == 1 && !find(&d, "."), "NOHIDDEN wins over DOTS");

  d = dirlist_read(&a, p, DIRLIST_DOTS);
  check(d.s && d.n == 5, "DOTS: everything");

  d = dirlist_read(&a, p, DIRLIST_NOHIDDEN | DIRLIST_SORT);
  check(d.s && d.n == 1 && sorted(&d, strcmp), "NOHIDDEN with SORT");

  d = dirlist_read(&a, p, 0);
  dirlist_foreach(r, d)
    n++;
  check(d.s && n == d.n, "dirlist_foreach visits every record once");
  arena_free(&a);
}

static void
test_empty(void) {
  arena a;
  struct dirlist d;
  char p[600];

  mkd("empty");
  path(p, "empty", NULL);
  arena_init(&a, &arena_heap, 0);
  d = dirlist_read(&a, p, 0);
  check(d.s && d.n == 0 && d.len == 0, "empty dir: valid empty list");
  d = dirlist_read(&a, p, DIRLIST_SORT);
  check(d.s && d.n == 0, "empty dir sorted: valid empty list");
  d = dirlist_read(&a, p, DIRLIST_DOTS);
  check(d.s && d.n == 2, "empty dir with dots: . and ..");
  arena_free(&a);
}

static void
test_many(void) {
  arena a;
  struct dirlist d;
  char p[600], n[64];
  int i, ok = 1;
  size_t used;

  mkd("many");
  for(i = 0; i < 20000; i++) {
    snprintf(n, sizeof(n), "entry_%05d_%d", (i * 7919) % 20000, i);
    touch("many", n);
  }
  path(p, "many", NULL);

  arena_init(&a, &arena_heap, 0);
  d = dirlist_read(&a, p, 0);
  check(d.s && d.n == 20000, "20000 entries: all read, unsorted");

  if(d.s) {
    const char* q = d.s;

    for(i = 0; i < 20000 && q < d.s + d.len; i++)
      q = dirlist_next(q);
    ok = i == 20000 && q == d.s + d.len;
  }
  check(ok, "20000 entries: len is exactly the sum of the records");

  d = dirlist_read(&a, p, DIRLIST_SORT);
  check(d.s && d.n == 20000 && sorted(&d, strcmp), "20000 entries: sorted");

  used = arena_used(&a);
  d = dirlist_read(&a, p, DIRLIST_SORT);
  check(d.s && arena_used(&a) - used < d.len + 4096 + 1024 * 1024, "20000 entries: sort scratch is returned to the arena");
  arena_free(&a);
}

static void
test_nomem(void) {
  static char buf[1024];
  arena a;
  struct dirlist d;
  char p[600];

  path(p, "many", NULL);
  arena_init_fixed(&a, buf, sizeof(buf));
  d = dirlist_read(&a, p, 0);
  check(!d.s && errno == ENOMEM && arena_used(&a) == 0, "arena too small: NULL, ENOMEM, arena unchanged");
}

#if !WINDOWS_NATIVE
static void
test_types(void) {
  arena a;
  struct dirlist d;
  const char* r;
  char p[600], q[600];

  mkd("ty");
  touch("ty", "reg");
  mkd("ty/dir");
  path(p, "ty", "reg");
  path(q, "ty", "lnk");
  check(symlink("reg", q) == 0, "types: symlink created");
  path(q, "ty", "fifo");
  check(mkfifo(q, 0644) == 0, "types: fifo created");
  path(q, "ty", "dangling");
  check(symlink("nowhere", q) == 0, "types: dangling symlink created");

  path(p, "ty", NULL);
  arena_init(&a, &arena_heap, 0);
  d = dirlist_read(&a, p, DIRLIST_SORT);
  check(d.s && d.n == 5, "types: five entries");
  /* a filesystem without d_type reports DL_UNKNOWN for everything */
  if(d.s && (r = find(&d, "reg")) && dirlist_type(r) == DL_UNKNOWN)
    puts("types: filesystem gives no d_type, exact types skipped");
  else {
    check((r = find(&d, "reg")) && dirlist_type(r) == DL_REG, "type: DL_REG");
    check((r = find(&d, "dir")) && dirlist_type(r) == DL_DIR, "type: DL_DIR");
    check((r = find(&d, "lnk")) && dirlist_type(r) == DL_LNK, "type: DL_LNK for a symlink");
    check((r = find(&d, "dangling")) && dirlist_type(r) == DL_LNK, "type: DL_LNK for a dangling symlink");
    check((r = find(&d, "fifo")) && dirlist_type(r) == DL_OTHER, "type: DL_OTHER for a fifo");
  }
  arena_free(&a);
}

static void
test_unreadable(void) {
  arena a;
  struct dirlist d;
  char p[600];

  mkd("noread");
  path(p, "noread", NULL);
  chmod(p, 0);
  if(!access(p, R_OK)) {
    puts("unreadable dir: still readable (root?), skipped");
  } else {
    arena_init(&a, &arena_heap, 0);
    d = dirlist_read(&a, p, 0);
    check(!d.s && errno == EACCES && arena_used(&a) == 0, "unreadable dir: NULL, EACCES, nothing allocated");
    arena_free(&a);
  }
  chmod(p, 0755);
}
#endif

static void
cleanup(void) {
  char cmd[600];

#if WINDOWS_NATIVE
  char* c;

  snprintf(cmd, sizeof(cmd), "rd /s /q \"%s\"", root);
  for(c = cmd; *c; c++)
    if(*c == '/')
      *c = '\\';
#else
  snprintf(cmd, sizeof(cmd), "rm -rf '%s'", root);
#endif
  if(system(cmd))
    puts("cleanup failed");
}

int
main(void) {
  const char* tmp = getenv("TMPDIR");

  snprintf(root, sizeof(root), "%s/dirlist_test.%d", tmp && *tmp ? tmp : "/tmp", (int)getpid());
  MKDIR(root);

  test_basic();
  test_hidden();
  test_empty();
  test_many();
  test_nomem();
#if !WINDOWS_NATIVE
  test_types();
  test_unreadable();
#endif

  cleanup();
  puts(failed ? "FAILED" : "all passed");
  return failed;
}
