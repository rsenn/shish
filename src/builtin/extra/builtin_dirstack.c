#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../var.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/scan.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"

/* pushd, popd and dirs.
 *
 * The stack is a persistent singly-linked list: a node is never modified once
 * linked, so a subshell and its parent share every node below the point where
 * the subshell started, and pushd/popd in the subshell only move the head.
 *
 *   head -> [s1] -> [s2] -> ... -> [sk]       dirs prints: cwd s1 s2 ... sk
 *
 * The first change inside an env records a fence (saved head) and hangs a
 * finalizer on that env; sh_pop() runs it, which frees the nodes the env
 * allocated (always a prefix of the list) and puts the saved head back.
 * ----------------------------------------------------------------------- */

struct dirnode {
  struct dirnode* next;
  struct fence* owner; /* fence that allocated it */
  size_t len;
  char path[];
};

struct fence {
  struct fence* up;       /* enclosing fence */
  struct env* env;        /* env this fence guards */
  pid_t pid;              /* process that made it; a fork() child does not inherit the fences */
  struct dirnode* saved;  /* head before this env changed the stack */
  struct handler fin;     /* in env->finalizers */
};

static struct dirnode* head;
static struct fence root_fence = {NULL, &sh_root, 0, NULL, {NULL, NULL}};
static struct fence* top = &root_fence;

static int
env_within(struct env* e, struct env* s) {
  for(; s; s = s->parent)
    if(s == e)
      return 1;

  return 0;
}

/* free the nodes of the innermost fence and restore the stack it saved */
static void
fence_drop(void) {
  struct fence* f = top;
  struct dirnode* n;

  while(head && head->owner == f) {
    n = head;
    head = n->next;
    alloc_free(n);
  }

  head = f->saved;
  top = f->up;
  alloc_free(f);
}

static void
dirstack_finalize(void) {
  fence_drop();
}

/* called before every change: make "top" the fence of the current env */
static void
fence_enter(void) {
  struct fence* f;

  /* after fork() the child keeps the stack as it stands, and owns no fence */
  if(top != &root_fence && top->pid != sh_pid)
    top = &root_fence;

  /* a fence whose env is gone without sh_pop() running its finalizer */
  while(top != &root_fence && !env_within(top->env, sh))
    fence_drop();

  if(top->env == sh)
    return;

  f = alloc_zero(sizeof(*f));
  f->up = top;
  f->env = sh;
  f->pid = sh_pid;
  f->saved = head;
  f->fin.fn = dirstack_finalize;
  f->fin.next = sh->finalizers;
  sh->finalizers = &f->fin;
  top = f;
}

static struct dirnode*
node_new(const char* s, size_t len, struct dirnode* next) {
  struct dirnode* n = alloc(sizeof(*n) + len + 1);

  n->next = next;
  n->owner = top;
  n->len = len;
  byte_copy(n->path, len, s);
  n->path[len] = '\0';
  return n;
}

static size_t
depth(void) {
  struct dirnode* n;
  size_t i = 0;

  for(n = head; n; n = n->next)
    i++;

  return i;
}

struct entry {
  const char* s;
  size_t len;
};

/* replace the whole stack by v[0..n-1] (v[0] on top), all freshly allocated; the
   strings of v may point into the old nodes. Nodes of an enclosing fence are left alone. */
static void
stack_set(const struct entry* v, size_t n) {
  struct dirnode *nh = NULL, *o;

  fence_enter();

  while(n--)
    nh = node_new(v[n].s, v[n].len, nh);

  while(head && head->owner == top) {
    o = head;
    head = o->next;
    alloc_free(o);
  }

  head = nh;
}

static void
stack_push(const char* s, size_t len) {
  fence_enter();
  head = node_new(s, len, head);
}

static void
stack_pop(void) {
  struct dirnode* n;

  fence_enter();
  n = head;
  head = n->next;

  if(n->owner == top)
    alloc_free(n);
}

/* chdir through the cd builtin so $PWD, $OLDPWD and the diagnostics match */
static int
cd_to(char* name, const char* dir) {
  char* a[4];
  long ind = shell_optind, ofs = shell_optofs;
  int ret;

  a[0] = name;
  a[1] = "--";
  a[2] = (char*)dir;
  a[3] = NULL;
  shell_optind = 1;
  shell_optofs = 0;
  ret = builtin_cd(3, a);
  shell_optind = ind;
  shell_optofs = ofs;
  return ret;
}

/* "+N" / "-N": 1 and *n filled when the whole argument is a sign and digits */
static int
parse_index(const char* a, size_t* n, int* from_right) {
  unsigned long v;

  if((a[0] != '+' && a[0] != '-') || a[1] < '0' || a[1] > '9')
    return 0;

  if(scan_ulong(a + 1, &v) != str_len(a + 1))
    return 0;

  *n = v;
  *from_right = a[0] == '-';
  return 1;
}

/* entry number of "+N"/"-N" among cwd + stack (0 = cwd); -1 if out of range */
static long
resolve_index(char* argv[], char* arg, size_t n, int from_right) {
  size_t count = depth() + 1;

  if(n >= count) {
    builtin_errmsg(argv, arg, "directory stack index out of range");
    return -1;
  }

  return from_right ? (long)(count - 1 - n) : (long)n;
}

/* one stack entry; "~" for $HOME unless long_fmt */
static void
put_dir(const char* s, size_t len, int long_fmt) {
  size_t hlen;
  const char* home = var_value("HOME", &hlen);

  if(!long_fmt && hlen && len >= hlen && !byte_diff(s, hlen, home) && (len == hlen || s[hlen] == '/')) {
    buffer_putc(fd_out->w, '~');
    s += hlen;
    len -= hlen;
  }

  buffer_put(fd_out->w, s, len);
}

/* print cwd + stack; only entry "only" (>= 0) when given */
static void
show(int long_fmt, int per_line, int verbose, long only) {
  struct dirnode* n = head;
  size_t total = depth() + 1, i;
  int each_line = per_line || verbose || only >= 0;

  for(i = 0; i < total; i++) {
    const char* s = sh->cwd.s;
    size_t len = sh->cwd.len;

    if(i) {
      s = n->path;
      len = n->len;
      n = n->next;
    }

    if(only >= 0 && only != (long)i)
      continue;

    if(verbose) {
      buffer_putulong0(fd_out->w, i, 2);
      buffer_puts(fd_out->w, "  ");
    }

    put_dir(s, len, long_fmt);

    if(each_line)
      buffer_putc(fd_out->w, '\n');
    else if(i + 1 < total)
      buffer_putc(fd_out->w, ' ');
  }

  if(!each_line)
    buffer_putc(fd_out->w, '\n');

  buffer_flush(fd_out->w);
}

/* [c0 s1 .. sk] -> [ci .. sk c0 .. c(i-1)], entry i becoming the cwd */
static int
rotate(char* name, size_t idx) {
  size_t count = depth() + 1, i, m = 0;
  struct entry* v = alloc(count * sizeof(*v));
  struct entry* w = alloc(count * sizeof(*w));
  struct dirnode* n = head;
  char* oldcwd = alloc(sh->cwd.len + 1);
  int ret;

  byte_copy(oldcwd, sh->cwd.len, sh->cwd.s);
  oldcwd[sh->cwd.len] = '\0';
  v[0].s = oldcwd;
  v[0].len = sh->cwd.len;

  for(i = 1; n; n = n->next, i++) {
    v[i].s = n->path;
    v[i].len = n->len;
  }

  if((ret = cd_to(name, v[idx].s)) == 0) {
    for(i = idx + 1; i < count; i++)
      w[m++] = v[i];

    for(i = 0; i < idx; i++)
      w[m++] = v[i];

    stack_set(w, m);
  }

  alloc_free(oldcwd);
  alloc_free(v);
  alloc_free(w);
  return ret;
}

/* [c0 s1 s2 ..] -> [s1 c0 s2 ..]: change to s1, the old cwd takes its place */
static int
exchange(char* name) {
  size_t m = 0;
  struct entry* w = alloc(depth() * sizeof(*w));
  struct dirnode* n;
  char* oldcwd = alloc(sh->cwd.len + 1);
  int ret;

  byte_copy(oldcwd, sh->cwd.len, sh->cwd.s);
  oldcwd[sh->cwd.len] = '\0';
  w[m].s = oldcwd;
  w[m++].len = sh->cwd.len;

  for(n = head->next; n; n = n->next) {
    w[m].s = n->path;
    w[m++].len = n->len;
  }

  if((ret = cd_to(name, head->path)) == 0)
    stack_set(w, m);

  alloc_free(oldcwd);
  alloc_free(w);
  return ret;
}

/* drop stack entry idx (1-based over cwd + stack) without changing directory */
static void
remove_at(size_t idx) {
  size_t k = depth(), i, m = 0;
  struct entry* w = alloc(k * sizeof(*w) + 1);
  struct dirnode* n = head;

  for(i = 1; n; n = n->next, i++)
    if(i != idx) {
      w[m].s = n->path;
      w[m].len = n->len;
      m++;
    }

  stack_set(w, m);
  alloc_free(w);
}

const char help_dirs[] = "    Display the directory stack: the current directory, then the pushed ones.\n"
                         "\n"
                         "    -c              clear the stack\n"
                         "    -l              do not abbreviate $HOME as ~\n"
                         "    -p              one entry per line\n"
                         "    -v              one entry per line, numbered\n"
                         "    +N              only entry N counted from the left, starting at 0\n"
                         "    -N              only entry N counted from the right\n";

int
builtin_dirs(int argc, char* argv[]) {
  int i, long_fmt = 0, per_line = 0, verbose = 0, clear = 0, opts = 1, from_right;
  long only = -1;
  size_t n;

  for(i = 1; i < argc; i++) {
    char* a = argv[i];

    if(opts && !str_diff(a, "--")) {
      opts = 0;
    } else if(parse_index(a, &n, &from_right)) {
      if((only = resolve_index(argv, a, n, from_right)) < 0)
        return 1;
    } else if(opts && a[0] == '-' && a[1]) {
      for(a++; *a; a++)
        switch(*a) {
          case 'c': clear = 1; break;
          case 'l': long_fmt = 1; break;
          case 'p': per_line = 1; break;
          case 'v': verbose = 1; break;
          default: return builtin_invopt(argv), 2;
        }
    } else {
      return builtin_errmsg(argv, a, "invalid argument"), 2;
    }
  }

  if(clear) {
    stack_set(NULL, 0);
    return 0;
  }

  show(long_fmt, per_line, verbose, only);
  return 0;
}

const char help_pushd[] = "    Add a directory to the stack and change to it.\n"
                          "\n"
                          "    dir             push the current directory, change to dir\n"
                          "    (none)          swap the current directory and the top of the stack\n"
                          "    +N              rotate the stack so entry N (from the left) is the cwd\n"
                          "    -N              the same, counting from the right\n"
                          "    -n              with dir: add dir to the stack without changing directory\n";

int
builtin_pushd(int argc, char* argv[]) {
  int i, no_cd = 0, opts = 1, from_right = 0, have_index = 0;
  char* dir = NULL;
  size_t n = 0;
  long idx;

  for(i = 1; i < argc; i++) {
    char* a = argv[i];

    if(opts && !str_diff(a, "--")) {
      opts = 0;
    } else if(opts && !str_diff(a, "-n")) {
      no_cd = 1;
    } else if(parse_index(a, &n, &from_right)) {
      have_index = 1;
      dir = a;
    } else if(dir) {
      return builtin_errmsg(argv, a, "too many arguments"), 2;
    } else {
      dir = a;
    }
  }

  if(have_index) {
    if(no_cd)
      return builtin_errmsg(argv, "-n", "cannot rotate without changing directory"), 2;

    if((idx = resolve_index(argv, dir, n, from_right)) < 0)
      return 1;

    if(idx && rotate(argv[0], (size_t)idx))
      return 1;
  } else if(!dir) {
    if(!head)
      return builtin_errmsg(argv, "no other directory", NULL), 1;

    if(exchange(argv[0]))
      return 1;
  } else if(no_cd) {
    stack_push(dir, str_len(dir));
  } else {
    char* old = alloc(sh->cwd.len + 1);
    size_t len = sh->cwd.len;

    byte_copy(old, len, sh->cwd.s);
    old[len] = '\0';

    if(cd_to(argv[0], dir)) {
      alloc_free(old);
      return 1;
    }

    stack_push(old, len);
    alloc_free(old);
  }

  show(0, 0, 0, -1);
  return 0;
}

const char help_popd[] = "    Remove an entry from the stack.\n"
                         "\n"
                         "    (none)          remove the top entry and change to it\n"
                         "    +N              remove entry N counted from the left (+0: as no argument)\n"
                         "    -N              remove entry N counted from the right\n"
                         "    -n              do not change directory, only remove the top entry\n";

int
builtin_popd(int argc, char* argv[]) {
  int i, no_cd = 0, opts = 1, from_right = 0;
  size_t n = 0;
  long idx = 0;

  for(i = 1; i < argc; i++) {
    char* a = argv[i];

    if(opts && !str_diff(a, "--"))
      opts = 0;
    else if(opts && !str_diff(a, "-n"))
      no_cd = 1;
    else if(parse_index(a, &n, &from_right)) {
      if((idx = resolve_index(argv, a, n, from_right)) < 0)
        return 1;
    } else
      return builtin_errmsg(argv, a, "invalid argument"), 2;
  }

  if(!head)
    return builtin_errmsg(argv, "directory stack empty", NULL), 1;

  if(idx == 0) {
    idx = 1;

    if(!no_cd && cd_to(argv[0], head->path))
      return 1;
  }

  if(idx == 1)
    stack_pop();
  else
    remove_at((size_t)idx);

  show(0, 0, 0, -1);
  return 0;
}
