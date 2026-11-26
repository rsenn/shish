#include "builtin_config.h"

#if BUILTIN_CHOWN || BUILTIN_CHGRP

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char help_chown[] = "    Change file ownership.\n"
                          "\n"
                          "    -R              change files and directories recursively\n"
                          "    -h              change a symbolic link itself, not its target\n"
                          "    -f              do not report errors\n"
                          "\n"
                          "    OWNER[:GROUP]   new owner and group, each a name or a number;\n"
                          "                    OWNER alone or :GROUP changes only that one\n"
                          "    file            file(s) to change\n";

const char help_chgrp[] = "    Change file group ownership.\n"
                          "\n"
                          "    -R              change files and directories recursively\n"
                          "    -h              change a symbolic link itself, not its target\n"
                          "    -f              do not report errors\n"
                          "\n"
                          "    GROUP           new group, a name or a number\n"
                          "    file            file(s) to change\n";

struct chown_ctx {
  char** argv;
  stralloc path; /* the file being changed; grows and shrinks while walking */
  uid_t uid;     /* (uid_t)-1: leave the owner alone */
  gid_t gid;     /* (gid_t)-1: leave the group alone */
  unsigned recursive : 1, link : 1, quiet : 1, failed : 1;
};

/* id of `name` (n bytes) in a passwd or group file, "name:x:ID:...";
   0 if found, -1 if not */
static int
lookup_id(const char* file, const char* name, size_t n, unsigned long* id) {
  stralloc sa = {0};
  const char* p;
  int fd, r = -1;

  if((fd = open_read(file)) == -1)
    return -1;

  if(readclose_append(fd, &sa, 4096) < 0) {
    stralloc_free(&sa);
    return -1;
  }

  stralloc_0(&sa);

  for(p = sa.s; *p;) {
    if(!str_diffn(p, name, n) && p[n] == ':') {
      p += n + 1;
      p += str_chr(p, ':');

      if(*p && scan_ulong(p + 1, id))
        r = 0;

      break;
    }

    p += str_chr(p, '\n');

    if(*p)
      p++;
  }

  stralloc_free(&sa);
  return r;
}

/* a number is its own id; anything else is looked up in `file` */
static int
parse_id(const char* file, const char* s, size_t n, unsigned long* id) {
  if(n && scan_ulong(s, id) == n)
    return 0;

  return lookup_id(file, s, n, id);
}

/* OWNER, OWNER:GROUP or :GROUP; 0 or -1 */
static int
parse_spec(const char* spec, uid_t* uid, gid_t* gid) {
  size_t n = str_chr(spec, ':');
  unsigned long id;

  if(!spec[0])
    return -1;

  if(n) {
    if(parse_id("/etc/passwd", spec, n, &id))
      return -1;

    *uid = (uid_t)id;
  }

  if(spec[n] && spec[n + 1]) {
    if(parse_id("/etc/group", spec + n + 1, str_len(spec + n + 1), &id))
      return -1;

    *gid = (gid_t)id;
  }

  return 0;
}

static void
fail(struct chown_ctx* c) {
  c->failed = 1;

  if(!c->quiet)
    builtin_error(c->argv, c->path.s);
}

/* change c->path and, with -R, everything below it */
static void
walk(struct chown_ctx* c, int top) {
  struct dirent* e;
  struct stat st;
  size_t len = c->path.len;
  DIR* d;

  stralloc_0(&c->path);

  /* below the operand a symlink is never followed out of the tree */
  if(((c->link || !top) ? lchown(c->path.s, c->uid, c->gid) : chown(c->path.s, c->uid, c->gid)) == -1)
    fail(c);

  if(!c->recursive || lstat(c->path.s, &st) == -1 || !S_ISDIR(st.st_mode))
    return;

  if(!(d = opendir(c->path.s))) {
    fail(c);
    return;
  }

  while((e = readdir(d))) {
    if(e->d_name[0] == '.' && (!e->d_name[1] || (e->d_name[1] == '.' && !e->d_name[2])))
      continue;

    c->path.len = len;

    if(len && c->path.s[len - 1] != '/')
      stralloc_cats(&c->path, "/");

    stralloc_cats(&c->path, e->d_name);
    walk(c, 0);
  }

  closedir(d);
  c->path.len = len;
}

/* chown (group_only == 0: OWNER[:GROUP]) and chgrp (GROUP) share everything */
static int
chown_run(int argc, char* argv[], int group_only) {
  struct chown_ctx c = {0};
  unsigned long id;
  int ch, i;

  c.argv = argv;
  c.uid = (uid_t)-1;
  c.gid = (gid_t)-1;

  while((ch = shell_getopt(argc, argv, "Rhf")) > 0) {
    switch(ch) {
      case 'R': c.recursive = 1; break;
      case 'h': c.link = 1; break;
      case 'f': c.quiet = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(argc - shell_optind < 2)
    return builtin_errmsg(argv, "operand", group_only ? "missing GROUP or file" : "missing OWNER[:GROUP] or file");

  if(group_only) {
    if(parse_id("/etc/group", argv[shell_optind], str_len(argv[shell_optind]), &id))
      return builtin_errmsg(argv, argv[shell_optind], "invalid group");

    c.gid = (gid_t)id;
  } else if(parse_spec(argv[shell_optind], &c.uid, &c.gid)) {
    return builtin_errmsg(argv, argv[shell_optind], "invalid user or group");
  }

  for(i = shell_optind + 1; i < argc; i++) {
    stralloc_copys(&c.path, argv[i]);
    walk(&c, 1);
  }

  stralloc_free(&c.path);
  return c.failed;
}

int
builtin_chown(int argc, char* argv[]) {
  return chown_run(argc, argv, 0);
}

int
builtin_chgrp(int argc, char* argv[]) {
  return chown_run(argc, argv, 1);
}

#endif /* BUILTIN_CHOWN || BUILTIN_CHGRP */
