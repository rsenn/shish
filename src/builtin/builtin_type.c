#include "../builtin.h"
#include "../fd.h"
#include "../exec.h"
#include "../parse.h"
#include "../sh.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"

enum type_index {
  TYPE_NONE = -1,
  TYPE_FILE,
  TYPE_ALIAS,
  TYPE_KEYWORD,
  TYPE_FUNCTION,
  TYPE_BUILTIN
};

#define TYPE_NAMES                                                                 \
  ((const char* const[]){"file", "alias", "keyword", "function", "builtin"})

#define TYPE_DESCRIPTIONS                                                          \
  ((const char* const[]){                                                          \
      0, "aliased to `", "shell keyword", "function", "shell builtin"})

static inline int
is_keyword(const char* str) {
  int i;
  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return 0;
}

/* type built-in
 * ----------------------------------------------------------------------- */
int
builtin_type(int argc, char* argv[]) {
  int c, all_locations = 0, suppress_functions = 0, force_path = 0, print_path = 0,
         type_name = 0;
  char* name;

  /* check options */
  while((c = shell_getopt(argc, argv, "afPpt")) > 0) {
    switch(c) {
      case 'a': all_locations = 1; break;
      case 'f': suppress_functions = 1; break;
      case 'P': force_path = 1; break;
      case 'p': print_path = 1; break;
      case 't': type_name = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* no arguments? return now! */
  if(!(name = argv[shell_optind]))
    return 0;

  exec_type(name, suppress_functions ? H_FUNCTION : 0, force_path, type_name);

  return 0;
}
