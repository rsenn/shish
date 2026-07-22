#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* apply a comma-separated symbolic mode spec ("[augo]*[+-=][rwx]*",
   repeated, comma-separated -- e.g. "u+rwx,go-w") on top of an
   existing mode. Returns 1 on success, 0 if spec doesn't parse as a
   symbolic mode at all. A simplified subset of POSIX symbolic mode:
   no "X"/"s"/"t" perm bits and no copying another class's bits
   ("g=u"), and an omitted who defaults to "a" unconditionally rather
   than being masked by umask. */
static int
chmod_symbolic(const char* spec, unsigned int* mode) {
  const char* p = spec;

  if(!*p)
    return 0;

  while(*p) {
    unsigned int who = 0;

    for(; *p == 'u' || *p == 'g' || *p == 'o' || *p == 'a'; p++) {
      switch(*p) {
        case 'u': who |= 0700; break;
        case 'g': who |= 0070; break;
        case 'o': who |= 0007; break;
        case 'a': who |= 0777; break;
      }
    }

    if(!who)
      who = 0777;

    if(*p != '+' && *p != '-' && *p != '=')
      return 0;

    while(*p == '+' || *p == '-' || *p == '=') {
      char op = *p++;
      unsigned int perm = 0;

      for(; *p == 'r' || *p == 'w' || *p == 'x'; p++) {
        switch(*p) {
          case 'r': perm |= 0444; break;
          case 'w': perm |= 0222; break;
          case 'x': perm |= 0111; break;
        }
      }

      switch(op) {
        case '+': *mode |= perm & who; break;
        case '-': *mode &= ~(perm & who); break;
        case '=': *mode = (*mode & ~who) | (perm & who); break;
      }
    }

    if(*p == ',') {
      p++;
      continue;
    }

    break;
  }

  return *p == '\0';
}

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_chmod(int argc, char* argv[]) {
  int c, ret;
  int verbose = 0;
  char *p, *spec;
  unsigned int octal_mode = 0;
  int symbolic;

  /* check options */
  while((c = shell_getopt(argc, argv, "v")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(!argv[shell_optind]) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  spec = argv[shell_optind++];

  /* a mode made up entirely of octal digits is the plain numeric
     form; anything else must parse as symbolic mode instead, applied
     relative to each file's own current mode */
  symbolic = scan_8int(spec, &octal_mode) != str_len(spec);

  while((p = argv[shell_optind++])) {
    unsigned int mode = octal_mode;

    if(symbolic) {
      struct stat st;

      if(stat(p, &st) == -1) {
        builtin_error(argv, p);
        return 1;
      }

      mode = st.st_mode & 07777;

      if(!chmod_symbolic(spec, &mode)) {
        builtin_errmsg(argv, spec, "invalid mode");
        return 1;
      }
    }

    ret = chmod(p, mode);

    if(ret == -1) {
      builtin_error(argv, p);
      return 1;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "changed mode of '", p, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }
  return 0;
}
