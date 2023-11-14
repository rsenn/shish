#include "../exec.h"
#include "../fdtable.h"
#include "../parse.h"
#include "../../lib/str.h"

enum type_index {
  TYPE_NONE = -1,
  TYPE_FILE,
  TYPE_ALIAS,
  TYPE_KEYWORD,
  TYPE_FUNCTION,
  TYPE_BUILTIN
};

#define TYPE_NAMES \
  ((const char* const[]){"file", "alias", "keyword", "function", "builtin"})

#define TYPE_DESCRIPTIONS \
  ((const char* const[]){ \
      0, "aliased to `", "a shell keyword", "a function", "a shell builtin"})

static inline int
is_keyword(const char* str) {
  int i;

  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return 0;
}

int
exec_type(char* name, int mask, int force_path, int type_name) {
  struct command cmd;
  enum type_index id = TYPE_NONE;
  struct alias* a;

  if(force_path)
    mask |= H_BUILTIN | H_SBUILTIN | H_FUNCTION;

  if(!force_path && (a = parse_findalias(0, name, str_len(name)))) {
    id = TYPE_ALIAS;
  } else if(!force_path && is_keyword(name)) {
    id = TYPE_KEYWORD;
  } else if((cmd = exec_hash(name, mask)).ptr) {
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
      buffer_puts(fd_out->w, TYPE_NAMES[id]);
    else
      buffer_putm_internal(fd_out->w,
                           name,
                           " is ",
                           id == TYPE_FILE ? cmd.path : TYPE_DESCRIPTIONS[id],
                           id == TYPE_ALIAS ? a->def : 0,
                           id == TYPE_ALIAS ? "'" : 0,
                           0);
    buffer_putnlflush(fd_out->w);
    return 0;
  }

  return 1;
}
