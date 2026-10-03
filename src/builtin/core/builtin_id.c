#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/alloc.h"
#include "../../../lib/windoze.h"
#include "config.h"
#include <sys/types.h>
#include <unistd.h>
#if !WINDOWS_NATIVE
#include <grp.h>
#include <pwd.h>
#endif

const char help_id[] = "    Print user and group identity.\n"
                       "\n"
                       "    -u              print only the user ID\n"
                       "    -g              print only the group ID\n"
                       "    -G              print all group IDs\n"
                       "    -n              print names instead of numbers (with -u, -g, -G)\n"
                       "    -r              print the real ID instead of the effective one\n"
                       "    user            print the identity of this user instead\n";

#if WINDOWS_NATIVE || defined(__wasi__) || defined(__wasm__)

int
builtin_id(int argc, char* argv[]) {
  builtin_errmsg(argv, "id", "not supported on this platform");
  return 1;
}

#else

/* "id" or "name" of a user or group, as the -n option asks
 * ----------------------------------------------------------------------- */
static void
id_put(unsigned long id, const char* name, int names, int paren) {
  if(names && name) {
    buffer_puts(fd_out->w, name);
    return;
  }

  buffer_putulong(fd_out->w, id);

  if(paren && name) {
    buffer_putc(fd_out->w, '(');
    buffer_puts(fd_out->w, name);
    buffer_putc(fd_out->w, ')');
  }
}

static const char*
id_gname(gid_t g) {
  struct group* gr = getgrgid(g);

  return gr ? gr->gr_name : 0;
}

static const char*
id_uname(uid_t u) {
  struct passwd* pw = getpwuid(u);

  return pw ? pw->pw_name : 0;
}

int
builtin_id(int argc, char* argv[]) {
  int c, mode = 0, names = 0, real = 0, ngroups = 0, i;
  uid_t uid, euid;
  gid_t gid, egid, *groups = 0;
  const char* user = 0;
  struct passwd* pw = 0;

  while((c = shell_getopt(argc, argv, "ugGnr")) > 0) {
    switch(c) {
      case 'u':
      case 'g':
      case 'G':
        if(mode && mode != c) {
          builtin_errmsg(argv, "cannot print more than one of -u, -g and -G", 0);
          return 1;
        }

        mode = c;
        break;
      case 'n': names = 1; break;
      case 'r': real = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if((names || real) && !mode) {
    builtin_errmsg(argv, "-n and -r need one of -u, -g and -G", 0);
    return 1;
  }

  if(argc - shell_optind > 1) {
    builtin_errmsg(argv, argv[shell_optind + 1], "extra operand");
    return 1;
  }

  if((user = argv[shell_optind])) {
    if(!(pw = getpwnam(user))) {
      builtin_errmsg(argv, (char*)user, "no such user");
      return 1;
    }

    uid = euid = pw->pw_uid;
    gid = egid = pw->pw_gid;
  } else {
    uid = getuid();
    euid = geteuid();
    gid = getgid();
    egid = getegid();
  }

  /* supplementary groups, the primary one included */
  if(!mode || mode == 'G' || !user) {
    if(pw) {
      ngroups = 32;

      for(;;) {
        groups = alloc(ngroups * sizeof(gid_t));
        int n = ngroups;

        if(getgrouplist(pw->pw_name, pw->pw_gid, groups, &n) >= 0) {
          ngroups = n;
          break;
        }

        alloc_free(groups);
        ngroups = n > ngroups ? n : ngroups * 2;
      }
    } else {
      ngroups = getgroups(0, 0);
      groups = alloc((ngroups + 1) * sizeof(gid_t));
      ngroups = ngroups > 0 ? getgroups(ngroups, groups) : 0;
    }
  }

  switch(mode) {
    case 'u': id_put(real ? uid : euid, id_uname(real ? uid : euid), names, 0); break;
    case 'g': id_put(real ? gid : egid, id_gname(real ? gid : egid), names, 0); break;
    case 'G':
      /* the effective group comes first, the others once each */
      id_put(egid, id_gname(egid), names, 0);

      for(i = 0; i < ngroups; i++) {
        if(groups[i] == egid)
          continue;

        buffer_putc(fd_out->w, ' ');
        id_put(groups[i], id_gname(groups[i]), names, 0);
      }
      break;
    default:
      buffer_puts(fd_out->w, "uid=");
      id_put(uid, id_uname(uid), 0, 1);
      buffer_puts(fd_out->w, " gid=");
      id_put(gid, id_gname(gid), 0, 1);

      if(euid != uid) {
        buffer_puts(fd_out->w, " euid=");
        id_put(euid, id_uname(euid), 0, 1);
      }

      if(egid != gid) {
        buffer_puts(fd_out->w, " egid=");
        id_put(egid, id_gname(egid), 0, 1);
      }

      {
        /* the primary group first, then the others once each */
        buffer_puts(fd_out->w, " groups=");
        id_put(gid, id_gname(gid), 0, 1);

        for(i = 0; i < ngroups; i++) {
          if(groups[i] == gid)
            continue;

          buffer_putc(fd_out->w, ',');
          id_put(groups[i], id_gname(groups[i]), 0, 1);
        }
      }
      break;
  }

  buffer_putc(fd_out->w, '\n');
  buffer_flush(fd_out->w);

  if(groups)
    alloc_free(groups);

  return 0;
}

#endif
