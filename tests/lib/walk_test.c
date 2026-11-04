/* lib/walk unit test over generated trees; prints "<what>: OK|FAIL".
 * Everything is created, read and removed below one private directory
 * ($TMPDIR/walk_test.<pid>); nothing here deletes through the walker. */
#include "../../lib/walk.h"
#include "../../lib/windoze.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if WINDOWS_NATIVE
int
main(void) {
  puts("walk_test: skipped on Windows");
  return 0;
}
#else

static int failed;
static char root[512];

static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}

static void
path(char* out, const char* rel) {
  snprintf(out, 1024, "%s/%s", root, rel);
}

static void
mkd(const char* rel) {
  char p[1024];

  path(p, rel);
  mkdir(p, 0755);
}

static void
touch(const char* rel) {
  char p[1024];
  int fd;

  path(p, rel);
  if((fd = open(p, O_WRONLY | O_CREAT, 0644)) >= 0)
    close(fd);
}

/* symlink at rel pointing at target (verbatim) */
static void
lnk(const char* rel, const char* target) {
  char p[1024];

  path(p, rel);
  if(symlink(target, p))
    perror("symlink");
}

/* what a walk saw */
struct rec {
  int phase, depth, type, islink, kept, err;
  char rel[128];
};

static struct rec R[256];
static int nr;

/* run a walk and record it. skip: rel of a directory to skip at WALK_PRE;
 * keep: rel of an entry to walk_keep(). */
static void
run(const char* from, unsigned flags, int maxdepth, const char* skip, const char* keep) {
  struct walk w;
  struct walk_ent* e;

  nr = 0;
  if(walk_open(&w, from, flags)) {
    puts("walk_open failed");
    failed = 1;
    return;
  }
  w.maxdepth = maxdepth;

  while((e = walk_read(&w))) {
    if(nr < 256) {
      struct rec* r = &R[nr++];

      r->phase = e->phase;
      r->depth = e->depth;
      r->type = e->type;
      r->islink = e->islink;
      r->kept = e->kept;
      r->err = e->err;
      snprintf(r->rel, sizeof(r->rel), "%s", e->rel);
    }
    if(skip && e->phase == WALK_PRE && !strcmp(e->rel, skip))
      walk_skip(&w);
    if(keep && e->phase != WALK_POST && !strcmp(e->rel, keep))
      walk_keep(&w);
  }
  walk_close(&w);
}

static int
idx(int phase, const char* rel) {
  int i;

  for(i = 0; i < nr; i++)
    if(R[i].phase == phase && !strcmp(R[i].rel, rel))
      return i;
  return -1;
}

static int
count(int phase) {
  int i, n = 0;

  for(i = 0; i < nr; i++)
    n += R[i].phase == phase;
  return n;
}

/* is any recorded rel below dir ("dir/...") */
static int
below(const char* dir) {
  size_t n = strlen(dir);
  int i;

  for(i = 0; i < nr; i++)
    if(!strncmp(R[i].rel, dir, n) && R[i].rel[n] == '/')
      return 1;
  return 0;
}

/* every PRE has its POST after it, properly nested, and every entry sits
 * directly below the innermost open directory */
static int
nested(void) {
  char stack[32][128];
  int i, sp = 0;

  for(i = 0; i < nr; i++) {
    struct rec* r = &R[i];
    const char* slash = strrchr(r->rel, '/');
    size_t pl = slash ? (size_t)(slash - r->rel) : 0;

    if(r->phase == WALK_POST) {
      if(!sp || strcmp(stack[sp - 1], r->rel))
        return 0;
      sp--;
      continue;
    }
    if(r->depth > 0 && (!sp || strlen(stack[sp - 1]) != pl || strncmp(stack[sp - 1], r->rel, pl) ||
                         (pl == 0 && strcmp(stack[sp - 1], ""))))
      if(!(sp && !slash && !stack[sp - 1][0]))
        return 0;
    if(r->phase == WALK_PRE)
      snprintf(stack[sp++], 128, "%s", r->rel);
  }
  return sp == 0;
}

static void
test_tree(void) {
  char p[1024];

  mkd("t");
  mkd("t/a");
  mkd("t/a/sub");
  mkd("t/empty");
  touch("t/a/f1");
  touch("t/a/sub/f2");
  touch("t/b");
  path(p, "t");

  run(p, 0, -1, 0, 0);
  check(nr == 3 && count(WALK_FILE) == 3, "default: only the three files");
  check(idx(WALK_FILE, "a/f1") >= 0 && idx(WALK_FILE, "a/sub/f2") >= 0 && idx(WALK_FILE, "b") >= 0, "default: rel is below the root");
  check(R[0].type == DL_REG && !R[0].islink, "file: type DL_REG, not a link");
  check(R[idx(WALK_FILE, "a/sub/f2")].depth == 3 && R[idx(WALK_FILE, "b")].depth == 1, "file: depth counts components");

  run(p, WALK_PRE | WALK_POST, -1, 0, 0);
  check(count(WALK_PRE) == 4 && count(WALK_POST) == 4 && count(WALK_FILE) == 3, "PRE|POST: four directories, three files");
  check(R[0].phase == WALK_PRE && R[0].rel[0] == '\0' && R[0].depth == 0, "root is the first visit, rel empty");
  check(R[nr - 1].phase == WALK_POST && R[nr - 1].rel[0] == '\0', "root is the last visit");
  check(nested(), "PRE and POST nest, entries sit below the open directory");
  check(R[idx(WALK_PRE, "a")].type == DL_DIR, "directory: type DL_DIR");

  strcat(p, "/");
  run(p, WALK_FILE, -1, 0, 0);
  check(idx(WALK_FILE, "a/f1") >= 0, "root with trailing slash: rel without leading slash");
}

static void
test_path(void) {
  struct walk w;
  struct walk_ent* e;
  char p[1024], want[1100];
  int ok = 0;

  path(p, "t");
  walk_open(&w, p, 0);
  while((e = walk_read(&w)))
    if(!strcmp(e->rel, "a/sub/f2")) {
      snprintf(want, sizeof(want), "%s/a/sub/f2", p);
      ok = !strcmp(e->path, want) && e->len == strlen(want) && !strcmp(e->name, "f2");
    }
  walk_close(&w);
  check(ok, "entry: path, len and name");

  path(p, "t/b");
  run(p, 0, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_FILE && R[0].depth == 0 && R[0].rel[0] == '\0', "a file as root: one WALK_FILE at depth 0");
}

static void
test_errors(void) {
  char p[1024];

  path(p, "missing");
  run(p, WALK_ERR, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_ERR && R[0].err == ENOENT, "missing root: one WALK_ERR, ENOENT");

  run(p, 0, -1, 0, 0);
  check(nr == 0, "missing root without WALK_ERR: silent");

  mkd("u");
  mkd("u/locked");
  mkd("u/ok");
  touch("u/locked/hidden");
  touch("u/ok/x");
  path(p, "u/locked");
  chmod(p, 0);
  if(!access(p, R_OK)) {
    puts("unreadable directory: still readable (root?), skipped");
  } else {
    path(p, "u");
    run(p, WALK_PRE | WALK_POST | WALK_ERR, -1, 0, 0);
    check(idx(WALK_ERR, "locked") >= 0 && R[idx(WALK_ERR, "locked")].err == EACCES, "unreadable directory: WALK_ERR, EACCES");
    check(idx(WALK_POST, "locked") < 0 && idx(WALK_PRE, "locked") >= 0, "unreadable directory: PRE yes, POST no");
    check(idx(WALK_FILE, "ok/x") >= 0 && idx(WALK_POST, "") >= 0, "unreadable directory: the walk goes on");
    run(p, WALK_POST, -1, 0, 0);
    check(idx(WALK_FILE, "ok/x") >= 0, "unreadable directory without WALK_ERR: silent");
  }
  path(p, "u/locked");
  chmod(p, 0755);
}

/* the point of the exercise: nothing outside the root is ever visited */
static void
test_symlinks(void) {
  char p[1024];

  mkd("outside");
  touch("outside/precious");
  mkd("outside/deep");
  touch("outside/deep/precious2");
  mkd("v");
  mkd("v/real");
  touch("v/real/f");
  lnk("v/tooutside", "../outside");
  lnk("v/abs", root);
  lnk("v/up", "..");
  lnk("v/dangling", "nowhere");
  lnk("v/tofile", "real/f");
  path(p, "v");

  run(p, WALK_PRE | WALK_POST | WALK_ERR | WALK_CYCLE, -1, 0, 0);
  check(idx(WALK_FILE, "tooutside") >= 0 && R[idx(WALK_FILE, "tooutside")].type == DL_LNK && R[idx(WALK_FILE, "tooutside")].islink,
        "physical: symlink to a directory is a file of type DL_LNK");
  check(idx(WALK_FILE, "up") >= 0 && idx(WALK_FILE, "abs") >= 0 && idx(WALK_FILE, "dangling") >= 0 && idx(WALK_FILE, "tofile") >= 0,
        "physical: every other link too");
  check(!below("tooutside") && !below("abs") && !below("up") && count(WALK_PRE) == 2 && count(WALK_CYCLE) == 0 && count(WALK_ERR) == 0,
        "physical: nothing below a link, no cycle");

  run(p, WALK_PRE | WALK_POST | WALK_ERR | WALK_CYCLE, 2, 0, 0);
  check(!below("tooutside") && !below("abs") && !below("up"), "physical, maxdepth: still nothing below a link");

  /* a link as the root */
  path(p, "v/tooutside");
  run(p, WALK_PRE | WALK_POST | WALK_ERR, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_FILE && R[0].type == DL_LNK, "root is a link to a directory: one file, not entered");

  strcat(p, "/");
  run(p, WALK_PRE | WALK_POST | WALK_ERR, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_FILE && R[0].type == DL_LNK && !below(""), "root \"link/\": still the link, not the directory");

  path(p, "v/abs");
  strcat(p, "/");
  run(p, WALK_PRE | WALK_POST | WALK_ERR, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_FILE && R[0].type == DL_LNK, "root \"link-to-/tmp/..../\": one file");

  path(p, "v/up");
  run(p, WALK_PRE | WALK_POST | WALK_ERR, -1, 0, 0);
  check(nr == 1 && R[0].phase == WALK_FILE, "root is a link to \"..\": one file");

  path(p, "v/tooutside");
  run(p, WALK_COMFOLLOW | WALK_PRE | WALK_POST, -1, 0, 0);
  check(idx(WALK_FILE, "precious") >= 0 && idx(WALK_FILE, "deep/precious2") >= 0 && R[0].islink && R[0].type == DL_DIR,
        "WALK_COMFOLLOW: the root link is followed");

  path(p, "v");
  run(p, WALK_COMFOLLOW | WALK_PRE | WALK_POST, -1, 0, 0);
  check(!below("tooutside"), "WALK_COMFOLLOW: links below the root are still not followed");

  run(p, WALK_LOGICAL | WALK_PRE | WALK_POST | WALK_CYCLE | WALK_ERR, -1, 0, 0);
  check(idx(WALK_FILE, "tooutside/precious") >= 0 && R[idx(WALK_PRE, "tooutside")].islink && R[idx(WALK_PRE, "tooutside")].type == DL_DIR,
        "WALK_LOGICAL: a link to a directory is entered, type DL_DIR, islink");
  check(idx(WALK_FILE, "tofile") >= 0 && R[idx(WALK_FILE, "tofile")].type == DL_REG && R[idx(WALK_FILE, "tofile")].islink,
        "WALK_LOGICAL: a link to a file has the file's type");
  check(idx(WALK_FILE, "dangling") >= 0 && R[idx(WALK_FILE, "dangling")].type == DL_LNK, "WALK_LOGICAL: dangling link stays DL_LNK");
  /* "up" and "abs" lead above the root and so through "v" itself again */
  check(count(WALK_CYCLE) >= 2 && idx(WALK_CYCLE, "up/v") >= 0 && idx(WALK_PRE, "up/v") < 0, "WALK_LOGICAL: a path back to a directory above is WALK_CYCLE, not entered");

  run(p, WALK_LOGICAL | WALK_POST, -1, 0, 0);
  check(idx(WALK_FILE, "tooutside/precious") >= 0 && idx(WALK_POST, "") >= 0, "WALK_LOGICAL without CYCLE flag: silent, terminates");

  mkd("loop");
  lnk("loop/self", ".");
  mkd("loop/d");
  lnk("loop/d/back", "..");
  path(p, "loop");
  run(p, WALK_LOGICAL | WALK_CYCLE, -1, 0, 0);
  check(count(WALK_CYCLE) == 2 && count(WALK_FILE) == 0, "loop: two links, two cycles, walk ends");
}

static void
test_skip_keep_depth(void) {
  char p[1024];

  path(p, "t");

  run(p, WALK_PRE | WALK_POST, -1, "a", 0);
  check(idx(WALK_PRE, "a") >= 0 && !below("a") && idx(WALK_POST, "a") < 0, "walk_skip: directory not read, POST not reported");
  check(idx(WALK_FILE, "b") >= 0 && idx(WALK_POST, "empty") >= 0 && idx(WALK_POST, "") >= 0, "walk_skip: siblings and the root still reported");

  run(p, WALK_PRE | WALK_POST, -1, "", 0);
  check(nr == 1 && R[0].phase == WALK_PRE, "walk_skip on the root: that is all");

  run(p, WALK_POST, -1, 0, "a/sub/f2");
  check(R[idx(WALK_POST, "a/sub")].kept && R[idx(WALK_POST, "a")].kept && R[idx(WALK_POST, "")].kept, "walk_keep: parent, grandparent and root report kept");
  check(!R[idx(WALK_POST, "empty")].kept, "walk_keep: a directory elsewhere does not");

  run(p, WALK_POST, -1, 0, "b");
  check(R[idx(WALK_POST, "")].kept && !R[idx(WALK_POST, "a")].kept, "walk_keep on a root file: only the root");

  run(p, WALK_PRE | WALK_POST, -1, "a", "a");
  check(R[idx(WALK_POST, "")].kept && idx(WALK_POST, "a") < 0, "skip+keep on a PRE: the parent is kept");

  run(p, WALK_PRE | WALK_POST, 1, 0, 0);
  check(idx(WALK_PRE, "a") >= 0 && idx(WALK_POST, "a") >= 0 && !below("a") && idx(WALK_FILE, "b") >= 0, "maxdepth 1: depth-1 directories reported, not read");

  run(p, WALK_PRE | WALK_POST, 0, 0, 0);
  check(nr == 2 && R[0].phase == WALK_PRE && R[1].phase == WALK_POST, "maxdepth 0: the root alone");

  run(p, 0, 2, 0, 0);
  check(idx(WALK_FILE, "a/f1") >= 0 && idx(WALK_FILE, "a/sub/f2") < 0, "maxdepth 2: files at depth 2, none deeper");
}

static void
test_xdev(void) {
  struct stat a, b;

  if(stat("/", &a) || stat("/proc", &b) || a.st_dev == b.st_dev) {
    puts("xdev: /proc is not a mount point here, skipped");
    return;
  }
  run("/", WALK_XDEV | WALK_PRE | WALK_POST, 2, 0, 0);
  check(idx(WALK_PRE, "proc") >= 0 && idx(WALK_POST, "proc") >= 0 && !below("proc"), "WALK_XDEV: mount point reported, not read");
  run("/", WALK_PRE | WALK_POST, 2, 0, 0);
  check(below("proc"), "without WALK_XDEV: it is read");
}

static void
test_nesting(void) {
  struct walk a, b;
  struct walk_ent *x, *y;
  char p[1024];
  int na = 0, nb = 0;

  path(p, "t");
  walk_open(&a, p, 0);
  walk_open(&b, p, WALK_PRE | WALK_POST);
  for(;;) {
    x = walk_read(&a);
    y = walk_read(&b);
    na += x != NULL;
    nb += y != NULL;
    if(!x && !y)
      break;
  }
  walk_close(&a);
  walk_close(&b);
  check(na == 3 && nb == 11, "two walks in step on one tree are independent");
}

/* walk_kids: the current directory's entries, then the walk goes through the same list */
static void
test_kids(void) {
  struct walk w;
  struct walk_ent* e;
  const struct dirlist* k;
  char p[1024];
  int nfiles = 0, root_n = -1, empty_n = -1, a_n = -1, file_null = 0, enotdir = 0, post_n = -1, ndirs = 0;

  path(p, "t");
  walk_open(&w, p, WALK_PRE | WALK_POST);
  while((e = walk_read(&w))) {
    nfiles += e->phase == WALK_FILE;
    ndirs += e->phase == WALK_PRE;

    if(e->phase == WALK_PRE) {
      k = walk_kids(&w);
      if(!e->rel[0])
        root_n = k ? (int)k->n : -2;
      else if(!strcmp(e->rel, "empty"))
        empty_n = k ? (int)k->n : -2;
      else if(!strcmp(e->rel, "a"))
        a_n = k ? (int)k->n : -2;
    }

    if(e->phase == WALK_FILE) {
      file_null = walk_kids(&w) == NULL;
      enotdir = errno == ENOTDIR;
    }
  }
  walk_close(&w);
  check(root_n == 3 && a_n == 2 && empty_n == 0, "walk_kids at PRE: the entries of the directory (root 3, a 2, empty 0)");
  check(nfiles == 3 && ndirs == 4, "walk_kids at PRE: the walk still visits everything, through the same list");
  check(file_null && enotdir, "walk_kids on a file: NULL, ENOTDIR");

  /* asked for at PRE and skipped: nothing is walked, nothing is left over */
  walk_open(&w, p, WALK_PRE | WALK_POST);
  nfiles = 0;
  while((e = walk_read(&w))) {
    nfiles += e->phase == WALK_FILE;
    if(e->phase == WALK_PRE && !strcmp(e->rel, "a")) {
      walk_kids(&w);
      walk_skip(&w);
    }
  }
  walk_close(&w);
  check(nfiles == 1, "walk_kids then walk_skip: the directory is not walked (only b is left)");

  /* asked for beyond maxdepth: reported, not descended */
  walk_open(&w, p, WALK_PRE | WALK_POST);
  w.maxdepth = 1;
  nfiles = 0;
  a_n = -1;
  while((e = walk_read(&w))) {
    nfiles += e->phase == WALK_FILE;
    if(e->phase == WALK_PRE && !strcmp(e->rel, "a"))
      a_n = (int)walk_kids(&w)->n;
  }
  walk_close(&w);
  check(a_n == 2 && nfiles == 1, "walk_kids beyond maxdepth: the entries are given, the directory is not walked");

  /* at WALK_POST the directory is read afresh: a removed child is gone from it */
  mkd("k");
  mkd("k/d");
  touch("k/d/f");
  path(p, "k");
  walk_open(&w, p, WALK_POST);
  while((e = walk_read(&w))) {
    char fp[1024];

    if(e->phase == WALK_FILE) {
      path(fp, "k/d/f");
      unlink(fp);
    }
    if(e->phase == WALK_POST && !strcmp(e->rel, "d"))
      post_n = (int)walk_kids(&w)->n;
  }
  walk_close(&w);
  check(post_n == 0, "walk_kids at POST: reads the directory as it is then (the file was removed)");
}

static void
test_big(void) {
  struct walk w;
  char p[1024], name[64];
  int i, n = 0;

  mkd("big");
  for(i = 0; i < 20000; i++) {
    snprintf(name, sizeof(name), "big/file_%05d_abcdefghij", i);
    touch(name);
  }
  path(p, "big");
  walk_open(&w, p, 0);
  while(walk_read(&w))
    n++;
  walk_close(&w);
  check(n == 20000, "20000 entries in one directory");
}

static void
test_deep(void) {
  struct walk w;
  struct walk_ent* e;
  char p[1024];
  int i, fd, pre = 0, post = 0, files = 0, maxd = 0, err = 0;

  mkd("deep");
  path(p, "deep");
  if((fd = open(".", O_RDONLY)) < 0 || chdir(p)) {
    check(0, "deep: chdir");
    return;
  }
  for(i = 0; i < 1500; i++)
    if(mkdir("d", 0755) || chdir("d")) {
      err = 1;
      break;
    }
  close(open("leaf", O_WRONLY | O_CREAT, 0644));
  if(fchdir(fd))
    perror("fchdir");
  close(fd);
  check(!err, "deep: 1500 nested directories created");

  walk_open(&w, p, WALK_PRE | WALK_POST | WALK_ERR);
  while((e = walk_read(&w))) {
    pre += e->phase == WALK_PRE;
    post += e->phase == WALK_POST;
    files += e->phase == WALK_FILE;
    err += e->phase == WALK_ERR;
    if(e->depth > maxd)
      maxd = e->depth;
  }
  walk_close(&w);
  check(pre == 1501 && post == 1501 && files == 1 && !err && maxd == 1501, "deep: 1500 levels, no recursion limit, path buffer grows");
}

static void
cleanup(void) {
  char cmd[1200];

  /* only ever our own private directory */
  if(!strstr(root, "/walk_test.")) {
    puts("cleanup: refusing, root is not a walk_test directory");
    return;
  }
  snprintf(cmd, sizeof(cmd), "chmod -R u+rwx '%s' 2>/dev/null; rm -rf -- '%s'", root, root);
  if(system(cmd))
    puts("cleanup failed");
}

int
main(void) {
  const char* tmp = getenv("TMPDIR");

  snprintf(root, sizeof(root), "%s/walk_test.%d", tmp && *tmp ? tmp : "/tmp", (int)getpid());
  if(mkdir(root, 0755)) {
    perror(root);
    return 1;
  }

  test_tree();
  test_path();
  test_errors();
  test_symlinks();
  test_skip_keep_depth();
  test_xdev();
  test_nesting();
  test_kids();
  test_big();
  test_deep();

  cleanup();
  puts(failed ? "FAILED" : "all passed");
  return failed;
}

#endif
