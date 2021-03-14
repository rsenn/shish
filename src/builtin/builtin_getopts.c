#include "../builtin.h"
#include "../var.h"

/* getopts built-in
 * ----------------------------------------------------------------------- */
int
builtin_getopts(int argc, char* argv[]) {
  const char *optstring, *name;

  if(argc < 2) {
    builtin_errmsg(argv, "optstring argument required", NULL);
    return 1;
  }
  if(argc < 3) {
    builtin_errmsg(argv, "name argument required", NULL);
    return 1;
  }

  optstring = argv[1];
  name = argv[2];

  if(!var_valid(name)) {
    builtin_errmsg(argv, name, "not a valid identifier");
    return 2;
  }

  return 0;
}
