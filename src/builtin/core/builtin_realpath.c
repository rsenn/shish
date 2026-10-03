#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/path.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include <errno.h>
#include <sys/stat.h>

const char help_realpath[] = "    Print the resolved absolute pathname.\n"
                             "\n"
                             "    -E                     all but the last component must exist (default)\n"
                             "    -e                     every component must exist\n"
                             "    -q                     no diagnostics, only the exit status\n"
                             "    -L                     don't resolve symbolic links (same as -s)\n"
                             "    -P                     resolve symbolic links (default)\n"
                             "    -s                     don't resolve symbolic links (short for --strip)\n"
                             "    --relative-to=DIR      print the result relative to DIR\n"
                             "    file                   path to resolve\n";

/* removes 'n' argv slots starting at 'i', shifting the rest (and the
 * NULL terminator) down; 'argc' is adjusted in place.
 * ----------------------------------------------------------------------- */
static void
remove_args(char* argv[], int* argc, int i, int n) {
  byte_copyr(&argv[i], (*argc - i - n) * sizeof(char*), &argv[i + n]);
  *argc -= n;
}

/* pulls "--relative-to DIR" or "--relative-to=DIR" out of argv,
 * wherever it appears, returning the DIR operand (or NULL if absent).
 * ----------------------------------------------------------------------- */
static char*
extract_relative_to(char* argv[], int* argc) {
  int i;

  for(i = 1; i < *argc; i++) {
    if(str_equal(argv[i], "--relative-to")) {
      char* dir = argv[i + 1];

      if(!dir)
        return (char*)-1;

      remove_args(argv, argc, i, 2);
      return dir;
    }

    if(!str_diffn(argv[i], "--relative-to=", 14)) {
      char* dir = argv[i] + 14;

      remove_args(argv, argc, i, 1);
      return dir;
    }
  }

  return NULL;
}

/* computes 'target's path relative to 'base' -- both already
 * absolute and canonicalized (a single PATHSEP_C style throughout).
 * ----------------------------------------------------------------------- */
static void
relative_to(stralloc* out, const char* base, const char* target) {
  size_t bi = 0, ti = 0;

  if(base[bi] == PATHSEP_C)
    bi++;
  if(target[ti] == PATHSEP_C)
    ti++;

  for(;;) {
    size_t bn = path_len_s(base + bi);
    size_t tn = path_len_s(target + ti);

    if(!bn || !tn || bn != tn || byte_diff(base + bi, bn, target + ti))
      break;

    bi += bn;
    ti += tn;

    if(base[bi] != PATHSEP_C || target[ti] != PATHSEP_C)
      break;

    bi++;
    ti++;
  }

  if(base[bi] == PATHSEP_C)
    bi++;
  if(target[ti] == PATHSEP_C)
    ti++;

  out->len = 0;

  while(base[bi]) {
    size_t n = path_len_s(base + bi);

    stralloc_cats(out, "..");
    stralloc_catc(out, PATHSEP_C);
    bi += n;

    if(base[bi] == PATHSEP_C)
      bi++;
  }

  if(target[ti])
    stralloc_cats(out, target + ti);
  else if(out->len)
    out->len--;
  else
    stralloc_cats(out, ".");

  stralloc_nul(out);
}

/* does the resolved path meet mode 'e' (it exists) or 'E' (its directory exists)? */
static int
realpath_exists(const char* resolved, int mode) {
  struct stat st;
  stralloc dir;
  size_t n = str_len(resolved);
  int ok;

  if(mode == 'e')
    return stat(resolved, &st) == 0;

  while(n && resolved[n - 1] != PATHSEP_C)
    n--;

  if(n <= 1)
    return 1;

  stralloc_init(&dir);
  stralloc_copyb(&dir, resolved, n - 1);
  stralloc_nul(&dir);
  ok = stat(dir.s, &st) == 0;
  stralloc_free(&dir);
  return ok;
}

int
builtin_realpath(int argc, char* argv[]) {
  int c, symbolic = 0, ret = 0, i, mode = 'E', quiet = 0;
  stralloc sa, base, rel;
  char* relto = extract_relative_to(argv, &argc);

  if(relto == (char*)-1) {
    builtin_errmsg(argv, "--relative-to", "option requires an argument");
    return 1;
  }

  while((c = shell_getopt(argc, argv, "EeLPqs")) > 0) {
    switch(c) {
      case 'L':
      case 's': symbolic = 1; break;
      case 'P': symbolic = 0; break;
      case 'E':
      case 'e': mode = c; break;
      case 'q': quiet = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(shell_optind >= argc) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  stralloc_init(&sa);
  stralloc_init(&base);
  stralloc_init(&rel);

  if(relto) {
    base.len = 0;

    if(!path_realpath(relto, &base, symbolic, NULL)) {
      builtin_error(argv, relto);
      stralloc_free(&sa);
      stralloc_free(&base);
      stralloc_free(&rel);
      return 1;
    }

    stralloc_nul(&base);
  }

  for(i = shell_optind; i < argc; i++) {
    sa.len = 0;

    if(!path_realpath(argv[i], &sa, symbolic, NULL)) {
      if(!quiet)
        builtin_error(argv, argv[i]);

      ret = 1;
      continue;
    }

    stralloc_nul(&sa);

    if(!realpath_exists(sa.s, mode)) {
      errno = ENOENT;

      if(!quiet)
        builtin_error(argv, argv[i]);

      ret = 1;
      continue;
    }

    if(relto) {
      relative_to(&rel, base.s, sa.s);
      buffer_puts(fd_out->w, rel.s);
    } else {
      buffer_puts(fd_out->w, sa.s);
    }

    buffer_putnlflush(fd_out->w);
  }

  stralloc_free(&sa);
  stralloc_free(&base);
  stralloc_free(&rel);
  return ret;
}
