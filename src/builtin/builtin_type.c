#include "../builtin.h"
#include "../fd.h"
#include "../exec.h"
#include "../parse.h"
#include "../sh.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"

enum type_id { TYPE_NONE = -1, TYPE_FILE, TYPE_ALIAS, TYPE_KEYWORD, TYPE_FUNCTION, TYPE_BUILTIN };

static struct {
  const char *word, *desc;
} type_descriptions[] = {{"file", 0},
                         {"alias", "aliased to `"},
                         {"keyword", "shell keyword"},
                         {"function", "function"},
                         {"builtin", "shell builtin"}};

static inline int
is_keyword(const char* str) {
  int i;
  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return 0;
}

/* type built-in
 *
 * all your drugs are belong to us
 * ----------------------------------------------------------------------- */
int
builtin_type(int argc, char* argv[]) {
  int c, all_locations = 0, suppress_functions = 0, force_path = 0, print_path = 0, type_name = 0;
  struct command cmd;
  char* name;

  /* check options, -l for login dash, -c for null env, -a to set argv[0] */
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

  {
    enum type_id id = TYPE_NONE;
    struct alias* a;
    int hash_flags = force_path ? (H_BUILTIN | H_SBUILTIN | H_FUNCTION) : suppress_functions ? H_FUNCTION : 0;

    if(!force_path && (a = parse_findalias(0, name, str_len(name)))) {
      id = TYPE_ALIAS;

    } else if(!force_path && is_keyword(name)) {
      id = TYPE_KEYWORD;
    } else if((cmd = exec_hash(name, hash_flags)).ptr) {

      switch(cmd.id) {
        case H_FUNCTION: id = TYPE_FUNCTION; break;
        case H_SBUILTIN:
        case H_BUILTIN: id = TYPE_BUILTIN; break;
        case H_EXEC:
        case H_PROGRAM: id = TYPE_FILE; break;
      }
    }

    if(id >= 0) {

      if(type_name)
        buffer_puts(fd_out->w, type_descriptions[id].word);
      else
        buffer_putm_internal(fd_out->w,
                             name,
                             " is ",
                             id == TYPE_FILE ? cmd.path : type_descriptions[id].desc,
                             id == TYPE_ALIAS ? a->def : 0,
                             id == TYPE_ALIAS ? "'" : 0,
                             0);

      buffer_putnlflush(fd_out->w);
    }

    return 0;
  }
}
