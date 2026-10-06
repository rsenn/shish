#include "../exec.h"
#include "../fdtable.h"
#include "../parse.h"
#include "../var.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"
#include <sys/stat.h>
#include <unistd.h>

enum type_index {
  TYPE_NONE = -1,
  TYPE_FILE,
  TYPE_ALIAS,
  TYPE_KEYWORD,
  TYPE_FUNCTION,
  TYPE_BUILTIN
};

#define TYPE_NAMES ((const char* const[]){"file", "alias", "keyword", "function", "builtin"})

#define TYPE_DESCRIPTIONS \
  ((const char* const[]){0, "aliased to `", "a shell keyword", "a function", "a shell builtin"})

static inline int
is_keyword(const char* str) {
  int i;

  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return str_equal("time", str); /* recognized by parse_pipeline(), not a token */
}

/* one result line: "name is ...", just the type (-t) or just the path (-p) */
static void
type_print(const char* name, enum type_index id, const char* path, struct alias* a, int type_name, int print_path) {
  if(print_path && !type_name) {
    if(id == TYPE_FILE) {
      buffer_puts(fd_out->w, path);
      buffer_putnlflush(fd_out->w);
    }
    return;
  }

  if(type_name)
    buffer_puts(fd_out->w, TYPE_NAMES[id]);
  else
    buffer_putm_internal(fd_out->w,
                         name,
                         " is ",
                         id == TYPE_FILE ? path : TYPE_DESCRIPTIONS[id],
                         id == TYPE_ALIAS ? alias_code(a, 0) : 0,
                         id == TYPE_ALIAS ? "'" : 0,
                         0);
  buffer_putnlflush(fd_out->w);
}

/* -a: every alias, keyword, function, builtin and $PATH file the name stands for;
 * returns 0 if at least one was printed
 * ----------------------------------------------------------------------- */
static int
type_all(char* name, int mask, int type_name, int print_path) {
  struct command cmd;
  struct alias* a;
  const char* path = var_value("PATH", NULL);
  size_t nlen = str_len(name);
  int found = 0;

  if((a = parse_findalias(0, name, nlen))) {
    type_print(name, TYPE_ALIAS, 0, a, type_name, print_path);
    found = 1;
  }

  if(is_keyword(name)) {
    type_print(name, TYPE_KEYWORD, 0, 0, type_name, print_path);
    found = 1;
  }

  if(!(mask & H_FUNCTION) && (cmd = exec_hash(name, H_SBUILTIN | H_BUILTIN)).ptr && cmd.id == H_FUNCTION) {
    type_print(name, TYPE_FUNCTION, 0, 0, type_name, print_path);
    found = 1;
  }

  if((cmd = exec_hash(name, H_FUNCTION)).ptr && (cmd.id == H_BUILTIN || cmd.id == H_SBUILTIN)) {
    type_print(name, TYPE_BUILTIN, 0, 0, type_name, print_path);
    found = 1;
  }

  /* a name with a slash is its own path */
  if(name[str_chr(name, '/')]) {
    if((cmd = exec_hash(name, H_FUNCTION | H_BUILTIN | H_SBUILTIN)).path) {
      type_print(name, TYPE_FILE, cmd.path, 0, type_name, print_path);
      found = 1;
    }
  } else if(path) {
    while(*path) {
      size_t n = str_chr(path, ':');
      stralloc full;
      struct stat st;

      stralloc_init(&full);
      stralloc_catb(&full, n ? path : ".", n ? n : 1);
      stralloc_catc(&full, '/');
      stralloc_cats(&full, name);
      stralloc_nul(&full);

      if(access(full.s, X_OK) == 0 && stat(full.s, &st) == 0 && S_ISREG(st.st_mode)) {
        type_print(name, TYPE_FILE, full.s, 0, type_name, print_path);
        found = 1;
      }

      stralloc_free(&full);
      path += n;

      if(*path == ':')
        path++;
    }
  }

  return !found;
}

int
exec_type(char* name, int mask, int force_path, int type_name, int print_path, int all) {
  struct command cmd;
  enum type_index id = TYPE_NONE;
  struct alias* a;

  if(all && !force_path)
    return type_all(name, mask, type_name, print_path);

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
    type_print(name, id, cmd.path, a, type_name, print_path);
    return 0;
  }

  return 1;
}
